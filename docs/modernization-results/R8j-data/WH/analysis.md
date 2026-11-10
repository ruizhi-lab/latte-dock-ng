# R8j cycle analysis: scenario W

Run: `/data/projects/latte-dock-ng/.claude/worktrees/sweet-boyd-c68e77/docs/modernization-results/R8j-data/WH`. Session reason: completed. Samples: 251; signals: 15; probe lines: 17.

**Verdict: outside malloc**

G = A_10 - A_0 = 8.46 MiB; TR = A_10 - T = 1.73 MiB; dU (glibc in use) = 3.96 MiB; dF (glibc free) = 3.67 MiB; growing cycles = 5; late growth (cycles 4 to 10) = 0.40 MiB; cycles with a step of at least 1 MiB = 3.

## Levels (MiB, Pss_Anon, and glibc counters after the rest)

| cycle | action | A_k | step | peak | peak - previous level | threads peak | uordblks | fordblks |
|---:|---|---:|---:|---:|---:|---:|---:|---:|
| 0 | post-trim baseline | 172.56 | - | - | - | - | 137.05 | 2.77 |
| 1 | window | 178.77 | 6.20 | 178.93 | 6.37 | 10 | 139.72 | 5.31 |
| 2 | window | 178.83 | 0.06 | 179.00 | 0.23 | 10 | 140.38 | 4.67 |
| 3 | window | 180.62 | 1.79 | 180.79 | 1.96 | 10 | 140.80 | 6.63 |
| 4 | window | 181.51 | 0.89 | 181.68 | 1.06 | 10 | 141.15 | 6.29 |
| 5 | window | 180.65 | -0.86 | 180.82 | -0.69 | 10 | 140.86 | 6.60 |
| 6 | window | 180.68 | 0.03 | 180.85 | 0.20 | 10 | 141.00 | 6.46 |
| 7 | window | 181.20 | 0.51 | 181.36 | 0.68 | 10 | 141.11 | 6.35 |
| 8 | window | 181.92 | 0.72 | 182.08 | 0.89 | 10 | 141.21 | 6.25 |
| 9 | window | 181.02 | -0.90 | 181.19 | -0.73 | 10 | 140.89 | 6.57 |
| 10 | window | 181.02 | 0.00 | 181.19 | 0.17 | 10 | 141.01 | 6.45 |
| 11 (post-trim) | window | 181.38 | 0.36 | 181.55 | 0.53 | 10 | 140.88 | 6.96 |
| 12 (post-trim) | window | 182.02 | 0.64 | 182.19 | 0.80 | 10 | 141.06 | 6.78 |

## Trim

- Before the baseline trim (warm-up end): 173.43 MiB; after it: 172.56 MiB.
- After cycle 10: 181.02 MiB; after the trim: 179.29 MiB.
- Post-trim check (supporting, not in the rule): cycle 11 takes back 2.09 MiB and cycle 12 2.73 MiB of the trim release of 1.73 MiB.
