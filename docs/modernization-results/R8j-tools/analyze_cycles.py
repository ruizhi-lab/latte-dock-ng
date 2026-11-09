#!/usr/bin/env python3
"""Verdict of one R8j scenario (leak or allocator retention), by the rule fixed in R8j.md.

Input: a run directory written by session.py (samples.csv, events.jsonl, summary.json and
session/probe.log). Output: <run dir>/analysis.json and analysis.md, and the same files
in the committed copy when it is passed as the run directory. Standard library only; the
output has no timestamps, so the same inputs give the same files.

    python3 -I analyze_cycles.py <run dir> [--out <dir>]

The levels, the rule and its thresholds are the ones written in R8j.md ("Measured
quantities" and "Verdict rule (fixed)"). They are constants here, not options.
"""

from __future__ import annotations

import argparse
import csv
import json
import re
import sys
from pathlib import Path

MIB_KIB = 1024.0
MIB_BYTES = 1024.0 * 1024.0
MIN_GROWTH_MIB = 5.0  # G below this: not reproduced
STEP_MIB = 1.0  # a cycle reproduces a step when its peak rises this much above the level before it
GROWING_CYCLE_MIB = 0.5  # a cycle counts as growing when its post-cycle level rises this much
MOST_CYCLES = 8  # of the ten cycles: "growth on most cycles"
EARLY_CYCLES = 3  # growth "only in the first cycles": cycles 4 to 10 add less than LATE_MIB
LATE_MIB = 1.0
TRIM_RETAINED = 0.5  # TR >= 0.5 G: allocator retention
TRIM_HELD = 0.25  # TR < 0.25 G: the trim returns little
MALLOC_LIVE = 0.5  # dU >= 0.5 G: the growth is in glibc's in-use bytes
CYCLES = 10  # the verdict uses cycles 1 to 10; cycles 11 and 12 are the post-trim check
WINDOW_S = 15.0  # a level is the mean of the samples in the last 15 s of its window
TRIM_WINDOW = (15.0, 25.0)  # after a trim: the samples 15 to 25 s later

PROBE_LINE = re.compile(
    r"^(?P<time>\S+) (?P<kind>trim-before|trim-after|dump) seq=(?P<seq>\d+) rc=(?P<rc>-?\d+) "
    r"uordblks=(?P<uordblks>\d+) fordblks=(?P<fordblks>\d+) hblkhd=(?P<hblkhd>\d+) "
    r"arena=(?P<arena>\d+) keepcost=(?P<keepcost>\d+) ordblks=(?P<ordblks>\d+)$"
)


# --- parsing -----------------------------------------------------------------------------


def parse_probe(lines: list) -> dict:
    """Probe records keyed by seq (the signal index). Unparseable lines are reported, not skipped."""
    records: dict = {}
    bad = []
    for line in lines:
        text = line.rstrip("\n")
        if not text:
            continue
        match = PROBE_LINE.match(text)
        if match is None:
            bad.append(text)
            continue
        record = {key: match.group(key) for key in ("kind", "time")}
        record.update({key: int(match.group(key)) for key in ("seq", "rc", "uordblks", "fordblks", "hblkhd", "arena", "keepcost", "ordblks")})
        records.setdefault(record["seq"], []).append(record)
    return {"records": records, "unparsed": bad}


def load_samples(path: Path) -> list:
    rows = []
    with path.open(encoding="utf-8", newline="") as handle:
        for raw in csv.DictReader(handle):
            row = {"t": float(raw["elapsed_s"])}
            for key in ("Pss_Anon", "Rss", "Pss", "Threads", "Fds", "CpuTicks"):
                value = raw.get(key, "")
                row[key] = float(value) if value not in ("", None) else None
            rows.append(row)
    rows.sort(key=lambda row: row["t"])
    return rows


def load_events(path: Path) -> list:
    events = []
    with path.open(encoding="utf-8") as handle:
        for line in handle:
            if line.strip():
                events.append(json.loads(line))
    return events


# --- levels ------------------------------------------------------------------------------


def window_values(samples: list, key: str, start: float, end: float) -> list:
    """Values with start < t <= end (a window that ends at an event excludes samples before it)."""
    return [row[key] for row in samples if start < row["t"] <= end and row.get(key) is not None]


def mean_level(samples: list, start: float, end: float):
    values = window_values(samples, "Pss_Anon", start, end)
    if not values:
        return None
    return sum(values) / len(values) / MIB_KIB


