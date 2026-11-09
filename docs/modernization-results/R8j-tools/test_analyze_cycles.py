#!/usr/bin/env python3
"""Unit tests for analyze_cycles.py (R8j): the probe parser, the windows and the fixed verdict rule.

Run it by hand; it is not registered in CTest:

    python3 -I docs/modernization-results/R8j-tools/test_analyze_cycles.py

The inputs are synthetic and built in memory: one timeline with the event order of
session.py, and levels with a known shape for each row of the verdict rule.
"""

from __future__ import annotations

import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import analyze_cycles as ac  # noqa: E402

MIB_KIB = 1024.0
MIB_BYTES = 1024.0 * 1024.0


def levels_for(base, cycle_levels, trimmed, use_gain=0.0, free_gain=0.0, post=(None, None)):
    """A levels dict as levels_of() returns it, from the cycle levels 1 to 10 (MiB)."""
    cycles = {}
    for number, level in enumerate(cycle_levels, start=1):
        cycles[str(number)] = {
            "start": 0.0,
            "rest_end": 0.0,
            "level": level,
            "peak": level,
            "threads_peak": 10,
            "counters": {"uordblks_mib": 50.0 + use_gain * number / 10.0, "fordblks_mib": 20.0 + free_gain * number / 10.0},
        }
    cycles["10"]["counters"] = {"uordblks_mib": 50.0 + use_gain, "fordblks_mib": 20.0 + free_gain}
    return {
        "pre_trim": base + 1.0,
        "post_trim_baseline": base,
        "baseline_counters": {"uordblks_mib": 50.0, "fordblks_mib": 20.0, "hblkhd_mib": 0.0, "arena_mib": 0.0},
        "cycles": cycles,
        "trimmed": trimmed,
        "trimmed_counters": {"uordblks_mib": 50.0 + use_gain, "fordblks_mib": 20.0 + free_gain, "hblkhd_mib": 0.0, "arena_mib": 0.0},
        "post_trim_cycles": {
            "11": {"level": post[0]} if post[0] is not None else {},
            "12": {"level": post[1]} if post[1] is not None else {},
        },
    }


class ProbeParserTest(unittest.TestCase):
    LINE = (
        "1731200000.123 dump seq=3 rc=-1 uordblks=1048576 fordblks=2097152 hblkhd=0 "
        "arena=8388608 keepcost=4096 ordblks=7"
    )

    def test_dump_line_is_parsed_by_seq(self):
        probe = ac.parse_probe([self.LINE])
        record = probe["records"][3][0]
        self.assertEqual(record["kind"], "dump")
        self.assertEqual(record["uordblks"], 1048576)
        self.assertEqual(record["fordblks"], 2097152)
        self.assertEqual(probe["unparsed"], [])

    def test_trim_pair_shares_one_seq(self):
        lines = [
            "1731200001.000 trim-before seq=4 rc=-1 uordblks=1 fordblks=2 hblkhd=0 arena=3 keepcost=0 ordblks=0",
            "1731200001.100 trim-after seq=4 rc=1 uordblks=1 fordblks=2 hblkhd=0 arena=3 keepcost=0 ordblks=0",
        ]
        probe = ac.parse_probe(lines)
        self.assertEqual([r["kind"] for r in probe["records"][4]], ["trim-before", "trim-after"])
        self.assertEqual(probe["records"][4][1]["rc"], 1)

    def test_bad_line_is_reported_not_skipped(self):
        probe = ac.parse_probe(["garbage line"])
        self.assertEqual(probe["unparsed"], ["garbage line"])
        self.assertEqual(probe["records"], {})

    def test_counters_convert_bytes_to_mib(self):
        record = ac.parse_probe([self.LINE])["records"][3][0]
        counters = ac.counters(record)
        self.assertAlmostEqual(counters["uordblks_mib"], 1.0)
        self.assertAlmostEqual(counters["fordblks_mib"], 2.0)


class WindowTest(unittest.TestCase):
    def test_window_excludes_start_and_includes_end(self):
        samples = [{"t": t, "Pss_Anon": 1024.0 * v, "Threads": 10} for t, v in ((10.0, 1.0), (15.0, 2.0), (20.0, 3.0))]
        # (10, 20]: the sample at 10 is out, the samples at 15 and 20 are in.
        self.assertAlmostEqual(ac.mean_level(samples, 10.0, 20.0), 2.5)
        self.assertAlmostEqual(ac.peak_level(samples, 10.0, 20.0), 3.0)

    def test_empty_window_has_no_level(self):
        self.assertIsNone(ac.mean_level([], 0.0, 10.0))


