# R8j cycle analysis: scenario I

Run: `docs/modernization-results/R8j-data/I`. Session reason: completed. Samples: 251; signals: 15; probe lines: 17.

**Verdict: not reproduced**

G = A_10 - A_0 = 0.00 MiB; TR = A_10 - T = 0.00 MiB; dU (glibc in use) = 0.00 MiB; dF (glibc free) = 0.00 MiB; growing cycles = 0; late growth (cycles 4 to 10) = 0.00 MiB; cycles with a step of at least 1 MiB = 0.

## Levels (MiB, Pss_Anon, and glibc counters after the rest)

| cycle | action | A_k | step | peak | peak - previous level | threads peak | uordblks | fordblks |
|---:|---|---:|---:|---:|---:|---:|---:|---:|
| 0 | post-trim baseline | 115.25 | - | - | - | - | 83.76 | 3.60 |
| 1 | idle | 115.25 | 0.00 | 115.25 | 0.00 | 8 | 83.76 | 3.60 |
| 2 | idle | 115.25 | 0.00 | 115.25 | 0.00 | 8 | 83.76 | 3.60 |
| 3 | idle | 115.25 | 0.00 | 115.25 | 0.00 | 8 | 83.76 | 3.60 |
| 4 | idle | 115.25 | 0.00 | 115.25 | 0.00 | 8 | 83.76 | 3.60 |
| 5 | idle | 115.25 | 0.00 | 115.25 | 0.00 | 8 | 83.76 | 3.60 |
| 6 | idle | 115.25 | 0.00 | 115.25 | 0.00 | 8 | 83.76 | 3.60 |
| 7 | idle | 115.25 | 0.00 | 115.25 | 0.00 | 8 | 83.76 | 3.60 |
| 8 | idle | 115.25 | 0.00 | 115.25 | 0.00 | 8 | 83.76 | 3.60 |
| 9 | idle | 115.25 | 0.00 | 115.25 | 0.00 | 8 | 83.76 | 3.60 |
| 10 | idle | 115.25 | 0.00 | 115.25 | 0.00 | 8 | 83.76 | 3.60 |
| 11 (post-trim) | idle | 115.25 | 0.00 | 115.25 | 0.00 | 8 | 83.76 | 3.60 |
| 12 (post-trim) | idle | 115.25 | 0.00 | 115.25 | 0.00 | 8 | 83.76 | 3.60 |

## Trim

- Before the baseline trim (warm-up end): 117.49 MiB; after it: 115.25 MiB.
- After cycle 10: 115.25 MiB; after the trim: 115.25 MiB.
- Post-trim check (supporting, not in the rule): cycle 11 takes back 0.00 MiB and cycle 12 0.00 MiB of the trim release of 0.00 MiB.
