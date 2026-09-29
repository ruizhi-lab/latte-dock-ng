#!/usr/bin/env python3
"""Create and compare stable, fail-closed qmllint diagnostic reports."""

from __future__ import annotations

import argparse
import collections
import json
from pathlib import Path
import platform
from pathlib import PurePosixPath
import re
import sys
from typing import Any


REPORT_SCHEMA_VERSION = 1
MANIFEST_SCHEMA_VERSION = 1
JSON_REVISION = 4


class BaselineError(ValueError):
    """Raised when an input cannot prove a complete comparable lint run."""


def _load_json(path: Path, label: str) -> Any:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as error:
        raise BaselineError(f"cannot read valid {label} JSON from {path}: {error}") from error


def _canonical_relative_file(value: Any, source_root: Path) -> str:
    if not isinstance(value, str) or not value:
        raise BaselineError("QML file paths must be non-empty strings")

    candidate = Path(value)
    if not candidate.is_absolute():
        candidate = source_root / candidate
    try:
        relative = candidate.resolve(strict=False).relative_to(source_root.resolve())
    except ValueError as error:
        raise BaselineError(f"QML file is outside the source root: {value}") from error

    normalized = relative.as_posix()
    if normalized in ("", ".") or not normalized.endswith(".qml"):
        raise BaselineError(f"not a source QML file: {value}")
    return normalized


def _canonical_expected_files(values: Any, source_root: Path) -> list[str]:
    if not isinstance(values, list) or not values:
        raise BaselineError("expected QML file list must be a non-empty array")
    files = [_canonical_relative_file(value, source_root) for value in values]
    if len(files) != len(set(files)):
        raise BaselineError("expected QML file list contains duplicates")
    return sorted(files)


def _validate_manifest(manifest: Any, expected_files: list[str], source_root: Path) -> dict[str, Any]:
    if not isinstance(manifest, dict) or manifest.get("schemaVersion") != MANIFEST_SCHEMA_VERSION:
        raise BaselineError("unsupported or missing run manifest schemaVersion")
    if manifest.get("complete") is not True:
        raise BaselineError("run manifest is not marked complete")

    attempted = manifest.get("attemptedFiles")
    if not isinstance(attempted, list):
        raise BaselineError("run manifest has no attemptedFiles array")
    attempted_files = [_canonical_relative_file(path, source_root) for path in attempted]
    if len(attempted_files) != len(set(attempted_files)):
        raise BaselineError("run manifest contains duplicate attempted files")
    if sorted(attempted_files) != expected_files:
        missing = sorted(set(expected_files) - set(attempted_files))
        unexpected = sorted(set(attempted_files) - set(expected_files))
        raise BaselineError(f"incomplete file coverage (missing={missing}, unexpected={unexpected})")

    file_exit_codes = manifest.get("fileExitCodes")
    if not isinstance(file_exit_codes, dict):
        raise BaselineError("run manifest has no per-file exit-code map")
    normalized_codes: dict[str, int] = {}
    for path, code in file_exit_codes.items():
        normalized_path = _canonical_relative_file(path, source_root)
        if normalized_path in normalized_codes:
            raise BaselineError(f"duplicate per-file exit code for {normalized_path}")
        if type(code) is not int:
            raise BaselineError(f"invalid exit code for {normalized_path}")
        normalized_codes[normalized_path] = code
    if sorted(normalized_codes) != expected_files:
        raise BaselineError("per-file exit-code coverage does not match expected files")
    failed_files = {path: code for path, code in normalized_codes.items() if code != 0}
    if failed_files:
        raise BaselineError(f"qmllint reported non-zero file exit codes: {failed_files}")

    process_codes = manifest.get("processExitCodes")
    if not isinstance(process_codes, list) or not process_codes:
        raise BaselineError("run manifest has no processExitCodes")
    if any(type(code) is not int for code in process_codes):
        raise BaselineError("run manifest contains an invalid process exit code")
    if any(code != 0 for code in process_codes):
        raise BaselineError(f"qmllint process failed with exit codes: {process_codes}")

    return {
        "complete": True,
        "attemptedFiles": expected_files,
        "processExitCodes": process_codes,
        "fileExitCodes": {path: 0 for path in expected_files},
    }