class VerdictRuleTest(unittest.TestCase):
    def test_not_reproduced_when_growth_is_small(self):
        levels = levels_for(100.0, [100.5] * 10, trimmed=100.4)
        self.assertEqual(ac.verdict(levels)["label"], "not reproduced")

    def test_retention_when_trim_returns_the_growth_in_the_first_cycles(self):
        cycle_levels = [105.0, 110.0, 115.0, 115.5, 115.5, 115.5, 115.5, 115.5, 115.5, 115.5]
        levels = levels_for(100.0, cycle_levels, trimmed=101.0, free_gain=15.0)
        result = ac.verdict(levels)
        self.assertEqual(result["label"], "allocator retention")
        self.assertEqual(result["pattern"], "growth only in the first cycles")
        self.assertAlmostEqual(result["trim_release_mib"], 14.5)

    def test_retention_accumulating_across_cycles_is_still_retention(self):
        cycle_levels = [100.0 + 1.5 * k for k in range(1, 11)]
        levels = levels_for(100.0, cycle_levels, trimmed=100.5, free_gain=15.0)
        result = ac.verdict(levels)
        self.assertEqual(result["label"], "allocator retention")
        self.assertEqual(result["pattern"], "growth on most cycles")

    def test_leak_candidate_when_growth_is_live_and_on_most_cycles(self):
        cycle_levels = [100.0 + 1.5 * k for k in range(1, 11)]
        levels = levels_for(100.0, cycle_levels, trimmed=114.5, use_gain=15.0)
        self.assertEqual(ac.verdict(levels)["label"], "live growth (leak candidate)")

    def test_live_growth_that_stopped(self):
        cycle_levels = [105.0, 110.0] + [110.0] * 8
        levels = levels_for(100.0, cycle_levels, trimmed=110.0, use_gain=10.0)
        self.assertEqual(ac.verdict(levels)["label"], "live growth that stopped")

    def test_outside_malloc_when_neither_trim_nor_in_use_explains_growth(self):
        cycle_levels = [100.0 + 1.5 * k for k in range(1, 11)]
        levels = levels_for(100.0, cycle_levels, trimmed=114.5, use_gain=0.0, free_gain=0.0)
        self.assertEqual(ac.verdict(levels)["label"], "outside malloc")

    def test_inconclusive_between_the_trim_fractions(self):
        cycle_levels = [100.0 + k for k in range(1, 11)]  # G = 10 MiB
        levels = levels_for(100.0, cycle_levels, trimmed=107.0, use_gain=5.0)  # TR = 3 MiB, 0.3 G
        self.assertEqual(ac.verdict(levels)["label"], "inconclusive")

    def test_missing_trim_level_is_insufficient_data(self):
        levels = levels_for(100.0, [110.0] * 10, trimmed=None)
        self.assertEqual(ac.verdict(levels)["label"], "insufficient data")

    def test_steps_count_peaks_above_the_previous_level(self):
        cycle_levels = [101.0, 101.0, 101.0, 101.0, 101.0, 101.0, 101.0, 101.0, 101.0, 101.0]
        levels = levels_for(100.0, cycle_levels, trimmed=100.0)
        levels["cycles"]["2"]["peak"] = 102.5  # a peak 1.5 MiB above cycle 1's level
        result = ac.verdict(levels)
        self.assertEqual(result["cycles_with_step"], 2)  # cycle 1 (101 - 100) and cycle 2 (102.5 - 101)


