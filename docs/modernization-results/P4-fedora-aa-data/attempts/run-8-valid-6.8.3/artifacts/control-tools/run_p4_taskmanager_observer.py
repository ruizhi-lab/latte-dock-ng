#!/usr/bin/env python3
"""Run the isolated P4 TaskManager observer and retain monotonic JSONL events."""

from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import re
import selectors
import subprocess
import sys
import time


HERE = Path(__file__).resolve().parent
OBSERVER = HERE / "p4-taskmanager-observer.qml"
EVENT_RE = re.compile(r"(?:qml: )?P4_EVENT (\{.*\})")
REQUIRED_EVENTS = (
    "baseline-row",
    "controller-activate",
    "observed-active",
    "controller-minimize",
    "observed-minimized",
    "controller-restore",
    "controller-reactivate",
    "observed-restored-active",
)


def validate_sequence(events: list[dict], app_id: str, target_title: str) -> tuple[bool, str]:
    records = [event for event in events if event.get("event") != "result"]
    results = [event for event in events if event.get("event") == "result"]
    if len(results) != 1 or results[0].get("verdict") != "valid-sequence":
        return False, "missing-or-invalid-final-verdict"
    readiness = next((event for event in records if event.get("event") == "readiness"), None)
    if readiness is None or any(readiness.get(key) is not True
                                for key in ("activityReady", "desktopReady", "screenReady")):
        return False, "missing-valid-readiness-event"

    names = [str(event.get("event", "")) for event in records]
    positions = []
    cursor = 0
    for name in REQUIRED_EVENTS:
        try:
            position = names.index(name, cursor)
        except ValueError:
            return False, f"missing-stage-{name}"
        positions.append(position)
        cursor = position + 1

    baseline = records[positions[0]]
    window_id = str(baseline.get("windowId", ""))
    if (not window_id or baseline.get("appId") != app_id
            or baseline.get("title") != target_title or baseline.get("isWindow") is not True
            or baseline.get("isStartup") is not False):
        return False, "invalid-target-baseline"

    active = records[positions[2]]
    minimized = records[positions[4]]
    restored = records[positions[7]]
    for position in positions[1:]:
        event = records[position]
        if (str(event.get("windowId", "")) != window_id
                or event.get("appId", app_id) != app_id
                or event.get("title", target_title) != target_title):
            return False, "window-identity-changed"
    for event in (active, minimized, restored):
        if event.get("isWindow") is not True or event.get("isStartup") is not False:
            return False, "target-row-not-a-window"
    if active.get("isActive") is not True or active.get("isMinimized") is not False:
        return False, "active-stage-not-observed"
    if minimized.get("isMinimized") is not True:
        return False, "minimized-stage-not-observed"
    if restored.get("isActive") is not True or restored.get("isMinimized") is not False:
        return False, "restore-stage-not-observed"
    if str(results[0].get("windowId", "")) != window_id:
        return False, "final-identity-changed"
    return True, "valid-sequence"


