#!/usr/bin/env python3
"""Compare the latte-dock-ng compile units of two compile_commands.json files.

Each unit is keyed by its object file (the CMake "output" field), which is the
same in two trees configured from the same sources. Both databases are first
normalized: the build root becomes <BUILD> and the source root becomes <SRC>,
matched on whole path components, so a database generated in one checkout can
be compared with one generated in another. Units are kept when their object
path contains latte-dock-ng.dir or latte-dock-ng_autogen; --all-units keeps
every unit. Output and dependency-file arguments and the source file itself are
not flags, so they are left out of the comparison.

By default the order of a unit's flags is ignored: units whose flags only differ
in order are listed as notes and do not fail the comparison. Include order can
change which header is found, so --strict-order turns those notes into
differences.

Usage:
    python3 -I compare_compile_commands.py BASELINE.json CANDIDATE.json \\
        --baseline-source-root DIR --baseline-build-root DIR \\
        --candidate-source-root DIR --candidate-build-root DIR \\
        [--all-units] [--strict-order]

Exit status: 0 when the compared units are identical, 1 when they differ, 2 when
a database cannot be read or is malformed.
"""

from __future__ import annotations

import argparse
import json
import os
import re
import shlex
import sys
from collections import Counter
from dataclasses import dataclass
from pathlib import Path
from typing import Sequence

OBJECT_MARKERS = ("latte-dock-ng.dir", "latte-dock-ng_autogen")
ARGUMENT_OPTIONS = frozenset({"-o", "-MF", "-MT", "-MQ"})


class InputError(Exception):
    """A compile database is unreadable, malformed or has duplicate outputs."""


@dataclass(frozen=True)
class Unit:
    """One translation unit after normalization."""

    source: str
    flags: tuple[str, ...]


class Normalizer:
    """Replaces the build and source roots with placeholders.

    The build root is replaced first because it normally lies below the source
    root. A root only matches up to a path-component boundary, so /x/tree never
    rewrites /x/tree-other.
    """

    def __init__(self, source_root: str, build_root: str) -> None:
        self._rules: list[tuple[re.Pattern[str], str]] = []
        for root, placeholder in ((build_root, "<BUILD>"), (source_root, "<SRC>")):
            root = root.rstrip("/")
            if root:
                # Compile databases escape quotes, so a root may end a token or
                # sit before a quote (-DLATTE_SOURCE_DIR="/tree"); only a
                # continuing path-name character must block the match.
                pattern = re.compile(re.escape(root) + r"(?![\w.+-])")
                self._rules.append((pattern, placeholder))

    def __call__(self, text: str) -> str:
        for pattern, placeholder in self._rules:
            text = pattern.sub(placeholder, text)
        return text


def _argument_after(tokens: Sequence[str], option: str) -> str | None:
    """Return the token following the first occurrence of option, if any."""
    for position, token in enumerate(tokens[:-1]):
        if token == option:
            return tokens[position + 1]
    return None


def _flags(
    tokens: Sequence[str], file_name: str, source: str, normalize: Normalizer
) -> tuple[str, ...]:
    """Return the normalized compile flags, without outputs and the source file."""
    flags: list[str] = []
    skip_argument = False
    for token in tokens:
        if skip_argument:
            skip_argument = False
            continue
        if token in ARGUMENT_OPTIONS:
            skip_argument = True
            continue
        if token in (file_name, source):
            continue
        flags.append(normalize(token))
    return tuple(flags)


def load_units(
    path: Path, normalize: Normalizer, *, all_units: bool = False
) -> dict[str, Unit]:
    """Return the selected units of one database, keyed by normalized object path."""
    try:
        entries = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeDecodeError, json.JSONDecodeError) as error:
        raise InputError(f"{path}: cannot read compile database: {error}") from error
    if not isinstance(entries, list):
        raise InputError(f"{path}: expected a JSON array of compile entries")

    units: dict[str, Unit] = {}
    for index, entry in enumerate(entries):
        where = f"{path}: entry {index}"
        if not isinstance(entry, dict):
            raise InputError(f"{where} is not a JSON object")
        try:
            directory = str(entry["directory"])
            file_name = str(entry["file"])
            if "arguments" in entry:
                tokens = [str(token) for token in entry["arguments"]]
            else:
                tokens = shlex.split(str(entry["command"]))
        except (KeyError, ValueError) as error:
            raise InputError(f"{where} is malformed: {error!r}") from error
        output = entry.get("output") or _argument_after(tokens, "-o")
        if not output:
            raise InputError(f"{where} has no output path")
        object_path = normalize(str(output))
        if not all_units and not any(m in object_path for m in OBJECT_MARKERS):
            continue
        source = os.path.normpath(os.path.join(directory, file_name))
        if object_path in units:
            raise InputError(f"{where}: duplicate output {object_path}")
        units[object_path] = Unit(
            source=normalize(source),
            flags=_flags(tokens, file_name, source, normalize),
        )
    return units


