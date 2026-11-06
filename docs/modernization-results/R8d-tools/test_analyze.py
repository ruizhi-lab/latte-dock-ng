#!/usr/bin/env python3
"""Unit tests for analyze.py and the parsers of memsample.py (R8d idle study).

Run it by hand; it is not registered in CTest:

    python3 docs/modernization-results/R8d-tools/test_analyze.py

The series are synthetic: flat with noise, linear growth, and a plateau after a rise.
The statistics are checked against known Student t quantiles.
"""

from __future__ import annotations

import random
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import analyze  # noqa: E402
import memsample  # noqa: E402

MIB_KIB = 1024.0
WARMUP_S = 900.0
END_S = 9000.0  # 150 minutes of 30 s samples


def synthetic_rows(value_at, metric: str = "Pss_Anon") -> list:
    rows = []
    for index in range(int(END_S / 30.0) + 1):
        t = index * 30.0
        rows.append({"t": t, metric: value_at(t)})
    return rows


class TStatisticsTest(unittest.TestCase):
    def test_quantiles_match_tables(self) -> None:
        self.assertAlmostEqual(analyze.t_quantile(0.975, 1), 12.7062, places=3)
        self.assertAlmostEqual(analyze.t_quantile(0.975, 2), 4.3027, places=3)
        self.assertAlmostEqual(analyze.t_quantile(0.975, 10), 2.2281, places=3)
        self.assertAlmostEqual(analyze.t_quantile(0.975, 27), 2.0518, places=3)

    def test_linear_fit_is_exact_on_a_line(self) -> None:
        fit = analyze.linear_fit([0.0, 1.0, 2.0, 3.0], [1.0, 3.0, 5.0, 7.0])
        self.assertAlmostEqual(fit["slope"], 2.0, places=9)
        self.assertAlmostEqual(fit["lo"], 2.0, places=6)
        self.assertAlmostEqual(fit["hi"], 2.0, places=6)

    def test_linear_fit_needs_three_points(self) -> None:
        self.assertIsNone(analyze.linear_fit([0.0, 1.0], [0.0, 1.0]))


class VerdictTest(unittest.TestCase):
    def test_flat_series_with_noise_is_no_growth(self) -> None:
        rng = random.Random(7)
        rows = synthetic_rows(lambda t: 265 * MIB_KIB + rng.uniform(-0.3, 0.3) * MIB_KIB)
        metrics = {"Pss_Anon": analyze.fit_metric(rows, "Pss_Anon", WARMUP_S)}
        self.assertEqual(analyze.verdict_for(metrics)["verdict"], "no growth")

    def test_linear_growth_reports_hours_to_100_mib(self) -> None:
        rows = synthetic_rows(lambda t: 265 * MIB_KIB + 5 * MIB_KIB * t / 3600.0)
        metrics = {"Pss_Anon": analyze.fit_metric(rows, "Pss_Anon", WARMUP_S)}
        verdict = analyze.verdict_for(metrics)
        self.assertEqual(verdict["verdict"], "growth")
        self.assertAlmostEqual(metrics["Pss_Anon"]["whole"]["slope"], 5 * MIB_KIB, places=3)
        self.assertIn("+100 MiB in 20.0 h", verdict["detail"])

    def test_rise_then_flat_is_plateau(self) -> None:
        def value(t: float) -> float:
            rise_s = 1800.0  # 20 MiB over the first 30 minutes after warm-up, then flat
            return 265 * MIB_KIB + 20 * MIB_KIB * min(max(t - WARMUP_S, 0.0), rise_s) / rise_s

        rows = synthetic_rows(value)
        metrics = {"Pss_Anon": analyze.fit_metric(rows, "Pss_Anon", WARMUP_S)}
        self.assertEqual(analyze.verdict_for(metrics)["verdict"], "plateau")

    def test_missing_data_is_insufficient(self) -> None:
        self.assertEqual(analyze.verdict_for({"Pss_Anon": None})["verdict"], "insufficient data")

    def test_buckets_need_at_least_five_samples(self) -> None:
        points = [(WARMUP_S + index * 30.0, 100.0) for index in range(10)]
        self.assertEqual(len(analyze.bucket_medians(points, WARMUP_S)), 1)
        self.assertEqual(analyze.bucket_medians(points[:4], WARMUP_S), [])

    def test_warmup_samples_are_dropped(self) -> None:
        points = [(index * 30.0, 1.0) for index in range(20)]  # all inside the first 10 minutes
        self.assertEqual(analyze.bucket_medians(points, WARMUP_S), [])