def _normalize_message(message: Any) -> str:
    if not isinstance(message, str) or not message.strip():
        raise BaselineError("diagnostic message must be a non-empty string")
    normalized = " ".join(message.split())
    # Normalize absolute paths before substituting caller-known roots so a
    # staging directory's changing suffix cannot leak into the message ID.
    normalized = re.sub(r"(?<![A-Za-z0-9_])/(?:[^\s:'\"<>]+/)*[^\s:'\"<>]*", "<absolute-path>", normalized)
    # Absolute build, temporary staging, and system import prefixes are machine
    # details; the diagnostic's QML file, category, property names and message
    # remain part of the identity.
    return normalized.strip()


def _make_diagnostic(file: Any, category: Any, severity: Any, message: Any,
                     source_root: Path) -> dict[str, str]:
    relative_file = _canonical_relative_file(file, source_root)
    if not isinstance(category, str) or not re.fullmatch(r"[A-Za-z][A-Za-z0-9_.-]*", category):
        raise BaselineError(f"invalid qmllint category for {relative_file}: {category!r}")
    if severity not in ("warning", "info", "error"):
        raise BaselineError(f"invalid diagnostic severity for {relative_file}: {severity!r}")
    return {
        "file": relative_file,
        "category": category,
        "severity": severity,
        "message": _normalize_message(message),
    }


def _aggregate_diagnostics(diagnostics: list[dict[str, str]]) -> list[dict[str, Any]]:
    counts = collections.Counter(
        (item["file"], item["category"], item["severity"], item["message"])
        for item in diagnostics
    )
    return [
        {"file": file, "category": category, "severity": severity, "message": message, "count": count}
        for (file, category, severity, message), count in sorted(counts.items())
    ]


def _validate_qmllint_json(data: Any, expected_files: list[str], source_root: Path,
                           ) -> tuple[list[dict[str, str]], int]:
    if not isinstance(data, dict) or type(data.get("revision")) is not int:
        raise BaselineError("qmllint JSON has no integer revision")
    revision = data["revision"]
    if revision != JSON_REVISION:
        raise BaselineError(f"unsupported qmllint JSON revision {revision}; expected {JSON_REVISION}")
    files = data.get("files")
    if not isinstance(files, list):
        raise BaselineError("qmllint JSON has no files array")

    seen_files: list[str] = []
    diagnostics: list[dict[str, str]] = []
    for file_result in files:
        if not isinstance(file_result, dict) or not isinstance(file_result.get("success"), bool):
            raise BaselineError("qmllint JSON contains a malformed file result")
        relative_file = _canonical_relative_file(file_result.get("filename"), source_root)
        seen_files.append(relative_file)
        warnings = file_result.get("warnings")
        if not isinstance(warnings, list):
            raise BaselineError(f"qmllint JSON has no warnings array for {relative_file}")
        for warning in warnings:
            if not isinstance(warning, dict):
                raise BaselineError(f"malformed qmllint diagnostic for {relative_file}")
            diagnostics.append(_make_diagnostic(
                relative_file, warning.get("id"), warning.get("type"), warning.get("message"),
                source_root,
            ))

    if len(seen_files) != len(set(seen_files)):
        raise BaselineError("qmllint JSON contains duplicate file results")
    if sorted(seen_files) != expected_files:
        missing = sorted(set(expected_files) - set(seen_files))
        unexpected = sorted(set(seen_files) - set(expected_files))
        raise BaselineError(f"qmllint JSON file coverage mismatch (missing={missing}, unexpected={unexpected})")
    return diagnostics, revision


