# Offline user guide

The settings windows open the installed HTML guide through
`app/settings/handbook.cpp`. Keep help in the window toolbar/footer, outside
settings tabs. Each menu opens its window’s guide section: `edit-dock` or
`configure-latte`. The `copies-and-clones` section belongs to the guide’s own
navigation, not to either window’s help menu. Its sub-anchors
(`create-independent-copy`, `create-clone`, `differences`) document how each
kind of dock is created, how it is configured afterwards and how they differ.
The `configure-latte` chapter also documents `layout-details`, `docks-dialog`,
`screens-dialog` and `context-menu-actions`, because those dialogs are opened
from the Layouts Editor and Preferences windows.

English is the fallback. Simplified Chinese (`zh_CN`, including Hans) and
Traditional Chinese (`zh_TW`, including Hant and Hong Kong) follow KDE's ordered
UI language list. A language is searched across data prefixes before trying the
next language. Missing files and browser-launch failures produce translated
messages rather than silently failing.

The language switcher is a native `<select>` dropdown, not a flat row of links.
It lists every language directory under `po/` plus `en`. A language with its own
`docs/help/<code>/index.html` points at that translation; every other language
points at the English guide and its option is labelled with an explicit
“English” suffix. The select uses one inline `onchange` handler to navigate;
the table of contents and all guide links still work without scripts. Keep the
select complete when a UI language is added or removed; `handbookdocumenttest`
compares its `data-lang` entries with `po/`.

Keep every language's section IDs, group rows, advanced badges, option coverage
and table row counts aligned. The left directory stays visible beside the option
tables and moves above the content on narrow screens. Use the actual UI labels;
Chinese pages retain the English source labels for cross-reference. Options that
need Advanced mode carry the `.advanced` badge, and each tab repeats the rule in
a short note. Update descriptions when option behavior, scope, defaults or
supported plugins change. Dock settings, widget settings, launcher sharing, layout settings and
global preferences have different scopes. Independent copies and linked clones
must remain distinct throughout the guide: an independent copy is created with
Duplicate Dock and is configured separately; a clone is created by the original
dock’s On All Screens or On All Secondary Screens mode and is configured through
its original. Use the UI terms for clones (`克隆` in `zh_CN`, `複本` in
`zh_TW`), `小部件` in `zh_CN` and `元件` in `zh_TW`; do not mix in `小程序`,
`组件` or untranslated English labels for the same control.

## Translation status

`en`, `zh_CN` and `zh_TW` have complete guide pages. Every other KDE UI
language is listed in the selector and opens the English guide with an explicit
“English” fallback label; it is not a machine-generated translation. To add a
language, create `docs/help/<code>/index.html` with the same section IDs, group
rows, advanced badges and table row counts, then update the selector and run
`handbookdocumenttest`. Do not add an untranslated copy under a language code;
the fallback already covers it.

The HTML and stylesheet are installed under `latte-dock-ng/help` in the standard
data directory. All assets and navigation are local; no network access or extra
runtime dependency is required. Interface strings remain in `latte-dock.po`;
additional interface languages use English until a matching guide is provided.
