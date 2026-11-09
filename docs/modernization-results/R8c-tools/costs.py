#!/usr/bin/env python3
"""Price the R8c gate candidates from the CI-version cost runs.

    python3 docs/modernization-results/R8c-tools/costs.py \\
        --inventory docs/modernization-results/R8c-data/ci-clang-tidy-18.json.gz \\
        --gate docs/modernization-results/R8c-data/ci-cost-gate.json.gz \\
        --checks docs/modernization-results/R8c-data/ci-cost-noanalyzer.json.gz \\
        --analyzer docs/modernization-results/R8c-data/ci-cost-analyzer.json.gz \\
        --candidates docs/modernization-results/R8c-data/ci-gate-candidates.tsv

The three cost runs are tidy_inventory.py reports with --jobs 4: the current gate's
two checks, every non-analyzer check with --check-profile, and the static analyzer
alone. A check's cost is the sum of its profiled wall time over all sources. The base
cost of a source (parsing and the clang-tidy framework) is its time in the gate run
minus the gate's two checks. Wall-time estimates divide CPU seconds by the four jobs
and so assume an even spread over them. Output is Markdown.
"""

from __future__ import annotations

import argparse
import gzip
import json
import statistics
from pathlib import Path

# The defect families whose zero-finding checks are gate candidates. cert is left out:
# most of its checks are aliases of these families, and an alias runs its matchers
# again (cert-dcl37-c and cert-dcl51-cpp each cost as much as bugprone-reserved-identifier).
CANDIDATE_FAMILIES = ("bugprone", "concurrency", "misc", "performance", "portability")
GATE_CHECKS = ("bugprone-unused-return-value", "bugprone-use-after-move")
BANDS = ((10.0, "A"), (50.0, "B"), (float("inf"), "C"))
JOBS = 4


def load(path: Path) -> dict:
    with gzip.open(path, "rt", encoding="utf-8") as handle:
        return json.load(handle)


def family(check: str) -> str:
    return "clang-analyzer" if check.startswith("clang-analyzer-") else check.split("-", 1)[0]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--inventory", required=True, type=Path)
    parser.add_argument("--gate", required=True, type=Path)
    parser.add_argument("--checks", required=True, type=Path)
    parser.add_argument("--analyzer", required=True, type=Path)
    parser.add_argument("--candidates", type=Path, help="write the ranked candidates as TSV")
    args = parser.parse_args()
    inventory, gate, checks, analyzer = (load(p) for p in (args.inventory, args.gate, args.checks, args.analyzer))

    print("## Cost runs (clang-tidy 18.1.3, 4 jobs)\n")
    print("| Run | Checks | Wall (s) | Sum of unit times (s) | Profiled check time (s) | Median / slowest unit (s) |")
    print("|---|---:|---:|---:|---:|---:|")
    for name, document in (("Gate's two checks", gate), ("All non-analyzer checks", checks), ("Static analyzer only", analyzer)):
        units = document["unit_seconds"].values()
        print(
            f"| {name} | {len(document['checks'])} | {document['elapsed_s']:.0f} | {sum(units):.0f} | "
            f"{sum(document['check_wall_s'].values()):.0f} | {statistics.median(units):.1f} / {max(units):.1f} |"
        )

    base = {
        source: seconds - gate["unit_check_wall_s"].get(source, 0.0) for source, seconds in gate["unit_seconds"].items()
    }
    base_total = sum(base.values())
    print(
        f"\nBase cost (parsing and framework): {base_total:.0f} s over {len(base)} sources, median "
        f"{statistics.median(base.values()):.1f} s, slowest {max(base.values()):.1f} s. The gate's two checks add "
        + ", ".join(f"`{c}` {gate['check_wall_s'][c]:.0f} s" for c in GATE_CHECKS)
        + f". The analyzer run's units sum to {sum(analyzer['unit_seconds'].values()):.0f} s, about "
        f"{sum(analyzer['unit_seconds'].values()) - base_total:.0f} s above the base.\n"
    )

    zero = {c for c, v in inventory["per_check"].items() if not v["findings"]}
    profiled = checks["check_wall_s"]
    pool = sorted(
        (c for c in zero if family(c) in CANDIDATE_FAMILIES and c not in GATE_CHECKS),
        key=lambda c: (profiled.get(c, float("inf")), c),
    )
    unpriced = [c for c in pool if c not in profiled]
    priced = [c for c in pool if c in profiled]

    print(f"## Candidates: checks without findings in {', '.join(CANDIDATE_FAMILIES)}\n")
    print(
        f"{len(pool)} checks besides the gate's two; {len(unpriced)} are not in the profile, so their cost is "
        "unknown: " + ", ".join(f"`{c}`" for c in unpriced) + ".\n"
    )
    print("| Band | Cost per check over the tree | Checks | Together (s) | Estimated wall added at 4 jobs (s) |")
    print("|---|---|---:|---:|---:|")
    low = 0.0
    rows = []
    for high, band in BANDS:
        names = [c for c in priced if low < profiled[c] <= high or (low == 0.0 and profiled[c] == 0.0)]
        together = sum(profiled[c] for c in names)
        span = f"up to {high:.0f} s" if high != float("inf") else f"over {low:.0f} s"
        if low and high != float("inf"):
            span = f"{low:.0f} to {high:.0f} s"
        print(f"| {band} | {span} | {len(names)} | {together:.0f} | {together / JOBS:.0f} |")
        rows += [(band, c) for c in names]
        low = high
    stage1 = gate["elapsed_s"]
    print(
        f"\nThe gate's two checks over all sources took {stage1:.0f} s of wall time; adding band A is estimated at "
        f"{(sum(gate['unit_seconds'].values()) + sum(profiled[c] for b, c in rows if b == 'A')) / JOBS:.0f} s in all, "
        f"bands A and B at {(sum(gate['unit_seconds'].values()) + sum(profiled[c] for b, c in rows if b in 'AB')) / JOBS:.0f} s.\n"
    )
    print("Band C, the most expensive:\n")
    print("| Check | Cost (s) |")
    print("|---|---:|")
    for band, check in sorted(rows, key=lambda row: -profiled[row[1]]):
        if band == "C":
            print(f"| `{check}` | {profiled[check]:.0f} |")

    if args.candidates:
        lines = ["check\tfamily\tband\tcost_s"]
        lines += [f"{c}\t{family(c)}\t{b}\t{profiled[c]:.1f}" for b, c in rows]
        lines += [f"{c}\t{family(c)}\tunpriced\tn/a" for c in unpriced]
        args.candidates.write_text("\n".join(lines) + "\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
