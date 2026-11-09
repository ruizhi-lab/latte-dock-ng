#!/usr/bin/env python3
"""Print the tables of the R8c record from the committed inventories.

    python3 docs/modernization-results/R8c-tools/summarize.py \\
        docs/modernization-results/R8c-data/ci-clang-tidy-18.json.gz \\
        docs/modernization-results/R8c-data/host-clang-tidy-22.json.gz

The first inventory is the reference (the clang-tidy that CI runs); the others are
compared with it. Output is Markdown.
"""

from __future__ import annotations

import argparse
import gzip
import json
import statistics
from collections import Counter, defaultdict
from pathlib import Path

# Families whose checks look for defects rather than style; the order of the record.
BUG_FAMILIES = ("bugprone", "cert", "clang-analyzer", "concurrency", "misc", "performance", "portability")
STYLE_FAMILIES = ("cppcoreguidelines", "modernize", "readability")
# The checks of the current gate (.clang-tidy), which analyzes app/data/errordata.cpp only.
GATE_CHECKS = ("bugprone-unused-return-value", "bugprone-use-after-move")
FEW = 10


def family(check: str) -> str:
    return "clang-analyzer" if check.startswith("clang-analyzer-") else check.split("-", 1)[0]


def load(path: Path) -> dict:
    opener = gzip.open if path.suffix == ".gz" else open
    with opener(path, "rt", encoding="utf-8") as handle:
        return json.load(handle)


def label(document: dict) -> str:
    words = document["clang_tidy"].split()
    version = words[words.index("version") + 1] if "version" in words else document["clang_tidy"]
    return f"clang-tidy {version}"


def alias_groups(document: dict) -> Counter:
    """Names that reported the same diagnostic (location and text) together."""
    names = defaultdict(set)
    for item in document["findings"]:
        names[(item["file"], item["line"], item["column"], item["message"])].add(item["check"])
    return Counter(tuple(sorted(group)) for group in names.values() if len(group) > 1)


def overview(documents: list[dict]) -> None:
    print("| | " + " | ".join(label(d) for d in documents) + " |")
    print("|---|" + "---:|" * len(documents))
    rows = [
        ("Checks enabled", lambda d: len(d["checks"])),
        ("Production sources", lambda d: len(d["sources"])),
        ("Sources that failed", lambda d: len(d["failed_sources"])),
        ("Findings (first-party, unique)", lambda d: len(d["findings"])),
        ("Checks with findings", lambda d: sum(1 for v in d["per_check"].values() if v["findings"])),
        ("Checks without findings", lambda d: sum(1 for v in d["per_check"].values() if not v["findings"])),
        ("Not counted (generated or test code)", lambda d: d["ignored_findings"]),
        ("Wall time (s) / jobs", lambda d: f"{d['elapsed_s']:.0f} / {d['jobs']}"),
        ("Sum of unit times (s)", lambda d: f"{sum(d['unit_seconds'].values()):.0f}"),
        ("Median / slowest unit (s)", lambda d: f"{statistics.median(d['unit_seconds'].values()):.1f} / {max(d['unit_seconds'].values()):.1f}"),
    ]
    for name, value in rows:
        print(f"| {name} | " + " | ".join(str(value(d)) for d in documents) + " |")


def families(documents: list[dict]) -> None:
    print("| Family | " + " | ".join(f"{label(d)}: findings / checks with / without" for d in documents) + " |")
    print("|---|" + "---:|" * len(documents))
    for name in BUG_FAMILIES + STYLE_FAMILIES:
        cells = []
        for document in documents:
            values = [v for c, v in document["per_check"].items() if family(c) == name]
            cells.append(
                f"{sum(v['findings'] for v in values)} / {sum(1 for v in values if v['findings'])} / "
                f"{sum(1 for v in values if not v['findings'])}"
            )
        print(f"| {name} | " + " | ".join(cells) + " |")


def ranked(document: dict, names: tuple[str, ...], low: int, high: int | None) -> list[tuple[str, dict]]:
    selected = [
        (check, value)
        for check, value in document["per_check"].items()
        if family(check) in names and value["findings"] >= low and (high is None or value["findings"] <= high)
    ]
    return sorted(selected, key=lambda pair: (-pair[1]["findings"], pair[0]))


