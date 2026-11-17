# T4k host evidence

Host: CMake 4.3.4, GCC 16.2.1, Clang 22.1.8. The application/autotest build
logs are compressed because they include the existing build-aware QML lint output.
Both Debug application/autotest builds completed with zero source-located C++
warning/error diagnostics. Build-aware QML diagnostics are retained in the
logs and are not compiler warning counts.

| Preset | Configure | Build | CTest |
| --- | --- | --- | --- |
| `gcc-debug` | passed | passed, `--parallel 8` | 64/64 passed, 36.94 s |
| `clang-debug` | passed | passed, `--parallel 8` | 64/64 passed, 36.54 s |

CTest ran as `dbus-run-session -- ctest --test-dir
build/modernization/<preset> --output-on-failure` after the first GCC attempt
without a private bus stalled at `qmlsmoketest`. The sandboxed `dbus-run-session`
retry exited 127 because the sandbox denied binding its bus socket. Retrying
with approved sandbox escalation ran the private session bus and passed. The
initially interrupted run and sandbox denial are recorded in
[`ctest-attempts.txt`](ctest-attempts.txt); final CTest logs are in this
directory.

The current host has no CMake 4.5 executable or cached package. Host checks
therefore cover only the below-4.5 branch of the version guard.
