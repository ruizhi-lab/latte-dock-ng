# M3b: Reviewed QML baseline and CI gate

Date: 2026-09-30

Branch: `codex/modernization-m0-baseline`

Starting revision: `6def9032d8943075124f71bcd12be6c2a0798ac0`

Status: implementation recorded; remote baseline and application-host smoke
remain Pending validation.

## Implementation

The deep QML lint path now records bounded machine-readable chunks, each
process's stdout and stderr, the complete tracked-file inventory, per-file and
per-process exit codes, qmllint version, import roots and a normalized current
report. Caller-supplied QML import environment variables are removed before
qmllint runs. The source build and staged Latte roots remain explicit; system
Qt/Plasma QML modules remain available for host types.

Generated `qmldir` files are checked for the expected module URI and their
declared plugin library and typeinfo file. The checker follows each module's
actual `typeinfo` filename. Import failures for build-owned `core`, containment
and tasks modules fail independently of the warning baseline. `private.app`
metadata and artifacts are checked, but its plugin resolves symbols exported
by the main executable. Standalone qmllint or a separate QML engine cannot
prove that host contract; an application-host smoke test remains required.

Following the user's platform clarification, the Debian 13.7 install verifier
now records CMake, Qt, KF and Plasma versions on all distro runs and enforces
the Debian 13 floors: CMake 3.31.6, Qt 6.8.2, KF 6.13, `libplasma-dev` 6.3.5
and `plasma-workspace-dev` 4:6.3.6 (Debian's packaged development and desktop
versions). The target desktop runtime is recorded during local desktop retest.
The source-declared compatibility floor remains CMake 3.20 / Qt 6.6 / KF 6.0 /
Plasma 6.3. The separate Debian 13 container exercises the distribution's
default CMake; Gentoo, Arch and Fedora remain forward-compatibility checks.

The baseline comparator runs in every build matrix job. GitHub Actions now
also runs for `codex/**` branches and retains the raw baseline evidence as an
artifact. The first local baseline is from Gentoo 2.18, qmllint/Qt 6.11.2 and
contains 7,343 occurrences across 10 categories and 236 source files. All four
local GCC/Clang Debug/Release reports have identical SHA-256:
`6b4fd15eb6ae5dc73bde467680e690637904df47e34ee94421e711e0ec99e7a6`.
The detailed category counts and scoped dynamic-interface exceptions are in
[`qmllint-backlog-plan.md`](../qmllint-backlog-plan.md), and the baseline is
[`qmllint-baseline.json`](../qmllint-baseline.json).

The local candidate intentionally fingerprints `gentoo-2.18`; the CI build
container is Ubuntu 24.04 with KDE Neon packages. CI will reject this mismatch
until its own run supplies a complete comparable report. Keep that failure
Pending validation and replace the baseline only after reviewing the uploaded
CI report and its additions/removals. No warning category was promoted because
none has two zero-warning runs.

Docker Compose now defaults Docker Hub base images to DaoCloud's mirror and
accepts per-image `LATTE_*_IMAGE` overrides. All eight configured images were
pulled, refreshed from their current package repositories and rebuilt. Ubuntu's
Deb822 source rewrite was verified by a successful USTC package-index update;
HTTP is used because this host's HTTPS proxy rejects the USTC certificate.
Debian 13, Ubuntu 26.04, Arch, Fedora 44, openSUSE Tumbleweed, Mageia 10,
Gentoo and NixOS all passed their complete local install verifiers. Gentoo was
run against a local `1.2.51` source archive after its live-ebuild fetch failed
without the host's GitHub proxy. Nix verification now stages source without
ignored `build*` directories so stale host CMake caches are not copied into the
Nix store. Mageia now selects UTF-8 only when that locale is installed.

The Debian 13 image verified CMake 3.31.6-2, Qt 6.8.2, KF 6.13,
`libplasma-dev` 6.3.5-1 and `plasma-workspace-dev` 4:6.3.6-2. The refreshed
moving stacks also built and installed successfully: Ubuntu (Qt 6.10.2/KF
6.24), Arch/Fedora/openSUSE (Qt 6.11.2/KF 6.30), and Mageia (Qt 6.10.0/KF
6.22). Gentoo and NixOS resolve Qt 6.11.2 with KF 6.29 and 6.30,
respectively. Headless Plasma version probes now suppress core dumps if the
runtime cannot start without a desktop session.

## Validation

The first feature-branch workflow run
[36598722902](https://github.com/ruizhi-lab/latte-dock-ng/actions/runs/36598722902)
completed with the QML syntax check and all distro install/package jobs except
the Debian 13 build-stack assertion passing. Four preset build jobs failed
before configuration because their multiline actions steps ran under `sh` and
rejected Bash `pipefail`; the Debian 13 assertion expected `libplasma-dev`
6.3.6 although the distribution publishes 6.3.5-1. Both causes are corrected
in the follow-up change and require a successful rerun before this batch passes.

- `python3 -m py_compile scripts/qmllint-baseline.py autotests/test_qmllint_baseline.py` — passed.
- `python3 autotests/test_qmllint_baseline.py` — **19/19 passed**, including
  execution failures, incomplete coverage, module plugin/typeinfo presence and
  fatal build-owned import diagnostics.
- `bash -n scripts/qmllint-deep.sh` — passed.
- `bash -n docker/verify-install.sh` — passed.
- `bash -n docker/verify-nix-nixos.sh docker/verify-ebuild-gentoo.sh` — passed.
- `bash scripts/qmllint.sh` — passed for all 236 tracked QML files.
- `python3 scripts/qmllint-baseline.py check-modules --build-qml build/modernization/gcc-debug/qml` — passed.
- `bash scripts/qmllint-deep.sh build/modernization/<preset>` — passed for all
  four local presets. Each had complete 236-file coverage and zero new or
  removed diagnostic identities against the candidate baseline.
- `ctest --preset <preset> -R '^qmllintbaselinetest$' --output-on-failure` —
  passed in all four preset trees. `packagingcontracttest` also passed in all
  four trees after adding the Debian build-stack contract.
- `git diff --check` — passed.
- Workflow YAML parsed and the GCC/Clang Debug/Release matrix and feature branch
  trigger were checked locally.
- Follow-up remote Actions results and an application-host smoke run — Pending validation.
- `packagingcontracttest` and `sourcecontracttest` rebuilt and passed after the
  package-floor, mirror and KWayland target changes.

Raw measurements and logs are in the ignored local directories
`build/modernization/<preset>/qmllint-baseline/` and
`build-modernization-baseline/logs/`. CI uploads the corresponding evidence
for 14 days.

## Handoff

1. Read the feature-branch GitHub Actions run. Download a complete successful
   job's QML baseline artifact and verify every preset report has identical
   file coverage and diagnostics.
2. If fingerprints match across build presets, review and replace the checked-in
   candidate with the Ubuntu/Neon report, then rerun comparison. If they differ,
   identify the differing system import or package rather than normalizing it
   away.
3. Add or document an application-host smoke path for `org.kde.latte.private.app`.
   The plugin's CMake contract intentionally resolves Latte symbols from the
   executable, so do not attempt to treat standalone plugin import as proof.
4. Only after both validations pass, mark M3b Complete and begin M4a.

M1a's CMake 3.20 CTest has one missing-icon environment failure, and M1b's
remote workflow rerun remains Pending validation. No desktop runtime behavior
changed in this batch.