_HUMAN_DIAGNOSTIC = re.compile(
    r"^(?P<severity>Warning|Info|Error):\s+(?P<file>.+?\.qml):"
    r"(?P<line>\d+):(?P<column>\d+):\s+(?P<message>.*?)(?:\s+\[(?P<category>[A-Za-z][A-Za-z0-9_.-]*)\])?$"
)


def _parse_qmllint_text(text: str, expected_files: list[str], source_root: Path,
                        ) -> list[dict[str, str]]:
    if text and not text.endswith("\n"):
        raise BaselineError("qmllint text output is truncated or lacks its final record terminator")
    diagnostics: list[dict[str, str]] = []
    for line_number, line in enumerate(text.splitlines(), 1):
        if not line.startswith(("Warning:", "Info:", "Error:")):
            continue
        match = _HUMAN_DIAGNOSTIC.match(line)
        if not match:
            # qmllint emits explanatory Info lines without a QML location after
            # a primary finding. They are context, while located records must
            # parse completely or the comparison fails closed.
            if line.startswith("Info:") and not re.search(r"\.qml:\d+:\d+:", line):
                continue
            raise BaselineError(f"unrecognized qmllint diagnostic at line {line_number}")
        if not match.group("category"):
            raise BaselineError(f"qmllint diagnostic has no category at line {line_number}")
        diagnostic = _make_diagnostic(
            match.group("file"), match.group("category"), match.group("severity").lower(),
            match.group("message"), source_root,
        )
        if diagnostic["file"] not in expected_files:
            raise BaselineError(f"diagnostic refers to an unattempted file: {diagnostic['file']}")
        diagnostics.append(diagnostic)
    return diagnostics


def build_report(raw: str, input_format: str, manifest: Any, expected_file_values: Any,
                 source_root: Path, tool_version: str, import_roots: list[str],
                 environment: dict[str, str] | None = None) -> dict[str, Any]:
    if input_format not in ("json", "text"):
        raise BaselineError(f"unsupported input format: {input_format}")
    if not isinstance(tool_version, str) or not tool_version.strip():
        raise BaselineError("qmllint tool version is required")
    if not isinstance(import_roots, list) or any(not isinstance(root, str) or not root for root in import_roots):
        raise BaselineError("import roots must be non-empty strings")
    if environment is None:
        environment = {}
    if not isinstance(environment, dict) or any(
        not isinstance(key, str) or not key or not isinstance(value, str) or not value
        for key, value in environment.items()
    ):
        raise BaselineError("environment fingerprint values must be non-empty strings")

    expected_files = _canonical_expected_files(expected_file_values, source_root)
    checked_manifest = _validate_manifest(manifest, expected_files, source_root)
    json_revision: int | None = None
    if input_format == "json":
        try:
            data = json.loads(raw)
        except json.JSONDecodeError as error:
            raise BaselineError(f"malformed or truncated qmllint JSON: {error}") from error
        diagnostics, json_revision = _validate_qmllint_json(data, expected_files, source_root)
    else:
        diagnostics = _parse_qmllint_text(raw, expected_files, source_root)

    return {
        "schemaVersion": REPORT_SCHEMA_VERSION,
        "tool": {"name": "qmllint", "version": tool_version, "format": input_format,
                 "jsonRevision": json_revision},
        "environment": {
            "os": platform.system(),
            "architecture": platform.machine(),
            "attributes": dict(sorted(environment.items())),
            "importRoots": sorted(import_roots),
        },
        "manifest": checked_manifest,
        "diagnostics": _aggregate_diagnostics(diagnostics),
    }