def peak_level(samples: list, start: float, end: float):
    values = window_values(samples, "Pss_Anon", start, end)
    if not values:
        return None
    return max(values) / MIB_KIB


def peak_threads(samples: list, start: float, end: float):
    values = window_values(samples, "Threads", start, end)
    return max(values) if values else None


def find_event(events: list, event: str, **match):
    for item in events:
        if item.get("event") == event and all(item.get(k) == v for k, v in match.items()):
            return item
    return None


def signal_by_label(events: list, label: str):
    return find_event(events, "signal", label=label)


def probe_record(probe: dict, seq: int, kind: str):
    for record in probe["records"].get(seq, []):
        if record["kind"] == kind:
            return record
    return None


def mib(value_bytes):
    return None if value_bytes is None else value_bytes / MIB_BYTES


# --- the scenario ------------------------------------------------------------------------


def levels_of(samples: list, events: list, probe: dict) -> dict:
    """Every level and counter the verdict uses, with None where the data are missing."""
    warm = find_event(events, "warmup-end")
    trim0 = signal_by_label(events, "trim at warm-up end: baseline")
    trim_after = signal_by_label(events, "trim after the cycles")
    result = {
        "pre_trim": None,
        "post_trim_baseline": None,
        "cycles": {},
        "trimmed": None,
        "post_trim_cycles": {},
    }
    if warm is not None:
        result["pre_trim"] = mean_level(samples, warm["elapsed_s"] - WINDOW_S, warm["elapsed_s"])
    if trim0 is not None:
        start, end = trim0["elapsed_s"] + TRIM_WINDOW[0], trim0["elapsed_s"] + TRIM_WINDOW[1]
        result["post_trim_baseline"] = mean_level(samples, start, end)
        record = probe_record(probe, trim0["index"], "trim-after")
        result["baseline_counters"] = counters(record)
    if trim_after is not None:
        start, end = trim_after["elapsed_s"] + TRIM_WINDOW[0], trim_after["elapsed_s"] + TRIM_WINDOW[1]
        result["trimmed"] = mean_level(samples, start, end)
        result["trimmed_counters"] = counters(probe_record(probe, trim_after["index"], "trim-after"))
    for cycle in range(1, CYCLES + 3):
        cycle_key = str(cycle)
        start_event = find_event(events, "cycle-start", cycle=cycle)
        rest_event = find_event(events, "rest-end", cycle=cycle)
        dump = signal_by_label(events, f"dump after cycle {cycle}")
        entry = {"start": None, "rest_end": None, "level": None, "peak": None, "threads_peak": None, "counters": None}
        if start_event is not None and rest_event is not None:
            entry["start"] = start_event["elapsed_s"]
            entry["rest_end"] = rest_event["elapsed_s"]
            entry["level"] = mean_level(samples, rest_event["elapsed_s"] - WINDOW_S, rest_event["elapsed_s"])
            entry["peak"] = peak_level(samples, start_event["elapsed_s"], rest_event["elapsed_s"])
            entry["threads_peak"] = peak_threads(samples, start_event["elapsed_s"], rest_event["elapsed_s"])
        if dump is not None:
            entry["counters"] = counters(probe_record(probe, dump["index"], "dump"))
        if cycle <= CYCLES:
            result["cycles"][cycle_key] = entry
        else:
            result["post_trim_cycles"][cycle_key] = entry
    return result


def counters(record):
    if record is None:
        return None
    return {
        "uordblks_mib": mib(record["uordblks"]),
        "fordblks_mib": mib(record["fordblks"]),
        "hblkhd_mib": mib(record["hblkhd"]),
        "arena_mib": mib(record["arena"]),
    }


