#!/usr/bin/env python3
"""Analyze idle memory series written by memsample.py (R8d idle study).

Inputs are sampler output directories with samples.csv, snapshots/*.json and
summary.json. The nested session directory holds the series 'nested-dock' (the dock
under test) and 'nested-kwin' (the KWin control). The desktop series, when present,
is 'desktop-dock' in its own directory.

Method, fixed before the run:
- the first 15 minutes after the first sample are warm-up and are dropped;
- the rest is cut into 5-minute buckets, each reduced to its median (a bucket needs at
  least 5 of its 10 samples);
- a metric's slope is the least-squares slope of the bucket medians against hours,
  with a 95 % confidence interval from the Student t distribution with n-2 degrees of
  freedom; the whole post-warm-up window and its first and second halves are fitted
  separately;
- the verdict for the dock's Pss_Anon: "no growth" when the whole-run slope interval
  includes 0 or its upper bound is below 1 MiB/h; otherwise "plateau" when the
  first-half slope exceeds 1 MiB/h and the second-half slope does not; otherwise
  "growth", with the hours the whole-run slope needs to add 100 MiB;
- the mapping table compares the 15-minute snapshot with the end snapshot per object
  (a file path, or the anonymous mappings of one permission set), by Pss and by
  Anonymous, because this kernel reports no per-mapping Pss_Anon.

Output: <out>/analysis.md and <out>/analysis.json. Standard library only. The output
has no timestamps, so the same inputs give the same files.
"""

from __future__ import annotations

import argparse
import csv
import json
import math
import statistics
import sys
from pathlib import Path

BUCKET_S = 300.0
MIN_BUCKET_SAMPLES = 5
BOUND_KIB_PER_H = 1024.0  # 1 MiB/h
HUNDRED_MIB_KIB = 100 * 1024.0
METRICS = ("Pss_Anon", "Rss", "Pss_File")
VERDICT_METRIC = "Pss_Anon"
WINDOWS = ("whole", "first_half", "second_half")
NON_NUMERIC = ("elapsed_s", "wall_time", "series", "comm")


# --- statistics ----------------------------------------------------------------------


def _betacf(a: float, b: float, x: float) -> float:
    """Continued fraction of the regularized incomplete beta function (modified Lentz)."""
    tiny, eps = 1e-300, 3e-14
    qab, qap, qam = a + b, a + 1.0, a - 1.0
    c = 1.0
    d = 1.0 - qab * x / qap
    d = 1.0 / (d if abs(d) > tiny else tiny)
    h = d
    for m in range(1, 301):
        m2 = 2 * m
        aa = m * (b - m) * x / ((qam + m2) * (a + m2))
        d = 1.0 + aa * d
        d = 1.0 / (d if abs(d) > tiny else tiny)
        c = 1.0 + aa / c
        c = c if abs(c) > tiny else tiny
        h *= d * c
        aa = -(a + m) * (qab + m) * x / ((a + m2) * (qap + m2))
        d = 1.0 + aa * d
        d = 1.0 / (d if abs(d) > tiny else tiny)
        c = 1.0 + aa / c
        c = c if abs(c) > tiny else tiny
        delta = d * c
        h *= delta
        if abs(delta - 1.0) < eps:
            break
    return h


def _betainc(a: float, b: float, x: float) -> float:
    if x <= 0.0:
        return 0.0
    if x >= 1.0:
        return 1.0
    log_front = math.lgamma(a + b) - math.lgamma(a) - math.lgamma(b) + a * math.log(x) + b * math.log1p(-x)
    front = math.exp(log_front)
    if x < (a + 1.0) / (a + b + 2.0):
        return front * _betacf(a, b, x) / a
    return 1.0 - front * _betacf(b, a, 1.0 - x) / b


def t_cdf(t: float, df: float) -> float:
    x = df / (df + t * t)
    tail = 0.5 * _betainc(df / 2.0, 0.5, x)
    return 1.0 - tail if t >= 0 else tail


