#!/usr/bin/env python3
"""Build R8a-inventory.md from the list_contracts.py table and classes.tsv.

Usage:
    python3 gen_inventory.py contracts-before.md classes.tsv ../R8a-inventory.md

classes.tsv has one line per contract function: index|name|class|reason.
The script checks that the names follow the table order, that every class is
one of K, B1, B2, B3, D or S, and that each reason is at most 15 words, then
writes the class counts and the rows. Standard library only.
"""

import re
import sys
import collections

table_path, classes_path, out_path = sys.argv[1:4]
CLASSES = {
    "K": "keep: protects text that no behavioural test can reach",
    "B1": "behavioural candidate: class constructible in a test, has a target",
    "B2": "behavioural candidate that needs a fixture or fake first",
    "B3": "needs a Corona, a display or a live session; not testable here",
    "D": "duplicate of another contract or an existing behavioural test",
    "S": "stale: pins an implementation detail that protects no failure mode",
}
rows = []
for line in open(table_path, encoding="utf-8"):
    if not re.match(r"^\| \d+ \|", line):
        continue
    cells = [c.strip() for c in line.strip().strip("|").split(" | ")]
    rows.append({"index": int(cells[0]), "name": cells[1].strip("`"),
                 "first": cells[2], "last": cells[3]})
classes = {}
for line in open(classes_path, encoding="utf-8"):
    line = line.rstrip("\n")
    if not line:
        continue
    index, name, cls, reason = line.split("|", 3)
    classes[int(index)] = (name, cls, reason)
assert len(rows) == 229, len(rows)
assert sorted(classes) == list(range(1, 230)), "index gap"
problems = []
for row in rows:
    name, cls, reason = classes[row["index"]][0], classes[row["index"]][1], classes[row["index"]][2]
    if name != row["name"]:
        problems.append(f"name mismatch at {row['index']}: {name} != {row['name']}")
    if cls not in CLASSES:
        problems.append(f"bad class {cls} at {row['index']}")
    if len(reason.split()) > 15:
        problems.append(f"reason over 15 words at {row['index']}: {reason}")
    row["class"], row["reason"] = cls, reason
if problems:
    print("\n".join(problems), file=sys.stderr)
    sys.exit(1)
counts = collections.Counter(row["class"] for row in rows)
out = []
out.append("# R8a source-contract inventory")
out.append("")
out.append("Scope: the 229 test functions of `autotests/sourcecontracttest.cpp` as they stood at 6bdd43086, before any R8a conversion. "
           "Line ranges and source paths come from [contracts-before.md](R8a-tools/contracts-before.md), the output of "
           "[list_contracts.py](R8a-tools/list_contracts.py) on that file. Each row gives one class and a reason of at most 15 words. "
           "The reason for a converted row says what replaced it; the conversions and their mutation proofs are in [R8a.md](R8a.md).")
out.append("")
out.append("## Class counts")
out.append("")
out.append("| Class | Meaning | Count |")
out.append("|---|---|---|")
for cls in ["K", "B1", "B2", "B3", "D", "S"]:
    out.append(f"| {cls} | {CLASSES[cls]} | {counts.get(cls, 0)} |")
out.append(f"| total | | {sum(counts.values())} |")
out.append("")
out.append("## Rows")
out.append("")
out.append("| # | Function | Lines | Class | Reason |")
out.append("|---|---|---|---|---|")
for row in rows:
    reason = row["reason"].replace("|", "\\|")
    out.append(f"| {row['index']} | `{row['name']}` | {row['first']}-{row['last']} | {row['class']} | {reason} |")
open(out_path, "w", encoding="utf-8").write("\n".join(out) + "\n")
print(dict(sorted(counts.items())), sum(counts.values()))