def verdict(levels: dict) -> dict:
    """The fixed rule of R8j.md, first matching row. Returns the label and every quantity."""
    cycles = levels["cycles"]
    base = levels["post_trim_baseline"]
    trimmed = levels["trimmed"]
    last = cycles.get(str(CYCLES), {})
    missing = []
    if base is None:
        missing.append("post-trim baseline level")
    if last.get("level") is None:
        missing.append("cycle 10 level")
    if trimmed is None:
        missing.append("trim level after the cycles")
    if last.get("counters") is None or levels.get("baseline_counters") is None:
        missing.append("allocator counters")
    if missing:
        return {"label": "insufficient data", "missing": missing}
    growth = last["level"] - base
    trim_release = last["level"] - trimmed
    d_use = last["counters"]["uordblks_mib"] - levels["baseline_counters"]["uordblks_mib"]
    d_free = last["counters"]["fordblks_mib"] - levels["baseline_counters"]["fordblks_mib"]
    steps = []
    previous = base
    for number in range(1, CYCLES + 1):
        entry = cycles[str(number)]
        if entry.get("level") is None or entry.get("peak") is None:
            return {"label": "insufficient data", "missing": [f"cycle {number} window"]}
        steps.append(entry["level"] - previous)
        previous = entry["level"]
    n_up = sum(1 for step in steps if step >= GROWING_CYCLE_MIB)
    cycle3 = cycles["3"]["level"]
    late = last["level"] - cycle3
    reproduced = 0
    previous = base
    for number in range(1, CYCLES + 1):
        if cycles[str(number)]["peak"] - previous >= STEP_MIB:
            reproduced += 1
        previous = cycles[str(number)]["level"]
    detail = {
        "growth_mib": growth,
        "trim_release_mib": trim_release,
        "delta_in_use_mib": d_use,
        "delta_free_mib": d_free,
        "cycle_steps_mib": steps,
        "cycles_growing": n_up,
        "late_growth_mib": late,
        "cycles_with_step": reproduced,
    }
    if growth < MIN_GROWTH_MIB:
        label = "not reproduced"
    elif trim_release >= TRIM_RETAINED * growth:
        if late < LATE_MIB:
            pattern = "growth only in the first cycles"
        elif n_up >= MOST_CYCLES:
            pattern = "growth on most cycles"
        else:
            pattern = "growth spread over the cycles"
        label = "allocator retention"
        detail["pattern"] = pattern
    elif trim_release < TRIM_HELD * growth and d_use >= MALLOC_LIVE * growth:
        label = "live growth (leak candidate)" if n_up >= MOST_CYCLES else "live growth that stopped"
    elif trim_release < TRIM_HELD * growth:
        label = "outside malloc"
    else:
        label = "inconclusive"
    return {"label": label, **detail}


def retention_check(levels: dict, verdict_result: dict) -> dict:
    """The post-trim cycles: retention predicts that cycle 11 takes back at least half of the trim release."""
    trimmed = levels["trimmed"]
    trim_release = verdict_result.get("trim_release_mib")
    out = {}
    for number in (11, 12):
        entry = levels["post_trim_cycles"].get(str(number), {})
        level = entry.get("level")
        out[str(number)] = None if level is None or trimmed is None else level - trimmed
    out["trim_release_mib"] = trim_release
    return out


def analyze(run_dir: Path) -> dict:
    samples = load_samples(run_dir / "samples.csv")
    events = load_events(run_dir / "events.jsonl")
    probe_path = run_dir / "session" / "probe.log"
    probe = parse_probe(probe_path.read_text(encoding="utf-8").splitlines() if probe_path.exists() else [])
    summary_path = run_dir / "summary.json"
    summary = json.loads(summary_path.read_text(encoding="utf-8")) if summary_path.exists() else None
    levels = levels_of(samples, events, probe)
    result = verdict(levels)
    report = {
        "run_dir": str(run_dir),
        "scenario": summary["scenario"] if summary else None,
        "reason": summary["reason"] if summary else "no summary.json",
        "levels": levels,
        "verdict": result,
        "checks": retention_check(levels, result),
        "probe_unparsed_lines": probe["unparsed"],
        "probe_lines": sum(len(v) for v in probe["records"].values()),
        "signals": sum(1 for e in events if e.get("event") == "signal"),
        "samples": len(samples),
    }
    return report


def fmt(value, digits: int = 2) -> str:
    return "-" if value is None else f"{value:,.{digits}f}"


