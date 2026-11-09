# R8j cycle analysis: scenario A

Run: `docs/modernization-results/R8j-data/A`. Session reason: completed. Samples: 251; signals: 15; probe lines: 17.

**Verdict: inconclusive**

G = A_10 - A_0 = 5.54 MiB; TR = A_10 - T = 2.36 MiB; dU (glibc in use) = 0.81 MiB; dF (glibc free) = 4.40 MiB; growing cycles = 5; late growth (cycles 4 to 10) = -0.26 MiB; cycles with a step of at least 1 MiB = 1.

## Levels (MiB, Pss_Anon, and glibc counters after the rest)

| cycle | action | A_k | step | peak | peak - previous level | threads peak | uordblks | fordblks |
|---:|---|---:|---:|---:|---:|---:|---:|---:|
| 0 | post-trim baseline | 115.15 | - | - | - | - | 83.76 | 3.97 |
| 1 | application | 120.32 | 5.17 | 120.32 | 5.17 | 10 | 84.34 | 8.60 |
| 2 | application | 120.33 | 0.01 | 120.34 | 0.02 | 8 | 84.50 | 8.44 |
| 3 | application | 120.95 | 0.61 | 120.95 | 0.61 | 8 | 84.61 | 8.33 |
| 4 | application | 121.61 | 0.66 | 121.61 | 0.66 | 8 | 84.76 | 8.19 |
| 5 | application | 120.56 | -1.05 | 120.56 | -1.05 | 8 | 84.45 | 8.49 |
| 6 | application | 120.56 | 0.00 | 120.56 | 0.00 | 8 | 84.58 | 8.37 |
| 7 | application | 121.16 | 0.59 | 121.16 | 0.59 | 8 | 84.70 | 8.24 |
| 8 | application | 121.77 | 0.62 | 121.77 | 0.62 | 8 | 84.79 | 8.15 |
| 9 | application | 120.69 | -1.09 | 120.69 | -1.08 | 8 | 84.45 | 8.49 |
| 10 | application | 120.69 | 0.00 | 120.69 | 0.00 | 8 | 84.57 | 8.37 |
| 11 (post-trim) | application | 120.99 | 0.30 | 120.99 | 0.30 | 8 | 84.69 | 8.09 |
| 12 (post-trim) | application | 120.29 | -0.70 | 121.35 | 0.36 | 8 | 84.39 | 8.40 |

## Trim

- Before the baseline trim (warm-up end): 118.07 MiB; after it: 115.15 MiB.
- After cycle 10: 120.69 MiB; after the trim: 118.33 MiB.
- Post-trim check (supporting, not in the rule): cycle 11 takes back 2.66 MiB and cycle 12 1.96 MiB of the trim release of 2.36 MiB.
