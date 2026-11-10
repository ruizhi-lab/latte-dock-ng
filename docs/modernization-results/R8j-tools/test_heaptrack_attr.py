#!/usr/bin/env python3
"""Unit tests for heaptrack_attr.py (R8j step 3): the parser, the sites and the window sums.

Run it by hand; it is not registered in CTest:

    python3 -I docs/modernization-results/R8j-tools/test_heaptrack_attr.py

PAIR is the interpreted output of a test program that makes two allocations of one size at
one site (called from two lines of main), frees the first, and makes two more. It was
recorded with heaptrack 1.5.80 and copied verbatim. The other timelines are small and
written by hand in the same format.
"""

from __future__ import annotations

import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import heaptrack_attr as ha  # noqa: E402

MIB = 1024 * 1024

PAIR = """v 10650 3
X ./pair
I 1000 7cf173
s 1b /data/tmp/r8j-ht/synth/pair
s f linux-vdso.so.1
s 2c /usr/lib64/heaptrack/libheaptrack_preload.so
s 14 /usr/lib64/libc.so.6
s 19 /usr/lib64/libunwind.so.8
s 32 /usr/lib/gcc/x86_64-pc-linux-gnu/16/libstdc++.so.6
s 1b /lib64/ld-linux-x86-64.so.2
s 14 /usr/lib64/libm.so.6
s 31 /usr/lib/gcc/x86_64-pc-linux-gnu/16/libgcc_s.so.1
s e _dl_start_user
s 42 /var/tmp/portage/sys-libs/glibc-2.43-r4/work/glibc-2.43/elf/rtld.c
i 7f5553c2b45f 7 a b 3b3
t 1 0
s 8 _dl_init
s 45 /var/tmp/portage/sys-libs/glibc-2.43-r4/work/glibc-2.43/elf/dl-init.c
i 7f5553c131fb 7 c d 79
t 2 1
s 14 call_init::call_init
s 9 call_init
i 7f5553c1312d 7 e d 4a f d 1a
t 3 2
i 7f55536b69c1 6
t 4 3
a 12000 4
+ 0
s 6 _start
i 55c4dd4780c4 1 10
t 5 0
s 11 __libc_start_main
i 7f5553a085d8 4 11
t 6 5
i 7f5553a084b0 4
t 7 6
s 4 main
s 1d /data/tmp/r8j-ht/synth/pair.c
i 55c4dd47825c 1 12 13 15
t 8 7
s 6 site_a
i 55c4dd4781a1 1 14 13 9
t 9 8
a 3e8 9
+ 1
i 55c4dd478266 1 12 13 16
t a 7
t 9 a
a 3e8 b
+ 2
- 1
i 55c4dd47827f 1 12 13 18
t b 7
s 6 site_b
i 55c4dd478202 1 15 13 f
t c c
a 7d0 d
+ 3
- 2
- 3
- 0
c 0
R 40b

# strings: 21
# ips: 12
"""


def lines_of(text: str) -> list:
    return text.splitlines()


class ParserTest(unittest.TestCase):
    def setUp(self) -> None:
        self.model = ha.parse(lines_of(PAIR))

    def test_record_counts_match_the_format(self):
        self.assertEqual(len(self.model.strings), 21)
        self.assertEqual(len(self.model.ips) - 1, 12)
        self.assertEqual(len(self.model.traces) - 1, 13)
        self.assertEqual(len(self.model.infos), 4)
        self.assertEqual(len(self.model.events), 8)
        self.assertEqual(self.model.unparsed_lines, 0)
        self.assertEqual(self.model.unknown_info_events, 0)

    def test_string_table_is_one_based_in_definition_order(self):
        self.assertEqual(self.model.string(18), "main")
        self.assertEqual(self.model.string(20), "site_a")
        self.assertEqual(self.model.string(21), "site_b")
        self.assertEqual(self.model.string(19), "/data/tmp/r8j-ht/synth/pair.c")

    def test_sites_resolve_names_files_and_call_lines(self):
        keys = {info: ha.site_key(self.model.frames(trace)) for info, (_, trace) in enumerate(self.model.infos)}
        self.assertTrue(keys[1].startswith("site_a (pair.c:9) < main (pair.c:21)"), keys[1])
        self.assertTrue(keys[2].startswith("site_a (pair.c:9) < main (pair.c:22)"), keys[2])
        self.assertNotEqual(keys[1], keys[2])
        self.assertTrue(keys[3].startswith("site_b (pair.c:15) < main (pair.c:24)"), keys[3])
        self.assertTrue(keys[0].startswith("??@libstdc++.so.6"), keys[0])

    def test_frees_refer_to_infos_per_the_format(self):
        # "- 1" frees the first site_a allocation; the second site_a allocation (info 2) is freed later.
        frees = [info for _, info, sign in self.model.events if sign < 0]
        self.assertEqual(frees, [1, 2, 3, 0])


