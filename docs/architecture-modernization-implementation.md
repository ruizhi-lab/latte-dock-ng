# Modernization Batch Implementation Guide

Use with the [plan](architecture-modernization-plan.md),
[source baseline](architecture-modernization-baseline.md),
[architecture map](architecture-overview.md), [AGENTS.md](../AGENTS.md) and
[testing guide](development-testing-guide.md). This is execution guidance,
not evidence that any batch has passed.

## Common execution rules

1. Select exactly one authorized batch ID. Check its plan dependencies and
   previous handoff. Split combined ledger rows before reporting slice progress.
2. Record branch, full HEAD and working-tree state. Reconcile changes since the
   recorded main baseline; do not silently pull, switch branches or overwrite
   user edits. If remote main advanced, explain the relevant delta in the batch
   record before applying old design assumptions.
3. Use CodeGraph first when indexed. Read only the selected entry points,
   callers, consumers and nearby contracts. Do not survey the whole repository
   again or ask another agent to rediscover the same paths.
4. Follow the allowed file families below. A necessary extra file is allowed
   when its dependency is traced and explained; an unrelated component is a
   scope change requiring a new task. No whole-tree formatting or cleanup.
5. Preserve public behavior and local invariants. Put changed rationale beside
   production code; these documents are not a substitute for source comments.
6. Capture commands, full logs and exit status. Use eight build jobs. Run builds
   sequentially unless measured resource headroom supports concurrency.
7. Required unavailable/failed checks leave Pending validation. Continue useful
   independent work, but do not start dependent runtime optimizations without a
   usable baseline. Source contracts, offscreen tests and desktop retests are
   different evidence classes.
8. Finish by updating the plan ledger and a durable English batch report under
   docs/modernization-results/ when that batch starts. Record the exact next
   command/action and Git authorization state. No new tool-specific rule files.

Routine configuration and bounded tooling work is suitable for a smaller
model. For window-state routing, render invalidation, asynchronous QML timing
or ownership, follow the selected design and its test matrix. If an assumption
fails, produce a concrete revised design and evidence for boundary review;
do not improvise a broader refactor or remove a fallback to make tests pass.
User instructions and existing session authorization take precedence over
process suggestions here. No automatic subagent or chat creation is requested.

To keep context small, read this guide through Common verification commands,
then only the selected batch section, Final slice review and Handoff template.
The baseline record is a lookup reference for the selected source area. Read
the performance protocol only for P batches. Once a dependency is evidenced in
its handoff, verify that evidence instead of repeating its entire investigation.

## Common verification commands

Before M1 exists, use fresh directories for M0. Run this from the repository
root in Bash; change the build-root name if it already contains unrelated work.

```bash
set -euo pipefail
modernization_build_root="$PWD/build-modernization-baseline"
mkdir -p "$modernization_build_root/logs"
for compiler in gcc clang; do
    if [[ "$compiler" == gcc ]]; then
        modernization_cc=gcc
        modernization_cxx=g++
    else
        modernization_cc=clang
        modernization_cxx=clang++
    fi
    for mode in Debug Release; do
        modernization_dir="$modernization_build_root/$compiler-$mode"
        modernization_log="$modernization_build_root/logs/$compiler-$mode"
        cmake -S . -B "$modernization_dir" \
            -DCMAKE_C_COMPILER="$modernization_cc" \
            -DCMAKE_CXX_COMPILER="$modernization_cxx" \
            -DCMAKE_BUILD_TYPE="$mode" -DBUILD_TESTING=ON \
            -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
            2>&1 | tee "$modernization_log-configure.log"
        cmake --build "$modernization_dir" --parallel 8 \
            2>&1 | tee "$modernization_log-application.log"
        cmake --build "$modernization_dir" --target latte-autotests --parallel 8 \
            2>&1 | tee "$modernization_log-tests-build.log"
        ctest --test-dir "$modernization_dir" --show-only=json-v1 \
            > "$modernization_log-registration.json"
        ctest --test-dir "$modernization_dir" --output-on-failure \
            2>&1 | tee "$modernization_log-ctest.log"
    done
done
bash scripts/qmllint.sh
bash scripts/qmllint-deep.sh "$modernization_build_root/gcc-Debug"
bash scripts/qmllint-deep.sh "$modernization_build_root/clang-Debug"
```

