# CI retry: clang-tidy follow-up clazy findings

Source baseline: published `main` commit `9e62bfaa9336c22df475e65ff22ccd6643fd708f`.
CI run `43404434373`, Clang Debug job `131815223553`, failed only in the
`Run gated clazy` step. Clazy reported `range-loop-detach` at
`app/view/indicator/indicator.cpp:395` for `m_configLoader->items()` and at
`app/view/settings/primaryconfigview.cpp:148` for `containment->applets()`.

Both getters return Qt implicitly shared list values. Binding each returned value
to a const local makes the range loop use const iteration. The snapshot preserves
the returned order and contents while avoiding the detach caused by mutable range
iteration over the getter temporary. Neither loop mutates its list; the second
loop stores guarded applet pointers for later configuration updates.

Validation on the repaired tree: isolated clazy 1.17 / Ubuntu LLVM 19.1.1
(the same tool versions as the failed CI) rejected all 49 negative fixtures
and reported no findings in 156 production sources. The host SDK differs from
CI (Plasma 6.7.6 / Qt 6.11.3); this is local proof, not remote acceptance.
No suppression or QML baseline change was made. The two const snapshots
preserve existing runtime semantics and do not require a desktop restart.

The next slice also adds a move-constructor check; its combined build/test
validation is recorded in [T4b](T4b-move-constructor.md). A fresh remote run is
required after publication; rerunning the unchanged failing commit would not
repair these deterministic findings.

The repair was published as `64a7f1ae5`. [Run 43424231020](https://github.com/ruizhi-lab/latte-dock-ng/actions/runs/43424231020)
passed all 23 jobs, including the previously failing Clang Debug clazy step.
The retry is accepted; the uncommitted T4c expansion is a separate batch.