def t_quantile(p: float, df: int) -> float:
    """Inverse of the Student t distribution function, found by bisection (p > 0.5)."""
    lo, hi = 0.0, 1.0e3
    for _ in range(200):
        mid = 0.5 * (lo + hi)
        if t_cdf(mid, df) < p:
            lo = mid
        else:
            hi = mid
    return 0.5 * (lo + hi)


def linear_fit(xs: list, ys: list):
    """Least-squares slope with a 95 % confidence interval, or None with fewer than 3 points."""
    n = len(xs)
    if n < 3:
        return None
    mean_x = sum(xs) / n
    mean_y = sum(ys) / n
    sxx = sum((x - mean_x) ** 2 for x in xs)
    if sxx <= 0.0:
        return None
    sxy = sum((x - mean_x) * (y - mean_y) for x, y in zip(xs, ys))
    slope = sxy / sxx
    intercept = mean_y - slope * mean_x
    sse = sum((y - intercept - slope * x) ** 2 for x, y in zip(xs, ys))
    standard_error = math.sqrt(max(sse, 0.0) / (n - 2) / sxx)
    half_width = t_quantile(0.975, n - 2) * standard_error
    return {"n": n, "slope": slope, "lo": slope - half_width, "hi": slope + half_width}


# --- data ----------------------------------------------------------------------------


def load_samples(path: Path) -> dict:
    """Return {series: [row, ...]} with numeric fields as floats (None when the cell is empty)."""
    series: dict = {}
    with path.open(encoding="utf-8", newline="") as handle:
        for raw in csv.DictReader(handle):
            row = {"t": float(raw["elapsed_s"])}
            for key, value in raw.items():
                if key is None or key in NON_NUMERIC:
                    continue
                row[key] = float(value) if value not in ("", None) else None
            series.setdefault(raw["series"], []).append(row)
    for rows in series.values():
        rows.sort(key=lambda row: row["t"])
    return series