def render_markdown(report: dict) -> str:
    levels = report["levels"]
    verdict_result = report["verdict"]
    lines = [f"# R8j cycle analysis: scenario {report['scenario']}", ""]
    lines.append(f"Run: `{report['run_dir']}`. Session reason: {report['reason']}. Samples: {report['samples']}; signals: {report['signals']}; probe lines: {report['probe_lines']}.")
    lines.append("")
    lines.append(f"**Verdict: {verdict_result['label']}**")
    lines.append("")
    if "missing" in verdict_result:
        lines.append("Missing: " + ", ".join(verdict_result["missing"]) + ".")
        lines.append("")
    else:
        lines.append(
            f"G = A_10 - A_0 = {fmt(verdict_result['growth_mib'])} MiB; TR = A_10 - T = "
            f"{fmt(verdict_result['trim_release_mib'])} MiB; dU (glibc in use) = {fmt(verdict_result['delta_in_use_mib'])} MiB; "
            f"dF (glibc free) = {fmt(verdict_result['delta_free_mib'])} MiB; growing cycles = {verdict_result['cycles_growing']}; "
            f"late growth (cycles 4 to 10) = {fmt(verdict_result['late_growth_mib'])} MiB; cycles with a step of at least 1 MiB = "
            f"{verdict_result['cycles_with_step']}."
        )
        if "pattern" in verdict_result:
            lines.append(f"Pattern: {verdict_result['pattern']}.")
        lines.append("")
    lines.append("## Levels (MiB, Pss_Anon, and glibc counters after the rest)")
    lines.append("")
    lines.append("| cycle | action | A_k | step | peak | peak - previous level | threads peak | uordblks | fordblks |")
    lines.append("|---:|---|---:|---:|---:|---:|---:|---:|---:|")
    previous = levels["post_trim_baseline"]
    lines.append(
        f"| 0 | post-trim baseline | {fmt(previous)} | - | - | - | - | "
        f"{fmt((levels.get('baseline_counters') or {}).get('uordblks_mib'))} | {fmt((levels.get('baseline_counters') or {}).get('fordblks_mib'))} |"
    )
    for number in range(1, CYCLES + 3):
        entry = levels["cycles"].get(str(number)) or levels["post_trim_cycles"].get(str(number)) or {}
        level = entry.get("level")
        step = None if level is None or previous is None else level - previous
        peak_gap = None if entry.get("peak") is None or previous is None else entry["peak"] - previous
        counters_entry = entry.get("counters") or {}
        label = str(number) if number <= CYCLES else f"{number} (post-trim)"
        lines.append(
            f"| {label} | {'idle' if report['scenario'] == 'I' else 'window' if report['scenario'] == 'W' else 'application'} | "
            f"{fmt(level)} | {fmt(step)} | {fmt(entry.get('peak'))} | {fmt(peak_gap)} | "
            f"{fmt(entry.get('threads_peak'), 0)} | {fmt(counters_entry.get('uordblks_mib'))} | {fmt(counters_entry.get('fordblks_mib'))} |"
        )
        if level is not None:
            previous = level
    lines.append("")
    lines.append("## Trim")
    lines.append("")
    lines.append(
        f"- Before the baseline trim (warm-up end): {fmt(levels['pre_trim'])} MiB; after it: {fmt(levels['post_trim_baseline'])} MiB."
    )
    lines.append(f"- After cycle 10: {fmt(levels['cycles'].get(str(CYCLES), {}).get('level'))} MiB; after the trim: {fmt(levels['trimmed'])} MiB.")
    checks = report["checks"]
    lines.append(
        f"- Post-trim check (supporting, not in the rule): cycle 11 takes back {fmt(checks.get('11'))} MiB and cycle 12 "
        f"{fmt(checks.get('12'))} MiB of the trim release of {fmt(checks.get('trim_release_mib'))} MiB."
    )
    lines.append("")
    if report["probe_unparsed_lines"]:
        lines.append(f"Unparsed probe lines: {len(report['probe_unparsed_lines'])}.")
        lines.append("")
    return "\n".join(lines)


def main(argv: list | None = None) -> int:
    parser = argparse.ArgumentParser(description="Verdict of one R8j scenario by the fixed rule.")
    parser.add_argument("run_dir", type=Path)
    parser.add_argument("--out", type=Path, help="directory for analysis.json and analysis.md (default: the run directory)")
    args = parser.parse_args(sys.argv[1:] if argv is None else argv)
    report = analyze(args.run_dir)
    out = args.out or args.run_dir
    out.mkdir(parents=True, exist_ok=True)
    (out / "analysis.json").write_text(json.dumps(report, indent=1, sort_keys=True) + "\n", encoding="utf-8")
    (out / "analysis.md").write_text(render_markdown(report), encoding="utf-8")
    print(f"scenario {report['scenario']}: {report['verdict']['label']}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