def _validate_report(report: Any, label: str) -> None:
    if not isinstance(report, dict) or report.get("schemaVersion") != REPORT_SCHEMA_VERSION:
        raise BaselineError(f"{label} report has an unsupported schema")
    tool = report.get("tool")
    environment = report.get("environment")
    manifest = report.get("manifest")
    diagnostics = report.get("diagnostics")
    if not isinstance(tool, dict) or tool.get("name") != "qmllint" or not all(
        isinstance(tool.get(key), str) and tool[key] for key in ("version", "format")
    ):
        raise BaselineError(f"{label} report has an invalid tool fingerprint")
    revision = tool.get("jsonRevision")
    if (tool["format"] == "json" and (type(revision) is not int or revision != JSON_REVISION)) or (
        tool["format"] == "text" and revision is not None
    ) or tool["format"] not in ("json", "text"):
        raise BaselineError(f"{label} report has an invalid diagnostic format fingerprint")
    if not isinstance(environment, dict) or not isinstance(environment.get("os"), str) or not environment["os"] or not isinstance(
        environment.get("architecture"), str
    ) or not environment["architecture"] or not isinstance(environment.get("attributes"), dict) or any(
        not isinstance(key, str) or not key or not isinstance(value, str) or not value
        for key, value in environment["attributes"].items()
    ) or not isinstance(environment.get("importRoots"), list) or any(
        not isinstance(root, str) or not root for root in environment["importRoots"]
    ) or environment["importRoots"] != sorted(set(environment["importRoots"])):
        raise BaselineError(f"{label} report has an invalid environment fingerprint")
    if not isinstance(manifest, dict) or manifest.get("complete") is not True:
        raise BaselineError(f"{label} report is incomplete")
    if not isinstance(manifest.get("attemptedFiles"), list) or not manifest["attemptedFiles"]:
        raise BaselineError(f"{label} report has no file coverage")
    attempted = manifest["attemptedFiles"]
    if any(
        not isinstance(path, str) or not path.endswith(".qml") or path.startswith("/")
        or any(part in ("", ".", "..") for part in PurePosixPath(path).parts)
        for path in attempted
    ):
        raise BaselineError(f"{label} report has invalid source-relative QML paths")
    if attempted != sorted(set(attempted)):
        raise BaselineError(f"{label} report file coverage is duplicate or unordered")
    file_codes = manifest.get("fileExitCodes")
    if not isinstance(file_codes, dict) or set(file_codes) != set(attempted) or any(
        type(code) is not int or code != 0 for code in file_codes.values()
    ):
        raise BaselineError(f"{label} report does not prove successful per-file execution")
    process_codes = manifest.get("processExitCodes")
    if not isinstance(process_codes, list) or not process_codes or any(
        type(code) is not int or code != 0 for code in process_codes
    ):
        raise BaselineError(f"{label} report does not prove successful qmllint processes")
    if not isinstance(diagnostics, list):
        raise BaselineError(f"{label} report has no diagnostics array")
    identities = set()
    for item in diagnostics:
        if not isinstance(item, dict) or not all(isinstance(item.get(key), str) and item[key]
                                                 for key in ("file", "category", "severity", "message")):
            raise BaselineError(f"{label} report contains a malformed diagnostic")
        if type(item.get("count")) is not int or item["count"] < 1:
            raise BaselineError(f"{label} report contains an invalid diagnostic multiplicity")
        if item["file"] not in attempted or item["severity"] not in ("warning", "info", "error"):
            raise BaselineError(f"{label} report contains a diagnostic outside successful coverage")
        identity = (item["file"], item["category"], item["severity"], item["message"])
        if identity in identities:
            raise BaselineError(f"{label} report contains duplicate identity rows")
        identities.add(identity)


def compare_reports(baseline: Any, current: Any) -> tuple[list[dict[str, Any]], list[dict[str, Any]]]:
    _validate_report(baseline, "baseline")
    _validate_report(current, "current")
    for key in ("tool", "environment"):
        if baseline[key] != current[key]:
            raise BaselineError(f"{key} fingerprint changed; establish a reviewed comparable baseline")
    if baseline["manifest"]["attemptedFiles"] != current["manifest"]["attemptedFiles"]:
        raise BaselineError("QML file coverage changed; baseline comparison is not comparable")

    def counts(report: dict[str, Any]) -> collections.Counter[tuple[str, str, str, str]]:
        return collections.Counter({
            (item["file"], item["category"], item["severity"], item["message"]): item["count"]
            for item in report["diagnostics"]
        })

    before = counts(baseline)
    after = counts(current)
    added = [
        {"file": file, "category": category, "severity": severity, "message": message,
         "count": count - before[(file, category, severity, message)]}
        for (file, category, severity, message), count in sorted(after.items())
        if count > before[(file, category, severity, message)]
    ]
    removed = [
        {"file": file, "category": category, "severity": severity, "message": message,
         "count": count - after[(file, category, severity, message)]}
        for (file, category, severity, message), count in sorted(before.items())
        if count > after[(file, category, severity, message)]
    ]
    return added, removed


