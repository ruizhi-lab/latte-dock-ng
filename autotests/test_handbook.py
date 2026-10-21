#!/usr/bin/env python3
"""Check shipped offline guides and their local navigation without a browser."""
import unittest
from html.parser import HTMLParser
from pathlib import Path
from urllib.parse import unquote, urlsplit

ROOT = Path(__file__).resolve().parents[1]
HELP = ROOT / "docs" / "help"


class GuideParser(HTMLParser):
    def __init__(self, source):
        super().__init__()
        self.ids = []
        self.links = []
        self.languages = {}
        self.rows = 0
        self.groups = 0
        self.advanced = 0
        self.language = None
        self.has_sidebar = False
        self.in_toc = False
        self.headings = []
        self.toc_targets = []
        self.feed(source)

    def handle_starttag(self, tag, attrs):
        attrs = dict(attrs)
        if "id" in attrs:
            self.ids.append(attrs["id"])
        if tag == "aside" and "toc" in attrs.get("class", "").split():
            self.has_sidebar = True
            self.in_toc = True
        if tag in ("h2", "h3", "section") and "id" in attrs:
            self.headings.append(attrs["id"])
        if tag == "a" and self.in_toc and attrs.get("href", "").startswith("#"):
            self.toc_targets.append(attrs["href"][1:])
        if tag == "option" and "data-lang" in attrs:
            target = attrs.get("data-href", "")
            self.languages[attrs["data-lang"]] = target
            if target:
                self.links.append(target)
        if tag == "html":
            self.language = attrs.get("lang")
        if tag == "tr":
            self.rows += 1
            if "group" in attrs.get("class", "").split():
                self.groups += 1
        if tag == "span" and "advanced" in attrs.get("class", "").split():
            self.advanced += 1
        for attribute in ("href", "src"):
            if attribute in attrs:
                self.links.append(attrs[attribute])

    def handle_endtag(self, tag):
        if tag == "aside" and self.in_toc:
            self.in_toc = False


class HandbookTest(unittest.TestCase):
    def test_languages_have_matching_sections_and_option_coverage(self):
        reference = GuideParser((HELP / "en" / "index.html").read_text(encoding="utf-8"))
        ui_languages = {"en", "zh_CN", "zh_TW"}
        for language in ("en", "zh_CN", "zh_TW"):
            with self.subTest(language=language):
                guide = GuideParser((HELP / language / "index.html").read_text(encoding="utf-8"))
                self.assertEqual(guide.language, language.replace("_", "-"))
                self.assertTrue(guide.has_sidebar)
                self.assertEqual(len(guide.ids), len(set(guide.ids)))
                self.assertEqual(set(guide.ids), set(reference.ids))
                self.assertEqual(guide.rows, reference.rows)
                self.assertEqual(guide.groups, reference.groups)
                self.assertEqual(guide.advanced, reference.advanced)
                self.assertGreater(guide.groups, 0)
                self.assertGreater(guide.advanced, 0)
                self.assertEqual(set(guide.languages), ui_languages)
                for code, href in guide.languages.items():
                    translated = (HELP / code / "index.html").is_file()
                    self.assertEqual(href, f"../{code}/index.html" if translated else "../en/index.html")
                # The left directory is the page's index: every chapter and
                # subsection heading must be reachable from it, in order.
                self.assertEqual(guide.headings, reference.headings)
                self.assertEqual(guide.toc_targets, guide.headings)
                source = (HELP / language / "index.html").read_text(encoding="utf-8")
                self.assertLess(source.index('<aside class="toc">'), source.index("<main>"))
                self.assertGreater(guide.rows, 60)
                for anchor in ("edit-dock", "configure-latte", "copies-and-clones"):
                    self.assertIn(anchor, guide.ids)

    def test_navigation_is_docked_left(self):
        css = (HELP / "style.css").read_text(encoding="utf-8")
        start = css.index(".guide-layout {")
        layout_rule = css[start : css.index("}", start)]
        self.assertIn("display:grid", layout_rule)
        self.assertIn("grid-template-columns", layout_rule)
        toc_start = css.index(".toc {")
        toc_rule = css[toc_start : css.index("}", toc_start)]
        self.assertIn("position:sticky", toc_rule)

    def test_language_switcher_is_a_dropdown(self):
        for path in HELP.glob("*/index.html"):
            source = path.read_text(encoding="utf-8")
            with self.subTest(page=path.parent.name):
                self.assertIn('<select id="language-select"', source)
                self.assertNotIn('<nav aria-label="Language"', source)

    def test_links_and_assets_are_offline_and_resolve(self):
        for path in HELP.glob("*/index.html"):
            for link in GuideParser(path.read_text(encoding="utf-8")).links:
                with self.subTest(page=path.parent.name, link=link):
                    parsed = urlsplit(link)
                    self.assertEqual(parsed.scheme, "")
                    self.assertEqual(parsed.netloc, "")
                    destination = (path.parent / unquote(parsed.path)).resolve() if parsed.path else path
                    self.assertIn(HELP.resolve(), destination.parents)
                    self.assertTrue(destination.is_file())
                    if parsed.fragment:
                        self.assertIn(parsed.fragment, GuideParser(destination.read_text(encoding="utf-8")).ids)

    def test_help_stays_outside_tab_navigation(self):
        qml = (ROOT / "shell/package/contents/configuration/LatteDockConfiguration.qml").read_text(encoding="utf-8")
        self.assertIn("LatteExtraControls.HelpMenuButton", qml)
        self.assertNotIn('pageName: i18n("Help")', qml)
        self.assertNotIn("copiesText:", qml)
        widgets = (ROOT / "app/settings/settingsdialog/settingsdialog.cpp").read_text(encoding="utf-8")
        self.assertIn("addButton(helpButton, QDialogButtonBox::HelpRole)", widgets)
        self.assertNotIn("copiesAction", widgets)
        self.assertIn('Settings::openHandbook(QStringLiteral("configure-latte"))', widgets)


if __name__ == "__main__":
    unittest.main()
