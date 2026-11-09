# R8j cycle analysis: scenario W

Run: `docs/modernization-results/R8j-data/W`. Session reason: completed. Samples: 251; signals: 15; probe lines: 17.

**Verdict: inconclusive**

G = A_10 - A_0 = 5.57 MiB; TR = A_10 - T = 1.91 MiB; dU (glibc in use) = 0.98 MiB; dF (glibc free) = 4.24 MiB; growing cycles = 5; late growth (cycles 4 to 10) = -0.05 MiB; cycles with a step of at least 1 MiB = 1.

## Levels (MiB, Pss_Anon, and glibc counters after the rest)

| cycle | action | A_k | step | peak | peak - previous level | threads peak | uordblks | fordblks |
|---:|---|---:|---:|---:|---:|---:|---:|---:|
| 0 | post-trim baseline | 115.05 | - | - | - | - | 83.69 | 3.47 |
| 1 | window | 119.95 | 4.91 | 119.95 | 4.91 | 10 | 84.15 | 8.22 |
| 2 | window | 120.03 | 0.08 | 120.03 | 0.08 | 10 | 84.43 | 7.94 |
| 3 | window | 120.67 | 0.64 | 120.67 | 0.64 | 10 | 84.69 | 7.68 |
| 4 | window | 121.37 | 0.70 | 121.37 | 0.70 | 10 | 84.84 | 7.53 |
| 5 | window | 120.39 | -0.98 | 120.39 | -0.98 | 10 | 84.52 | 7.84 |
| 6 | window | 120.42 | 0.02 | 120.42 | 0.03 | 9 | 84.66 | 7.72 |
| 7 | window | 120.96 | 0.55 | 120.96 | 0.55 | 10 | 84.78 | 7.60 |
| 8 | window | 121.59 | 0.62 | 121.59 | 0.62 | 10 | 84.87 | 7.51 |
| 9 | window | 120.62 | -0.97 | 120.62 | -0.97 | 10 | 84.53 | 7.85 |
| 10 | window | 120.62 | 0.00 | 120.62 | 0.00 | 10 | 84.67 | 7.71 |
| 11 (post-trim) | window | 120.71 | 0.09 | 120.71 | 0.09 | 9 | 84.52 | 8.07 |
| 12 (post-trim) | window | 120.12 | -0.59 | 121.11 | 0.41 | 10 | 84.25 | 8.34 |

## Trim

- Before the baseline trim (warm-up end): 117.38 MiB; after it: 115.05 MiB.
- After cycle 10: 120.62 MiB; after the trim: 118.71 MiB.
- Post-trim check (supporting, not in the rule): cycle 11 takes back 2.00 MiB and cycle 12 1.41 MiB of the trim release of 1.91 MiB.