def bucket_medians(points: list, warmup_s: float) -> list:
    """Median per 5-minute bucket after warm-up, as (hours since warm-up end, median) pairs."""
    buckets: dict = {}
    for t, value in points:
        if t < warmup_s:
            continue
        buckets.setdefault(int((t - warmup_s) // BUCKET_S), []).append(value)
    result = []
    for index in sorted(buckets):
        values = buckets[index]
        if len(values) < MIN_BUCKET_SAMPLES:
            continue
        centre_s = (index + 0.5) * BUCKET_S
        result.append((centre_s / 3600.0, statistics.median(values)))
    return result


# smaps_rollup is hidden for a non-dumpable process (KWin has file capabilities), so the
# control series falls back to the readable status fields. Those are RSS, not PSS.
FALLBACK_COLUMNS = {"Pss_Anon": "RssAnon", "Rss": "VmRSS", "Pss_File": "RssFile"}


def fit_metric(rows: list, metric: str, warmup_s: float):
    source = metric
    if all(row.get(metric) is None for row in rows) and metric in FALLBACK_COLUMNS:
        source = FALLBACK_COLUMNS[metric]
    points = [(row["t"], row[source]) for row in rows if row.get(source) is not None]
    buckets = bucket_medians(points, warmup_s)
    if not buckets:
        return None
    xs = [x for x, _ in buckets]
    ys = [y for _, y in buckets]
    half = len(buckets) // 2
    return {
        "source": source,
        "buckets": len(buckets),
        "whole": linear_fit(xs, ys),
        "first_half": linear_fit(xs[:half], ys[:half]),
        "second_half": linear_fit(xs[half:], ys[half:]),
        "first_kib": ys[0],
        "last_kib": ys[-1],
        "min_kib": min(ys),
        "max_kib": max(ys),
    }


def verdict_for(metrics: dict) -> dict:
    """Apply the fixed verdict rule to the dock's Pss_Anon fit."""
    fit = metrics.get(VERDICT_METRIC)
    if fit is None or fit["whole"] is None:
        return {"verdict": "insufficient data", "detail": ""}
    whole = fit["whole"]
    detail = (
        f"whole-run {VERDICT_METRIC} (column {fit['source']}) slope {whole['slope']:,.0f} KiB/h "
        f"({whole['slope'] / 1024:.2f} MiB/h), 95 % CI [{whole['lo']:,.0f}, {whole['hi']:,.0f}] KiB/h"
    )
    if whole["lo"] <= 0.0 <= whole["hi"] or whole["hi"] < BOUND_KIB_PER_H:
        return {"verdict": "no growth", "detail": detail}
    first, second = fit["first_half"], fit["second_half"]
    if first and second and first["slope"] > BOUND_KIB_PER_H and second["slope"] <= BOUND_KIB_PER_H:
        return {"verdict": "plateau", "detail": detail}
    hours = HUNDRED_MIB_KIB / whole["slope"] if whole["slope"] > 0 else float("inf")
    return {"verdict": "growth", "detail": f"{detail}; +100 MiB in {hours:.1f} h at this slope"}


def cpu_percent(rows: list, warmup_s: float, clk_tck: float) -> float | None:
    """Mean CPU use of the process over the post-warm-up window, from its CpuTicks column."""
    after = [row for row in rows if row["t"] >= warmup_s and row.get("CpuTicks") is not None]
    if len(after) < 2:
        return None
    first, last = after[0], after[-1]
    seconds = last["t"] - first["t"]
    if seconds <= 0:
        return None
    return (last["CpuTicks"] - first["CpuTicks"]) / clk_tck / seconds * 100.0


# --- mappings ------------------------------------------------------------------------


def load_snapshot(run_dir: Path, label: str, series: str):
    path = run_dir / "snapshots" / f"{label}-{series}.json"
    if not path.exists():
        return None
    return json.loads(path.read_text(encoding="utf-8"))


def mapping_changes(before: dict, after: dict, top: int = 10) -> dict:
    zero = {"mappings": 0, "Pss": 0, "Anonymous": 0, "Rss": 0}
    keys = sorted(set(before["by_object"]) | set(after["by_object"]))
    rows = []
    for key in keys:
        old = {**zero, **before["by_object"].get(key, {})}
        new = {**zero, **after["by_object"].get(key, {})}
        rows.append(
            {
                "object": key,
                "mappings_before": old["mappings"],
                "mappings_after": new["mappings"],
                "delta": {name: new[name] - old[name] for name in ("Pss", "Anonymous", "Rss")},
            }
        )
    rows.sort(key=lambda row: (-abs(row["delta"]["Pss"]), row["object"]))
    totals = {
        "pss_before": sum(v["Pss"] for v in before["by_object"].values()),
        "pss_after": sum(v["Pss"] for v in after["by_object"].values()),
        "delta": {name: sum(row["delta"][name] for row in rows) for name in ("Pss", "Anonymous", "Rss")},
    }
    return {"top": rows[:top], "totals": totals, "objects": len(keys)}


# --- report --------------------------------------------------------------------------


GPU_LABELS = ("0min", "15min", "75min", "end")


def gpu_at_snapshots(runs: list, series: str) -> dict:
    """GPU memory (MiB, nvidia-smi) of the series at each snapshot; None where it is missing."""
    values: dict = {}
    for label in GPU_LABELS:
        values[label] = None
        for _, run_dir in runs:
            snapshot = load_snapshot(run_dir, label, series)
            if snapshot is not None:
                values[label] = snapshot.get("gpu_mib")
                break
    return values


def analyze(args: argparse.Namespace) -> dict:
    warmup_s = args.warmup_min * 60.0
    runs = [("nested", args.nested)]
    if args.desktop is not None:
        runs.append(("desktop", args.desktop))

    series_rows: dict = {}
    run_info: dict = {}
    for name, run_dir in runs:
        for series, rows in load_samples(run_dir / "samples.csv").items():
            series_rows[series] = rows
        summary_path = run_dir / "summary.json"
        run_info[name] = {
            "directory": str(run_dir),
            "summary": json.loads(summary_path.read_text(encoding="utf-8")) if summary_path.exists() else None,
        }

    report: dict = {"warmup_min": args.warmup_min, "bucket_min": BUCKET_S / 60.0, "series": {}, "runs": run_info}
    for series in sorted(series_rows):
        rows = series_rows[series]
        clk_tck = 100.0
        summary = None
        for info in run_info.values():
            if info["summary"] is not None:
                summary = info["summary"]
                clk_tck = float(summary.get("clk_tck", clk_tck))
        metrics = {metric: fit_metric(rows, metric, warmup_s) for metric in METRICS}
        after = [row["t"] for row in rows if row["t"] >= warmup_s]
        report["series"][series] = {
            "samples": len(rows),
            "first_s": rows[0]["t"],
            "last_s": rows[-1]["t"],
            "post_warmup_span_h": (after[-1] - after[0]) / 3600.0 if len(after) > 1 else 0.0,
            "mean_cpu_percent": cpu_percent(rows, warmup_s, clk_tck),
            "metrics": metrics,
            "verdict": verdict_for(metrics),
            "gpu_mib": gpu_at_snapshots(runs, series),
        }

    report["mappings"] = {}
    for series in sorted(series_rows):
        for name, run_dir in runs:
            before = load_snapshot(run_dir, "15min", series)
            after = load_snapshot(run_dir, "end", series)
            if before is not None and after is not None and "by_object" in before and "by_object" in after:
                report["mappings"][series] = mapping_changes(before, after)
                break
    return report


def fmt_mib(kib: float | None) -> str:
    return "-" if kib is None else f"{kib / 1024:,.2f}"


def fmt_fit(fit: dict | None) -> tuple:
    if fit is None:
        return ("-", "-", "-")
    return (f"{fit['slope']:,.0f}", f"[{fit['lo']:,.0f}, {fit['hi']:,.0f}]", f"{fit['slope'] / 1024:,.3f}")


def render_markdown(report: dict) -> str:
    lines = ["# R8d idle memory analysis", ""]
    lines.append(
        f"Warm-up dropped: the first {report['warmup_min']:g} minutes of each series. Buckets: "
        f"{report['bucket_min']:g}-minute medians of at least {MIN_BUCKET_SAMPLES} samples. Slopes: least squares "
        "of bucket medians against hours; intervals are 95 % (Student t, n-2 degrees of freedom). "
        "Levels are bucket medians in MiB."
    )
    lines.append("")
    lines.append(
        f"Verdict rule, `{VERDICT_METRIC}` of the series: no growth when the whole-run slope interval includes 0 "
        "or its upper bound is below 1 MiB/h; plateau when the first-half slope exceeds 1 MiB/h and the second-half "
        "slope does not; otherwise growth."
    )
    lines.append("")
    lines.append("## Series")
    lines.append("")
    lines.append("| series | samples | span (h) | post-warm-up span (h) | mean CPU % | verdict |")
    lines.append("|---|---:|---:|---:|---:|---|")
    for series, data in report["series"].items():
        cpu = "-" if data["mean_cpu_percent"] is None else f"{data['mean_cpu_percent']:.2f}"
        lines.append(
            f"| {series} | {data['samples']} | {(data['last_s'] - data['first_s']) / 3600:.2f} | "
            f"{data['post_warmup_span_h']:.2f} | {cpu} | {data['verdict']['verdict']} |"
        )
    lines.append("")
    lines.append("## Slopes (KiB/h) and levels (MiB)")
    lines.append("")
    lines.append(
        "| series | metric | window | buckets | slope KiB/h | 95 % CI KiB/h | slope MiB/h | first | last | min | max |"
    )
    lines.append("|---|---|---|---:|---:|---|---:|---:|---:|---:|---:|")
    for series, data in report["series"].items():
        for metric in METRICS:
            fit = data["metrics"][metric]
            if fit is None:
                lines.append(f"| {series} | {metric} | whole | 0 | - | - | - | - | - | - | - |")
                continue
            label = metric if fit["source"] == metric else f"{metric} (from {fit['source']})"
            for window in WINDOWS:
                slope, interval, mib = fmt_fit(fit[window])
                if window == "whole":
                    levels = (
                        fmt_mib(fit["first_kib"]),
                        fmt_mib(fit["last_kib"]),
                        fmt_mib(fit["min_kib"]),
                        fmt_mib(fit["max_kib"]),
                    )
                else:
                    levels = ("-", "-", "-", "-")
                lines.append(
                    f"| {series} | {label} | {window.replace('_', ' ')} | {fit['buckets']} | {slope} | "
                    f"{interval} | {mib} | {' | '.join(levels)} |"
                )
    lines.append("")
    lines.append("## Verdicts")
    lines.append("")
    for series, data in report["series"].items():
        lines.append(f"- **{series}**: {data['verdict']['verdict']}. {data['verdict']['detail']}")
    lines.append("")
    lines.append("## GPU memory at the snapshots (nvidia-smi, MiB)")
    lines.append("")
    lines.append("| series | 0 min | 15 min | 75 min | end |")
    lines.append("|---|---:|---:|---:|---:|")
    for series, data in report["series"].items():
        cells = ["-" if data["gpu_mib"][label] is None else f"{data['gpu_mib'][label]:,.0f}" for label in GPU_LABELS]
        lines.append(f"| {series} | " + " | ".join(cells) + " |")
    lines.append("")
    lines.append("## Mappings: 15-minute snapshot to end")
    lines.append("")
    lines.append(
        "Per object (file path, or anonymous mappings of one permission set), largest absolute change in Pss first. "
        "Totals sum all objects. MiB."
    )
    lines.append("")
    for series, table in report["mappings"].items():
        totals = table["totals"]
        lines.append(f"### {series}")
        lines.append("")
        lines.append(
            f"Total Pss {totals['pss_before'] / 1024:,.2f} -> {totals['pss_after'] / 1024:,.2f} MiB "
            f"(delta {totals['delta']['Pss'] / 1024:+,.2f}); Anonymous delta "
            f"{totals['delta']['Anonymous'] / 1024:+,.2f}; Rss delta {totals['delta']['Rss'] / 1024:+,.2f}; "
            f"{table['objects']} objects."
        )
        lines.append("")
        lines.append("| object | mappings 15 min -> end | delta Pss | delta Anonymous | delta Rss |")
        lines.append("|---|---:|---:|---:|---:|")
        for row in table["top"]:
            delta = row["delta"]
            lines.append(
                f"| {row['object']} | {row['mappings_before']} -> {row['mappings_after']} | "
                f"{delta['Pss'] / 1024:+,.2f} | {delta['Anonymous'] / 1024:+,.2f} | {delta['Rss'] / 1024:+,.2f} |"
            )
        lines.append("")
    lines.append("## Sampler events")
    lines.append("")
    for name, info in report["runs"].items():
        summary = info["summary"]
        if summary is None:
            lines.append(f"- {name}: no summary.json")
            continue
        coredumps = summary["coredumps"]
        lines.append(
            f"- {name}: reason \"{summary['reason']}\"; wall {summary['wall_start']} to {summary['wall_end']}; "
            f"{summary['ticks']} sample ticks; new latte-dock-ng coredumps: {len(coredumps['new'])}"
        )
    lines.append("")
    return "\n".join(lines)


def main(argv: list | None = None) -> int:
    parser = argparse.ArgumentParser(description="Analyze idle memory series written by memsample.py.")
    parser.add_argument("--nested", type=Path, required=True, help="sampler output directory of the nested session")
    parser.add_argument("--desktop", type=Path, help="sampler output directory of the desktop series, if any")
    parser.add_argument("--out", type=Path, required=True, help="directory for analysis.md and analysis.json")
    parser.add_argument("--warmup-min", type=float, default=15.0)
    args = parser.parse_args(sys.argv[1:] if argv is None else argv)

    report = analyze(args)
    args.out.mkdir(parents=True, exist_ok=True)
    (args.out / "analysis.json").write_text(json.dumps(report, indent=1, sort_keys=True) + "\n", encoding="utf-8")
    (args.out / "analysis.md").write_text(render_markdown(report), encoding="utf-8")
    for series, data in report["series"].items():
        print(f"{series}: {data['verdict']['verdict']}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