def _read_environment(values: list[str]) -> dict[str, str]:
    environment: dict[str, str] = {}
    for value in values:
        if "=" not in value:
            raise BaselineError(f"environment fingerprint must have NAME=VALUE form: {value}")
        name, content = value.split("=", 1)
        if not re.fullmatch(r"[A-Za-z][A-Za-z0-9_.-]*", name) or not content:
            raise BaselineError(f"invalid environment fingerprint: {value}")
        if name in environment:
            raise BaselineError(f"duplicate environment fingerprint: {name}")
        environment[name] = content
    return environment


def _capture(args: argparse.Namespace) -> int:
    source_root = Path(args.source_root).resolve()
    try:
        raw = Path(args.input).read_text(encoding="utf-8")
        manifest = _load_json(Path(args.manifest), "run manifest")
        expected = _load_json(Path(args.expected_files), "expected-file")
        report = build_report(
            raw, args.format, manifest, expected, source_root, args.tool_version,
            args.import_root, _read_environment(args.environment),
        )
        destination = Path(args.output)
        destination.parent.mkdir(parents=True, exist_ok=True)
        temporary = destination.with_suffix(destination.suffix + ".tmp")
        temporary.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        temporary.replace(destination)
        print(f"qmllint-baseline: wrote {destination} ({len(report['diagnostics'])} identities)")
        return 0
    except (BaselineError, OSError, UnicodeError) as error:
        print(f"qmllint-baseline: error: {error}", file=sys.stderr)
        return 2


def _compare(args: argparse.Namespace) -> int:
    try:
        baseline = _load_json(Path(args.baseline), "baseline")
        current = _load_json(Path(args.current), "current")
        added, removed = compare_reports(baseline, current)
        result = {"added": added, "removed": removed}
        if args.output:
            Path(args.output).write_text(json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        print(json.dumps(result, indent=2, sort_keys=True))
        return 1 if added else 0
    except (BaselineError, OSError, UnicodeError) as error:
        print(f"qmllint-baseline: error: {error}", file=sys.stderr)
        return 2


def _argument_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    capture = commands.add_parser("capture", help="normalize one complete qmllint run")
    capture.add_argument("--input", required=True, help="raw qmllint output")
    capture.add_argument("--format", choices=("json", "text"), required=True)
    capture.add_argument("--manifest", required=True, help="run manifest with attempted files and exit codes")
    capture.add_argument("--expected-files", required=True, help="JSON array of all expected source-relative QML paths")
    capture.add_argument("--source-root", required=True)
    capture.add_argument("--tool-version", required=True)
    capture.add_argument("--import-root", action="append", default=[])
    capture.add_argument("--environment", action="append", default=[], metavar="NAME=VALUE")
    capture.add_argument("--output", required=True)
    capture.set_defaults(handler=_capture)

    compare = commands.add_parser("compare", help="compare a run with a reviewed baseline")
    compare.add_argument("--baseline", required=True)
    compare.add_argument("--current", required=True)
    compare.add_argument("--output", help="optional additions/removals JSON report")
    compare.set_defaults(handler=_compare)
    return parser


def main(argv: list[str] | None = None) -> int:
    args = _argument_parser().parse_args(argv)
    return args.handler(args)


if __name__ == "__main__":
    raise SystemExit(main())