@dataclass(frozen=True)
class Comparison:
    """The differences between two sets of normalized units."""

    baseline_units: int
    candidate_units: int
    only_baseline: tuple[str, ...]
    only_candidate: tuple[str, ...]
    source_changes: tuple[tuple[str, str, str], ...]
    flag_changes: tuple[tuple[str, tuple[str, ...], tuple[str, ...]], ...]
    order_only: tuple[str, ...]

    @property
    def identical(self) -> bool:
        return not (
            self.only_baseline
            or self.only_candidate
            or self.source_changes
            or self.flag_changes
        )


def compare(
    baseline: dict[str, Unit], candidate: dict[str, Unit], *, strict_order: bool = False
) -> Comparison:
    """Compare two unit sets keyed by object path; entry order never matters."""
    common = sorted(baseline.keys() & candidate.keys())
    source_changes: list[tuple[str, str, str]] = []
    flag_changes: list[tuple[str, tuple[str, ...], tuple[str, ...]]] = []
    order_only: list[str] = []
    for key in common:
        left, right = baseline[key], candidate[key]
        if left.source != right.source:
            source_changes.append((key, left.source, right.source))
        if Counter(left.flags) != Counter(right.flags):
            flag_changes.append((key, left.flags, right.flags))
        elif left.flags != right.flags:
            order_only.append(key)
            if strict_order:
                flag_changes.append((key, left.flags, right.flags))
    return Comparison(
        baseline_units=len(baseline),
        candidate_units=len(candidate),
        only_baseline=tuple(sorted(baseline.keys() - candidate.keys())),
        only_candidate=tuple(sorted(candidate.keys() - baseline.keys())),
        source_changes=tuple(source_changes),
        flag_changes=tuple(sorted(flag_changes, key=lambda change: change[0])),
        order_only=tuple(order_only),
    )


def render(comparison: Comparison) -> str:
    """Return a deterministic text report."""
    common = comparison.baseline_units - len(comparison.only_baseline)
    lines = [
        f"baseline units:  {comparison.baseline_units}",
        f"candidate units: {comparison.candidate_units}",
        f"common units:    {common}",
        f"only in baseline:   {len(comparison.only_baseline)}",
        f"only in candidate:  {len(comparison.only_candidate)}",
        f"source changes:     {len(comparison.source_changes)}",
        f"flag changes:       {len(comparison.flag_changes)}",
        f"order-only notes:   {len(comparison.order_only)}",
    ]
    for key in comparison.only_baseline:
        lines.append(f"- only in baseline: {key}")
    for key in comparison.only_candidate:
        lines.append(f"+ only in candidate: {key}")
    for key, left, right in comparison.source_changes:
        lines.append(f"~ source of {key}: {left} -> {right}")
    for key, left, right in comparison.flag_changes:
        lines.append(f"~ flags of {key}:")
        removed = Counter(left) - Counter(right)
        added = Counter(right) - Counter(left)
        for token in sorted(removed.elements()):
            lines.append(f"    - {token}")
        for token in sorted(added.elements()):
            lines.append(f"    + {token}")
        if not removed and not added:
            lines.append("    (same flags, different order)")
    for key in comparison.order_only:
        if comparison.identical:
            lines.append(f"note: flag order differs only: {key}")
    lines.append("result: " + ("identical" if comparison.identical else "different"))
    return "\n".join(lines)


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n", 1)[0])
    parser.add_argument("baseline", type=Path, help="compile_commands.json of the reference build")
    parser.add_argument("candidate", type=Path, help="compile_commands.json to check")
    parser.add_argument("--baseline-source-root", required=True)
    parser.add_argument("--baseline-build-root", required=True)
    parser.add_argument("--candidate-source-root", required=True)
    parser.add_argument("--candidate-build-root", required=True)
    parser.add_argument("--all-units", action="store_true", help="compare every unit, not only latte-dock-ng ones")
    parser.add_argument("--strict-order", action="store_true", help="treat flag order differences as changes")
    args = parser.parse_args(argv)
    try:
        baseline = load_units(
            args.baseline,
            Normalizer(args.baseline_source_root, args.baseline_build_root),
            all_units=args.all_units,
        )
        candidate = load_units(
            args.candidate,
            Normalizer(args.candidate_source_root, args.candidate_build_root),
            all_units=args.all_units,
        )
    except InputError as error:
        print(f"error: {error}", file=sys.stderr)
        return 2
    comparison = compare(baseline, candidate, strict_order=args.strict_order)
    print(render(comparison))
    return 0 if comparison.identical else 1


if __name__ == "__main__":
    sys.exit(main())