def consume_process(proc: subprocess.Popen, deadline: float, emit) -> tuple[int, bool]:
    """Drain a child without blocking past a monotonic deadline."""
    assert proc.stdout is not None
    selector = selectors.DefaultSelector()
    selector.register(proc.stdout, selectors.EVENT_READ)
    pending = bytearray()
    timed_out = False
    try:
        while selector.get_map():
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                timed_out = True
                proc.kill()
                break
            ready = selector.select(min(remaining, 0.25))
            if not ready:
                if proc.poll() is not None:
                    continue
                continue
            chunk = os.read(proc.stdout.fileno(), 4096)
            if not chunk:
                selector.unregister(proc.stdout)
                break
            pending.extend(chunk)
            while b"\n" in pending:
                line, _, rest = pending.partition(b"\n")
                pending = bytearray(rest)
                emit(line.decode("utf-8", errors="replace") + "\n")
        if pending:
            emit(pending.decode("utf-8", errors="replace"))
    finally:
        selector.close()
        proc.stdout.close()
    return proc.wait(timeout=2), timed_out


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--app-id", required=True)
    parser.add_argument("--target-title", required=True)
    parser.add_argument("--qml", default="qml6",
                        help="Qt QML launcher executable (default: qml6)")
    parser.add_argument("--timeout-ms", type=int, default=20000)
    parser.add_argument("--stage-timeout-ms", type=int, default=5000)
    parser.add_argument("--log", type=Path)
    args = parser.parse_args()

    if os.environ.get("P4_PRIVATE_SESSION") != "1":
        print("Refusing to control a TaskManager window outside the marked private test session.", file=sys.stderr)
        return 64
    if args.timeout_ms < 1000 or args.stage_timeout_ms < 500:
        parser.error("timeouts are too short for bounded event observation")

    argv = [
        args.qml,
        str(OBSERVER),
        "--",
        "--app-id",
        args.app_id,
        "--target-title",
        args.target_title,
        "--timeout-ms",
        str(args.timeout_ms),
        "--stage-timeout-ms",
        str(args.stage_timeout_ms),
    ]
    child_env = os.environ.copy()
    # User units route Qt messages to journald; the event parser consumes this child pipe.
    child_env["QT_FORCE_STDERR_LOGGING"] = "1"
    try:
        proc = subprocess.Popen(argv, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                env=child_env)
    except OSError as error:
        print(f"Unable to start qml6: {error}", file=sys.stderr)
        return 69

    log_stream = None
    try:
        if args.log:
            args.log.parent.mkdir(parents=True, exist_ok=True)
            log_stream = args.log.open("w", encoding="utf-8")
        else:
            log_stream = sys.stdout
        events: list[dict] = []
        start_ns = time.monotonic_ns()

        def record(line: str) -> None:
            match = EVENT_RE.search(line)
            if match:
                try:
                    event = json.loads(match.group(1))
                except json.JSONDecodeError:
                    event = {"event": "malformed", "raw": line.rstrip()}
                events.append(event)
                row = {"monotonic_ns": time.monotonic_ns() - start_ns, "qml": event}
                log_stream.write(json.dumps(row, sort_keys=True) + "\n")
            else:
                row = {"monotonic_ns": time.monotonic_ns() - start_ns,
                       "raw": line.rstrip("\n")}
                log_stream.write(json.dumps(row, sort_keys=True) + "\n")
            log_stream.flush()

        deadline = time.monotonic() + (args.timeout_ms + 2000) / 1000
        return_code, timed_out = consume_process(proc, deadline, record)
        if timed_out:
            record("P4_RUNNER_TIMEOUT: child exceeded the monotonic hard deadline.\n")
    except (OSError, subprocess.TimeoutExpired) as error:
        if proc.poll() is None:
            proc.kill()
            proc.wait()
        print(f"P4 observer runner failed: {error}", file=sys.stderr)
        return 70
    finally:
        if args.log and log_stream is not None:
            log_stream.close()

    if timed_out:
        print("P4_RUNNER_TIMEOUT: child exceeded the monotonic hard deadline.", file=sys.stderr)
        return 70
    if return_code != 0:
        return 71

    result = next((event for event in reversed(events) if event.get("event") == "result"), None)
    if result is None:
        print("Observer exited without a final verdict; no performance sample is valid.", file=sys.stderr)
        return 72
    verdict = str(result.get("verdict", ""))
    if verdict == "valid-sequence":
        valid, reason = validate_sequence(events, args.app_id, args.target_title)
        if valid:
            return 0
        print(f"Observer sequence rejected: {reason}; no performance sample is valid.", file=sys.stderr)
        return 72
    if verdict == "invalid-readiness":
        return 20
    if verdict == "model-or-driver-failure":
        return 21
    print(f"Unknown observer verdict {verdict!r}; no performance sample is valid.", file=sys.stderr)
    return 72


if __name__ == "__main__":
    raise SystemExit(main())