Registration JSON must contain a nonempty test set. Compare names and required
executables, not just counts or CTest exit status. Add Release deep lint when
module output differs. These commands create evidence; they do not enforce
zero warnings until M2. Classify pre-existing failures without changing code
under M0. Retain the raw QML logs before another run overwrites them.

After M1a, the planned preset names are gcc-debug, gcc-release, clang-debug and
clang-release. Each build preset builds the default application/plugin targets
and latte-autotests. The following commands are future interfaces until M1a
is implemented, not tools already present at the source baseline:

```bash
cmake --preset gcc-debug
cmake --build --preset gcc-debug
ctest --preset gcc-debug
```

Use the equivalent Clang preset and relevant Release presets. Do not install
sanitizer or performance binaries over the user-mode Debug test binary.
For runtime work follow AGENTS.md's exact install/stop/fresh-log/dev-env/
setsid-and-nohup launch sequence, then user feedback and log review. The
[testing guide](development-testing-guide.md#runtime-retest-workflow) supplies
clean-quit/coredump requirements. Performance A/B builds use the same mode on
both sides and the [performance protocol](performance-validation-guide.md).

## M0: Reproducible quality baseline

**Scope:** evidence report only; no production or CI changes.
**Read:** root/application/plugin/autotest CMake files, current workflows,
QML lint scripts, Nix files and the source baseline.

**Steps:** run fresh GCC/Clang Debug/Release configurations; inventory compiler,
CMake, ECM, Qt, Plasma and lint versions; save effective compile/link commands;
compare sorted registered test names and missing executables. Classify tests
as production behavior, copied logic, source contract or host smoke where
verified. Run syntax/deep lint; record categories, diagnostic identities and
import roots. Record unavailable tools and existing failures separately.

**Acceptance:** four configuration reports, zero unexplained test-list
mismatches, actual test execution results, compiler warning inventory and
comparable lint output. A pre-existing failure may be documented as a baseline
limitation, but a dependent batch cannot claim green acceptance over it.
**Handoff:** exact commands and minimal follow-up needed for each unresolved
failure; distinguish M0 evidence collection from a validated green baseline.

## M1a: Reproducible presets

**Files:** CMakePresets.json (new), .gitignore, .clangd, the Debian 13 build
version assertion and testing documentation.
**Do not change:** install.sh behavior or the source-declared dependency floors.

**Steps:** use JSON schema version 2 compatible with CMake 3.20. Set explicit
C/C++ compilers, separate binary directories, Debug/Release, BUILD_TESTING=ON
and compilation database export. Use a supported explicit generator available
in the documented environment. Add four build/test presets, eight jobs,
application/test build coverage and outputOnFailure; reject an empty test set.
Ignore CMakeUserPresets.json. Retain .clangd's existing build default or document
an explicit local database selection; do not hard-code a developer path or
create a global symlink that changes another developer's database selection.

**Acceptance:** list/configure/build/test all four presets; compare registrations
with M0. Verify schema-v2 parsing with CMake 3.20 as well as the local version.
Also record the Debian 13.7 default CMake 3.31.6-2 and desktop stack (Qt 6.8.2,
KF 6.13, Plasma 6.3.6) from its package build. Keep these environment checks
distinct from the source-declared compatibility floors. An unavailable check
stays pending. Existing user-mode install remains supported. Do not introduce
unsupported workflow presets.

## M1b: Use presets in CI

**Files:** .github/workflows/build.yml, presets and testing documentation only.

**Steps:** map each compiler job to the matching preset. Cover Debug and Release
or add an equivalent explicitly recorded check matrix. Replace touched project
build parallelism with eight jobs. Keep syntax lint, deep lint and distribution
installation/package jobs. Preserve tool versions/flags in logs and upload
failure evidence. Do not change distribution repositories or action versions
as unrelated cleanup.

**Acceptance:** local equivalents pass; workflow parses and matrix/preset
mapping is verified. Actual CI acceptance requires run URLs for affected jobs;
without an authorized push/CI run it remains pending remote validation. On main,
finish the reviewable patch and local checks before requesting commit approval.

## M2a: Strict compiler warning gate

**Files:** root/target CMake, a focused cmake helper if useful, presets and CI.

**Steps:** keep KDECompilerSettings as the diagnostic source. Add a named
LATTE_STRICT_WARNINGS option and a target-scoped application mechanism; enable
strict mode for project development/CI. A documented downstream escape hatch
must not be presented as satisfying the zero-warning policy. For CMake 3.20
use compatible GCC/Clang options; version-guard COMPILE_WARNING_AS_ERROR if used.
Enumerate all first-party targets, including those created before the helper,
logging, preview/add-launcher executables, plugins and EXCLUDE_FROM_ALL tests.
Do not apply flags to imported dependencies or capability-probe code globally.

**Acceptance:** fresh command inspection proves every intended target receives
the gate. A disposable fixture using the same helper fails on a deliberate
warning for GCC/Clang Debug/Release and passes after correction. Build the real
application/tests with zero warnings; retain configure/link/QML diagnostics as
separate classes. The fixture must not modify committed production code.
If pre-existing warnings appear, record them and do M2b slices; keep M2a pending
until the actual gate is green. Do not disable strict mode to complete it.

## M2b: One warning family

**Files:** only the diagnosed production/test files and associated build rule.

**Steps:** group warnings by cause; select one group, preserve behavior, format
only touched C++ files and inspect the diff. A suppression needs a specific
upstream/compiler cause, scope and removal condition beside the owning code.

**Acceptance:** no selected warnings in GCC/Clang Debug/Release; affected tests
pass and runtime changes receive the desktop checks. No blanket -Wno-* or
warning-as-error exceptions to first-party targets. Return to M2a after cleanup.

## M3a: Stable QML diagnostic comparison

**Files:** a new focused scripts/qmllint-baseline.py (or equivalent), parser
fixtures under autotests, and test registration. No QML warning cleanup yet.

**Steps:** represent identities as relative file, category, normalized message
and multiplicity; exclude line shifts and temporary/import absolute prefixes.
Do not erase meaningful identifiers/property names. Include tool/environment
fingerprints and a manifest of attempted files and exit codes. Prefer machine
output when the selected qmllint version supports it; otherwise parse its
verified format. A changed tool version needs a new comparable baseline.

**Acceptance fixtures:** a new identity fails even if another warning vanished;
removal passes; duplicates are counted; path/line shifts compare equal;
meaningful message changes fail; tool-version mismatch, execution failure,
truncated/malformed diagnostics and incomplete file coverage fail closed.
A valid zero-diagnostic run is allowed when execution/coverage are proven.
Do not infer success from an empty log or a category total alone.

## M3b: Baseline, backlog and CI gate

**Files:** lint scripts, comparator, reviewed machine baseline, new
docs/qmllint-backlog-plan.md, CI and testing docs.

**Steps:** establish one controlled lint environment using a reproducible
container/toolchain identity, and record matching imports. Preserve unconditional
syntax errors and missing/failed project-module imports. Check actual generated
qmldir plugin/typeinfo filenames and libraries; system/user modules must not
mask missing build outputs. private.app additionally needs its intended host
smoke path; a standalone import is insufficient. Publish a measured category
backlog and precise dynamic-interface exceptions. Baseline refresh is an
explicit reviewable addition/removal report, never automatic CI blessing.

**Acceptance:** comparator fixtures and baseline comparison pass in the declared
environment. Syntax, missing plugin/metadata and failed import fixtures still
fail. Raw logs and environment manifests are retained. Resolve the historical
7k+ comment using fresh measurement. Promote one category only after two
comparable zero-warning runs and green CI; no whole-file missing-property waiver.
Live QML behavior is checked separately when QML source is changed later.

## M4a: Targeted static analysis

**Files:** .clang-tidy (new) or scoped clazy configuration, a runner/CI hook,
matching compilation-database selection and testing docs.

**Steps:** choose a small documented set of useful checks for changed
first-party code; exclude generated sources without hiding production warnings.
Start on one owned component. Each discovered fix is a separate scoped slice.

**Acceptance:** an intentional disposable issue is detected by the real runner;
selected production files have no unexplained findings; tool/database versions
and exclusions are recorded. No whole-tree automated rewriting.

## M4b: ASan/UBSan configuration

**Files:** CMake/presets, selected test setup, CI and testing docs.

**Steps:** opt-in instrumented build tree with compatible compile/link flags
for participating executables and project plugins. Keep release packages and
normal install unchanged. Start offscreen/isolated-bus tests. Include helper
instrumentation when a tested scenario actually starts the preview process.

**Acceptance:** disposable faults demonstrate both tools; targeted production
cases pass without unexplained findings. Inventory instrumented modules and
third-party limitations. Suppressions are reproduced and narrow; no all-leaks
or all-Qt suppression. Real shutdown findings are verified in the desktop path.

## M5a: Extract only hiding-blocker policy

**Files:** app/view/visibilitymanager.h/.cpp, a nearby small value helper,
focused production-helper tests and autotests/CMakeLists.txt.
**Read:** hidingIsBlocked, hasBlockHidingEvent, addBlockHidingEvent,
removeBlockHidingEvent, onHidingIsBlockedChanged and all m_blockHidingEvents uses.

**Steps:** replace the private string-list manipulation with a small value-type
helper owned by VisibilityManager. Preserve unique nonempty event names and
empty/nonempty transition semantics. Keep signal emission, hide/show timers,
QML APIs and window-system interaction in VisibilityManager. The helper reports
whether blocked state changed; it is not a new QObject or second state owner.
Use the existing header-only production-helper style if appropriate; no new
static target is required merely for four small operations.

**Acceptance:** tests execute the production helper for empty names, duplicate
adds, unknown removes, two simultaneous blockers and final removal. Existing
behavior/test suite remains intact. A controlled production-helper mutation
fails these tests. GCC/Clang and targeted sanitizers pass. Desktop verification
checks menu, drag, shortcut and edit blockers; record signal/timer wiring limits.

## M5b: Replace copied blocker tests and verify wiring

**Files:** visibilitylogictest, selected host/integration tests, source contracts
if a narrow wiring lock remains useful, and the M5 helper/wrappers only.

**Steps:** remove only the duplicate BlockHidingEvents implementation; retain
unrelated visibility test cases. Test the production helper and, where a viable
production host fixture exists, use QSignalSpy for the wrapper's public notify
behavior. Do not instantiate the entire desktop with unbounded mocks solely to
claim unit coverage. If wrapper hosting is unavailable, retain a focused wiring
contract plus actual desktop evidence, explicitly reporting that distinction.

**Acceptance:** adding the first blocker and removing the last notify exactly
once; duplicate/empty/middle changes do not prematurely unblock. Overlapping
menu/drag/shortcut blockers and destruction pass desktop retest with clean logs
and clean quit. Report coverageestimate after the test change, with its heuristic
limits. Essential behavior is explainable beside the production helper/wrapper.

## P0: Comparable performance baseline

**Scope:** evidence and optional read-only measurement tooling; no behavior fix.
**Read:** [performance protocol](performance-validation-guide.md), selected
source entry points and M0 manifests.

**Steps:** select the first hotspot using profiles and counters. Record idle,
hidden, hover, edge-change and multi-window scenarios. Preserve an A binary
with matching QML/plugins/helpers; measure B using the same compiler/mode,
configuration and warmup. Separate instrumented work counts from uninstrumented
CPU/latency samples. Any collector is limited to identified Latte process IDs
and read-only counters; no all-process dumps or user configuration publication.

**Acceptance:** at least five paired comparable runs for selected scenarios,
raw data/units/method/environment, variability and unavailable metrics. No claim
that CPU samples measure wakeups or RSS measures unique memory/GPU allocation.
Choose one next P batch and predeclare its expected work reduction and guards.

## P1a: Specify and characterize title-event routing

**Files:** batch report, focused test harness/instrumentation and minimal test
seams only. Production behavior remains unchanged in this slice.
**Read:** WaylandInterface track/untrack/updateWindowCache/requestInfo,
AbstractWindowInterface signals, Windows::init and LastActiveWindow consumers.

**Steps:** document the full current title-to-hints path. Count hint schedules/
executions under title-only changes, with no geometry/focus changes. List every
consumer requiring metadata notification. Establish how missing/removed IDs,
eligibility changes and concurrent geometry changes fall back to full updates.
Select one minimal seam that tests production dispatch, not a copied fake
routing algorithm. Review the resulting routing specification before P1b.

**Acceptance matrix:** title updates stay fresh; current baseline's redundant
schedule is observed; geometry/maximize/minimize/fullscreen/activity/desktop,
skipTaskbar/onAllDesktops/parentWindow and blocked/ignored status retain current
update semantics. Title plus geometry cannot cancel an already pending scan.
Active-window/removal immediacy and stale/deleted-window handling are preserved.
Explain requestInfo's possible removal side effects; do not let metadata refresh
reinsert a removed window. Unknown events use the conservative full path.

## P1b: Title-only fast path

**Files:** wm adapter/interface/tracker, focused tests, owning comments and batch
report; no QML task-model redesign or unrelated window API changes.

**Steps:** implement P1a's reviewed route. A title-only notification may use a
separate internal signal/slot, preserving tracker-level metadata notification.
Do not optimize the whole updateWindowCache bucket: desktop membership and
tracking eligibility are not cosmetic. Maintain balanced track/untrack
connections and queued-callback lifetime guards. Preserve full-update fallback
when validity or relevant state changed. Keep the existing debounce intervals.

**Acceptance:** production tests show title-only events cause zero new hint
schedules while metadata notifications still occur; all P1a state transitions
still recompute. Existing pending work remains scheduled. A/B title-heavy
scenario shows reduced intended work and no latency/memory regression;
window drag, hide/dodge modes, task actions, tooltips and clean quit pass.
If benefit is below measurement resolution, report that limit rather than a
CPU percentage. Do not broaden the optimization to obtain a larger number.

## P2a: Icon invalidation and work characterization

**Files:** IconItem tests/test seams and batch report; no cache optimization yet.
**Read:** all source/color/overlay/state setters, schedulePixmapUpdate,
updatePolish/loadPixmap, updatePaintNode, itemChange and geometryChange.

**Steps:** record raster dimensions/DPR/source generation and observed pixmap
loads/texture creations while resizing. Distinguish destination-rectangle
updates from changed pixels. Use actual QIcon/image/SVG sources and theme signals;
string source identity alone is insufficient when the underlying image changes.
Specify a one-entry per-item cache key and dirty-generation rules for P2b.

**Acceptance matrix:** source change including same-name replacement, theme/
SVG repaint, color group, Plasma-theme switch, overlays, enabled/active state,
providesColors, zero size/reappearance, fractional DPR and moving between screens
still update correctly. Keep the geometry-only SVG signal blocker limited to
its current scope. Scene-graph/window recreation cannot retain an invalid GPU
texture. Expected same-key redundancy is measured before selecting P2b.

## P2b: Conservative per-item deduplication

**Files:** IconItem.h/.cpp, its focused tests and owning comments.

**Steps:** implement only P2a's reviewed invalidation scheme. Reuse current
pixels for identical effective requests; still update the destination rectangle
when geometry changes. Do not add an unbounded/global/multi-size cache, share
QSGTexture objects across windows or move QPixmap/GUI work to worker threads.
No maximum-resolution zoom rasterization or quality change in this slice.

**Acceptance:** equivalent requests do not repeat raster/texture work; every
invalidation case in P2a repaints. Dynamic source changes under the same name
are covered. Render-thread ownership and resource release remain correct.
Hover/zoom visual comparison at integer/fractional scale shows no blur, clipping,
wrong colors, flicker or stale textures. A/B work/frame/memory evidence passes.

## P3a: One bounded edge-layout refresh optimization

**Files:** tasks main.qml, focused QML tests and nearby timing comments.

**Steps:** characterize location/formFactor changes and asynchronous delegate
readiness. Define a readiness/convergence condition that includes actual layout
and published geometry, not only unchanged width/height. Restart generations
on a new edge change and reject stale callbacks. Stop early only on proven-ready
paths; preserve the eight-pass bound, immediate pass and existing fallback for
unverified readiness. Do not shorten the interval as a substitute for proof.

**Acceptance:** fewer forceLayout/publication calls in already-ready scenarios;
late delegates, empty tasks, task arrival/removal during relocation and rapid
repeated changes still finish at the latest edge. Four edges, horizontal/
vertical, center/justify, multiple screens and fractional scaling pass. No lost
scroll/minimize geometry or new binding/polish loop. Runtime A/B evidence passes.
If readiness cannot be established, leave behavior unchanged and defer.

## P3b: Edit-state notification and compatibility poll

**Files:** tasks main.qml, the actual containment/bridge assignment site,
focused QML tests and local compatibility comments.

**Steps:** trace direct assignment, bridge attach/detach and tasks arrival.
Prove reliable notification for each supported route before reducing the
central 200ms poll. Keep a narrowly documented fallback for a route with missing
notification; do not add per-task timers or another authoritative edit state.
A broad Plasma version check is not notification proof.

**Acceptance:** live and isolated QML cases cover edit entry/exit, startup bridge
absence, reattachment, empty-to-nonempty tasks and teardown. Menus, dragging,
click actions and tooltips obey the current edit contract. Idle polls decrease
only where correctness is demonstrated; no state lag or stuck edit state.
Supported older Plasma/Qt behavior is checked or remains pending validation.

## M6: One useful internal dependency boundary

**Files:** selected component, its CMake target/callers/tests and architecture map.

**Steps:** select a deterministic boundary after evidence, not by file length.
Write inputs/outputs/state authority and dependencies before moving code.
Characterize behavior, then extract once; application/tests use the same
implementation. A private object/static target is appropriate only if it
actually enforces dependency ownership. Preserve PIC, AUTOMOC, generated
headers, logging and private.app host exports. Do not invent a public ABI.

**Acceptance:** actual removed dependency/duplicate compilation is documented;
GCC/Clang, plugin/host smoke, relevant install paths and desktop cases pass.
If M5's header-only helper needs no target, record that and select a different
justified boundary or defer M6. Do not split a second component in this batch.

## M7: Explicit Nix checks and development environment

**Files:** flake.nix, default.nix and testing docs. Keep flake.lock unchanged.

**Steps:** add a development shell using existing dependency definitions and
an explicit test derivation. Enable BUILD_TESTING, build latte-autotests (they
are excluded from the normal build), then execute CTest with isolated D-Bus/
offscreen setup. Keep test tools separate from release runtime contents and
avoid recursive package/check dependencies. Preserve x86_64-linux support.

**Acceptance:** nix flake check --print-build-logs visibly executes tests;
nix build .#default --no-link --print-build-logs passes; the dev shell configures
and builds the documented preset. Inspect packaged module/helper paths. Real
Plasma cases remain separate. Nix unavailable means pending, not success.

## P4-P6: Conditional evaluations

Each ID starts with an evidence/design slice. An implementation is a separate
user-selected slice after the named boundary is reviewed. No-benefit decisions
are valid Deferred outcomes, not permission to expand the optimization.

| ID | Scope and steps | Acceptance before implementation |
| --- | --- | --- |
| P4 | Measure AppletItem's eligible TasksModel/ActivityInfo/VirtualDesktopInfo instances and filtering; prototype one per-view query owner with per-applet subscriptions | Distinguish shared underlying sources from duplicate proxies. Preserve screen/desktop/activity filters, app identity/grouping, late model roles, attach/detach and last-subscriber destruction. Evidence shows saved work/memory without cross-view state leakage. |
| P5 | Inspect the user-confirmed dock background shadow path: `MultiLayered.qml` → `CustomBackground.qml` → `KirigamiShadowedRectangle.qml`; measure actual allocations and offscreen passes with the shadow on/off | Preserve clipping, margins, theme colors, opacity and visual transitions at fractional scale. Idle effect teardown must save resources without breaking live state or popup/capture visibility requirements. |
| P6 | Select one pure-QML module with explicit inputs; evaluate QML_FILES build-time caching and stable installation/override behavior | Compare cold and warm startup, metadata/URI/resource discovery, both install roots and real host loading. Keep applet package entry points and compatibility imports intact; do not assume everything can use qmltc or the newest Qt APIs. |

All three require the common GCC/Clang/QML checks, appropriate distribution
installation checks, desktop retest and performance protocol for implementation.
No blanket visible:false conversion, extra services or dependency-floor bump.

## Final slice review

Before writing Complete, answer these questions in the handoff:

1. Does the diff stay within the selected scope, including generated/installed
   artifacts? Were source comments and relevant contracts updated together?
2. Do tests call the changed production implementation? What controlled fault
   or negative fixture proves the new check detects its intended failure?
3. Are the compiler/mode/import environments comparable, and were required
   binaries actually built and tests run rather than only registered?
4. What ensures stale callbacks, cached state and removed objects cannot affect
   the next view/task/window generation? Which fallback remains authoritative?
5. Which desktop/CI/Nix/performance checks are still unavailable? Does the ledger
   honestly retain Pending validation until those required checks finish?

For documentation-only batches review accuracy, links and whitespace without
building/restarting the dock. For implementation batches select checks by the
actual change, following AGENTS.md; do not rerun the entire four-build baseline
for every evidence edit or expand tests after green results without a reason.

## Handoff template

Create docs/modernization-results/<batch-id>.md when a batch starts and link it
from the plan ledger. Keep essentials and summarized evidence durable; large raw
logs may live in reproducible artifacts, but /tmp alone is not a handoff.

```text
Batch / slice and selected scope:
Status: Not started | In progress | Pending validation | Complete | Deferred
Source baseline / actual start HEAD / implementation HEAD or uncommitted diff:
Branch / existing user edits preserved:
Dependencies and evidence links:
Files and externally observable behavior changed:
State authority / invalidation / lifetime / fallback constraints:
Toolchain, import environment and artifact identity:
Commands executed with exit status and durable log/artifact locations:
GCC/Clang Debug/Release warning/error results:
Tests registered / run / passed / skipped / failed and name deltas:
Production behavior vs copied logic vs source-contract evidence:
QML diagnostics added/removed and environment compatibility:
Negative fixtures or controlled mutation evidence:
Desktop user feedback / log review / clean-quit and coredump evidence:
Performance paired samples / variability / work counts / tradeoffs:
CI, distribution and Nix checks (passed / failed / unavailable):
Unresolved failures, review assumptions and unavailable checks:
Next exact action, file/symbol or command; do not start another batch implicitly:
Commit approval status / push approval status:
```

## Reusable task prompt

Replace <BATCH_ID> and the final scope preference. This prompt can be pasted to
any implementation model; it deliberately does not require conversation memory.

```text
Implement only batch <BATCH_ID> from docs/architecture-modernization-plan.md.
Read AGENTS.md, the relevant architecture-overview.md entries, the common
rules and selected batch section in architecture-modernization-implementation.md,
and that batch's latest handoff. For performance work read the performance
protocol. The recorded source baseline is main at
e3ef1ddf001aa032cffa62db21cfd97196174746; verify actual HEAD and reconcile changes.
Use CodeGraph first when indexed. Preserve unrelated edits and do not switch
branches. Follow the specified design, file scope, invariants and test matrix.
Produce one reviewable slice, run every applicable acceptance check, and record
failed/unavailable checks as pending. Do not remove compatibility behavior,
weaken tests, automatically update baselines or broaden the refactor to finish.
If an assumption fails, record the concrete evidence and proposed minimal
revision for review; continue independent authorized work.
Update the plan ledger and docs/modernization-results/<BATCH_ID>.md with exact
commands/results and the next action. On main, do not commit or push without
separate explicit approvals. For this run, leave changes uncommitted for review.
Stop after this batch and report the result, remaining validation and risks.
```