class MappingTest(unittest.TestCase):
    def test_changes_are_attributed_by_object(self) -> None:
        before = {
            "by_object": {
                "[heap]": {"mappings": 1, "Pss": 100, "Anonymous": 100, "Rss": 100, "Private_Dirty": 100, "Swap": 0},
                "/usr/lib/libx.so": {"mappings": 2, "Pss": 50, "Anonymous": 0, "Rss": 60, "Private_Dirty": 0, "Swap": 0},
            }
        }
        after = {
            "by_object": {
                "[heap]": {"mappings": 1, "Pss": 300, "Anonymous": 300, "Rss": 300, "Private_Dirty": 300, "Swap": 0},
                "/usr/lib/libx.so": {"mappings": 2, "Pss": 50, "Anonymous": 0, "Rss": 60, "Private_Dirty": 0, "Swap": 0},
                "[anon rw-p]": {"mappings": 3, "Pss": 40, "Anonymous": 40, "Rss": 40, "Private_Dirty": 40, "Swap": 0},
            }
        }
        table = analyze.mapping_changes(before, after)
        self.assertEqual(table["top"][0]["object"], "[heap]")
        self.assertEqual(table["top"][0]["delta"]["Pss"], 200)
        self.assertEqual(table["totals"]["delta"]["Pss"], 240)
        self.assertEqual(table["objects"], 3)


class SamplerParserTest(unittest.TestCase):
    def test_stat_handles_parentheses_in_the_command_name(self) -> None:
        text = "1234 (latte (dock)) S 1 1234 1234 0 -1 4194560 100 0 0 0 5 7 0 0 20 0 3 0 99999 1000000 500"
        stat = memsample.parse_stat(text)
        self.assertEqual(stat["comm"], "latte (dock)")
        self.assertEqual(stat["state"], "S")
        self.assertEqual((stat["utime"], stat["stime"], stat["starttime"]), (5, 7, 99999))

    def test_rollup_and_status(self) -> None:
        rollup = memsample.parse_rollup(
            "Rss:      100 kB\nPss:       80 kB\nPss_Anon:  60 kB\nPss_File:  20 kB\n"
            "Pss_Shmem:  0 kB\nPrivate_Dirty: 10 kB\nSwap:  0 kB\n"
        )
        self.assertEqual(rollup["Pss_Anon"], 60)
        self.assertEqual(rollup["Private_Dirty"], 10)
        status = memsample.parse_status(
            "Name:\tlatte\nState:\tS (sleeping)\nVmRSS:\t  1234 kB\nRssAnon:\t 1000 kB\n"
            "RssFile:\t 200 kB\nRssShmem:\t 34 kB\nVmData:\t 5000 kB\nThreads:\t 12\n"
        )
        self.assertEqual(status["VmRSS"], 1234)
        self.assertEqual(status["RssAnon"], 1000)
        self.assertEqual(status["Threads"], 12)

    def test_nvidia_smi_process_table_rows(self) -> None:
        # Rows copied from nvidia-smi's process table on the R8d host.
        kwin = "|    0   N/A  N/A            3863    C+G   /usr/bin/kwin_wayland                    74MiB |"
        dock = "|    0   N/A  N/A         1416075      G   ...home/.local/bin/latte-dock-ng         28MiB |"
        header = "| Processes:                                                                              |"
        match = memsample.GPU_PROCESS_LINE.match(kwin)
        self.assertEqual((match.group(1), match.group(2)), ("3863", "74"))
        match = memsample.GPU_PROCESS_LINE.match(dock)
        self.assertEqual((match.group(1), match.group(2)), ("1416075", "28"))
        self.assertIsNone(memsample.GPU_PROCESS_LINE.match(header))

    def test_smaps_groups_anonymous_mappings_per_permission_set(self) -> None:
        text = (
            "55d0c6d4b000-55d0c6d4c000 r--p 00000000 103:02 1234                     /usr/bin/foo\n"
            "Rss:                  8 kB\nPss:                  4 kB\nAnonymous:             0 kB\n"
            "Private_Dirty:        0 kB\nSwap:                  0 kB\n"
            "7f0000000000-7f0000001000 rw-p 00000000 00:00 0 \n"
            "Rss:                 40 kB\nPss:                 30 kB\nAnonymous:            40 kB\n"
            "Private_Dirty:       40 kB\nSwap:                  0 kB\n"
            "7f0000002000-7f0000003000 rw-p 00000000 00:00 0 \n"
            "Rss:                 12 kB\nPss:                 10 kB\nAnonymous:            12 kB\n"
            "Private_Dirty:       12 kB\nSwap:                  0 kB\n"
        )
        mappings = memsample.parse_smaps(text)
        self.assertEqual(len(mappings), 3)
        summary = memsample.summarize_smaps(mappings)
        self.assertEqual(summary["top"][0]["Pss"], 30)
        self.assertEqual(summary["by_object"]["[anon rw-p]"]["mappings"], 2)
        self.assertEqual(summary["by_object"]["[anon rw-p]"]["Pss"], 40)
        self.assertEqual(summary["by_object"]["/usr/bin/foo"]["Pss"], 4)


if __name__ == "__main__":
    unittest.main(verbosity=2)