class WindowTest(unittest.TestCase):
    def test_all_pair_events_in_one_window_are_transient(self):
        model = ha.parse(lines_of(PAIR))
        result = ha.attribute(model, [(1, 0.0, 10.0)])
        totals = result["windows"][0]["totals"]
        # The four infos each allocate once and free once inside the window.
        expected = 0x12000 + 0x3E8 + 0x3E8 + 0x7D0
        self.assertEqual(totals["alloc_bytes"], expected)
        self.assertEqual(totals["freed_bytes"], expected)
        self.assertEqual(totals["transient_bytes"], expected)
        self.assertEqual(totals["net_bytes"], 0)
        chosen = ha.attribution_set(result["windows"][0]["sites"])
        self.assertEqual(len(chosen), 1)  # the 72 KiB stdio buffer is 95 % of the transient bytes
        self.assertTrue(chosen[0].startswith("??@libstdc++.so.6"))


TIMELINE = """s 3 abc
i 1000 1 1 1 1
t 1 0
a 100 1
c 0
+ 0
c 3e8
- 0
c 7d0
+ 0
c 898
- 0
c bb8
+ 0
"""


class TimelineTest(unittest.TestCase):
    """One 256-byte info allocated and freed at known times (heaptrack time in ms)."""

    def setUp(self) -> None:
        self.model = ha.parse(lines_of(TIMELINE))

    def test_events_carry_the_time_of_the_last_c_line(self):
        self.assertEqual([(t, sign) for t, _, sign in self.model.events], [(0, 1), (1000, -1), (2000, 1), (2200, -1), (3000, 1)])

    def test_windows_split_allocations_and_frees(self):
        windows = [(1, 0.0, 500.0), (2, 900.0, 1100.0), (3, 1500.0, 2100.0), (4, 2900.0, 3100.0)]
        result = ha.attribute(self.model, windows)["windows"]
        self.assertEqual(result[0]["totals"]["net_bytes"], 256)  # allocation at 0 ms, not freed in window 1
        self.assertEqual(result[1]["totals"]["freed_bytes"], 256)  # free at 1000 ms
        self.assertEqual(result[1]["totals"]["net_bytes"], -256)
        self.assertEqual(result[2]["totals"]["net_bytes"], 256)  # allocation at 2000 ms, freed at 2200 ms: transient
        self.assertEqual(result[2]["totals"]["transient_bytes"], 0)  # the free is at 2200, outside [1500, 2100]
        self.assertEqual(result[3]["totals"]["net_bytes"], 256)

    def test_transient_estimate_needs_both_events_inside_the_window(self):
        windows = [(1, 1900.0, 2300.0)]  # allocation at 2000 and free at 2200 both inside
        result = ha.attribute(self.model, windows)["windows"][0]["totals"]
        self.assertEqual(result["transient_bytes"], 256)
        self.assertEqual(result["net_bytes"], 0)  # one allocation and one free inside the window
        # The allocation at 3000 ms is outside this window and is not counted.
        self.assertEqual(result["alloc_bytes"], 256)

    def test_unknown_info_is_counted_not_fatal(self):
        model = ha.parse(lines_of(TIMELINE + "+ 9\n"))
        self.assertEqual(model.unknown_info_events, 1)


class ReuseTest(unittest.TestCase):
    def test_reuse_share_is_computed_against_the_window_one_set(self):
        model = ha.parse(lines_of(PAIR))
        result = ha.attribute(model, [(1, 0.0, 10.0), (2, 20.0, 30.0)])
        reuse = ha.reuse_check(result)
        self.assertEqual(reuse["windows"]["2"]["transient_bytes"], 0)
        self.assertIsNone(reuse["windows"]["2"]["share_from_window_1_set"])


if __name__ == "__main__":
    unittest.main()