def check_table(rows: list[tuple[str, dict]], others: list[dict]) -> None:
    print("| Check | Findings | Files |" + "".join(f" {label(d)} |" for d in others))
    print("|---|---:|---:|" + "---:|" * len(others))
    for check, value in rows:
        cells = "".join(f" {d['per_check'].get(check, {}).get('findings', 'n/a')} |" for d in others)
        print(f"| `{check}` | {value['findings']} | {value['files']} |" + cells)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("inventories", nargs="+", type=Path, help="tidy_inventory.py reports; the first is the reference")
    args = parser.parse_args()
    documents = [load(path) for path in args.inventories]
    reference, others = documents[0], documents[1:]

    print("## Overview\n")
    overview(documents)
    print("\n## Families\n")
    families(documents)

    print(f"\n## The gate's checks over all production sources ({label(reference)})\n")
    check_table([(c, reference["per_check"][c]) for c in GATE_CHECKS], others)

    zero = defaultdict(list)
    for check, value in reference["per_check"].items():
        if not value["findings"]:
            zero[family(check)].append(check)
    print(f"\n## Checks without findings ({label(reference)})\n")
    extra = f" Of them also without findings in {', '.join(label(d) for d in others)} |" if others else ""
    print("| Family | Checks |" + extra)
    print("|---|---:|" + ("---:|" if others else ""))
    for name in BUG_FAMILIES + STYLE_FAMILIES:
        both = [c for c in zero[name] if all(d["per_check"].get(c, {}).get("findings", 0) == 0 for d in others)]
        print(f"| {name} | {len(zero[name])} |" + (f" {len(both)} |" if others else ""))

    print(f"\n## Defect families: checks with 1 to {FEW} findings ({label(reference)})\n")
    check_table(ranked(reference, BUG_FAMILIES, 1, FEW), others)
    print(f"\n## Defect families: checks with more than {FEW} findings ({label(reference)})\n")
    check_table(ranked(reference, BUG_FAMILIES, FEW + 1, None), others)
    print(f"\n## Style families: the 15 largest ({label(reference)})\n")
    check_table(ranked(reference, STYLE_FAMILIES, 1, None)[:15], others)

    print(f"\n## Names that report the same diagnostic ({label(reference)})\n")
    print("| Names | Diagnostics |")
    print("|---|---:|")
    for names, count in alias_groups(reference).most_common():
        print("| " + " + ".join(f"`{n}`" for n in names) + f" | {count} |")

    for other in others:
        only = sorted(set(other["checks"]) - set(reference["checks"]))
        gone = sorted(set(reference["checks"]) - set(other["checks"]))
        found = [c for c in only if other["per_check"][c]["findings"]]
        print(f"\n## {label(other)} against {label(reference)}\n")
        print(f"- Checks only in {label(other)}: {len(only)}, {len(found)} of them with findings"
              + (": " + ", ".join(f"`{c}` {other['per_check'][c]['findings']}" for c in found) if found else "") + ".")
        print(f"- Checks only in {label(reference)}: {len(gone)}" + (": " + ", ".join(f"`{c}`" for c in gone) if gone else "") + ".")
        changed = [
            (c, reference["per_check"][c]["findings"], other["per_check"][c]["findings"])
            for c in sorted(set(reference["checks"]) & set(other["checks"]))
            if reference["per_check"][c]["findings"] != other["per_check"][c]["findings"]
        ]
        print(f"- Checks in both whose count differs: {len(changed)}.\n")
        if changed:
            print(f"| Check | {label(reference)} | {label(other)} |")
            print("|---|---:|---:|")
            for check, before, after in sorted(changed, key=lambda row: (-abs(row[2] - row[1]), row[0])):
                print(f"| `{check}` | {before} | {after} |")

    print(f"\n## Slowest units ({label(reference)})\n")
    print("| Source | Seconds |")
    print("|---|---:|")
    for source, seconds in sorted(reference["unit_seconds"].items(), key=lambda pair: -pair[1])[:8]:
        print(f"| `{source}` | {seconds:.1f} |")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