class TimelineTest(unittest.TestCase):
    """levels_of() on one synthetic run with the event order of session.py."""

    def build(self):
        """The timeline of session.py: warm-up, baseline trim, ten cycles, a trim, two cycles."""
        events = [{"event": "warmup-end", "elapsed_s": 60.0}]
        probe_lines = []
        seq = 0
        trims = []  # (start, end) of the trims that set the level

        def signal(name, label, at):
            nonlocal seq
            seq += 1
            events.append({"event": "signal", "name": name, "index": seq, "label": label, "elapsed_s": at})
            if name == "SIGUSR2":
                probe_lines.append(f"1.0 dump seq={seq} rc=-1 uordblks={50 * MIB_BYTES:.0f} fordblks={20 * MIB_BYTES:.0f} hblkhd=0 arena=0 keepcost=0 ordblks=0")
            else:
                probe_lines.append(f"1.0 trim-before seq={seq} rc=-1 uordblks=0 fordblks=0 hblkhd=0 arena=0 keepcost=0 ordblks=0")
                probe_lines.append(f"1.0 trim-after seq={seq} rc=1 uordblks={50 * MIB_BYTES:.0f} fordblks={20 * MIB_BYTES:.0f} hblkhd=0 arena=0 keepcost=0 ordblks=0")

        signal("SIGUSR2", "dump at warm-up end (before the trim)", 60.0)
        signal("SIGUSR1", "trim at warm-up end: baseline", 60.0)
        trims.append(60.0)
        events.append({"event": "baseline-ready", "elapsed_s": 85.0})
        starts = {}
        start = 85.0
        for cycle in range(1, 11):
            starts[cycle] = start
            events.append({"event": "cycle-start", "cycle": cycle, "elapsed_s": start})
            events.append({"event": "rest-end", "cycle": cycle, "elapsed_s": start + 40.0})
            signal("SIGUSR2", f"dump after cycle {cycle}", start + 40.0)
            start += 45.0
        signal("SIGUSR1", "trim after the cycles", 530.0)
        trims.append(530.0)
        events.append({"event": "post-trim-ready", "elapsed_s": 555.0})
        start = 555.0
        for cycle in range(11, 13):
            starts[cycle] = start
            events.append({"event": "cycle-start", "cycle": cycle, "elapsed_s": start})
            events.append({"event": "rest-end", "cycle": cycle, "elapsed_s": start + 40.0})
            signal("SIGUSR2", f"dump after cycle {cycle}", start + 40.0)
            start += 45.0
        # Level: 100 MiB; each started cycle adds 2 MiB; the trim at 530 s returns 15 MiB (to 105);
        # cycles 11 and 12 add 2 MiB each from the trimmed level. A cycle's step begins just after
        # its start, and the trim's effect just after its signal, so the samples at those instants
        # belong to the earlier level (the real sampler takes its samples before the signal).
        samples = []
        t = 0.0
        while t <= start:
            if t <= 530.0:
                level = 100.0 + 2.0 * sum(1 for k in range(1, 11) if starts[k] < t)
            else:
                level = 105.0 + 2.0 * sum(1 for k in (11, 12) if starts[k] < t)
            samples.append({"t": t, "Pss_Anon": level * MIB_KIB, "Threads": 10})
            t += 5.0
        return samples, events, ac.parse_probe(probe_lines)

    def test_levels_follow_the_event_windows(self):
        samples, events, probe = self.build()
        levels = ac.levels_of(samples, events, probe)
        self.assertAlmostEqual(levels["pre_trim"], 100.0)
        self.assertAlmostEqual(levels["post_trim_baseline"], 100.0)
        self.assertAlmostEqual(levels["cycles"]["1"]["level"], 102.0)
        self.assertAlmostEqual(levels["cycles"]["10"]["level"], 120.0)
        self.assertAlmostEqual(levels["trimmed"], 105.0)
        self.assertAlmostEqual(levels["post_trim_cycles"]["11"]["level"], 107.0)
        self.assertEqual(len(levels["cycles"]), 10)
        self.assertEqual(sorted(levels["post_trim_cycles"]), ["11", "12"])
        # The baseline counters come from the trim's trim-after line (seq 2).
        self.assertAlmostEqual(levels["baseline_counters"]["uordblks_mib"], 50.0)

    def test_full_timeline_is_retention_by_the_rule(self):
        samples, events, probe = self.build()
        result = ac.verdict(ac.levels_of(samples, events, probe))
        self.assertEqual(result["label"], "allocator retention")
        self.assertAlmostEqual(result["growth_mib"], 20.0)
        self.assertAlmostEqual(result["trim_release_mib"], 15.0)
        self.assertEqual(result["cycles_with_step"], 10)


if __name__ == "__main__":
    unittest.main()
