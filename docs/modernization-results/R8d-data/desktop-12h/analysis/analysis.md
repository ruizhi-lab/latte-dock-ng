# R8d idle memory analysis

Warm-up dropped: the first 15 minutes of each series. Buckets: 5-minute medians of at least 5 samples. Slopes: least squares of bucket medians against hours; intervals are 95 % (Student t, n-2 degrees of freedom). Levels are bucket medians in MiB.

Verdict rule, `Pss_Anon` of the series: no growth when the whole-run slope interval includes 0 or its upper bound is below 1 MiB/h; plateau when the first-half slope exceeds 1 MiB/h and the second-half slope does not; otherwise growth.

## Series

| series | samples | span (h) | post-warm-up span (h) | mean CPU % | verdict |
|---|---:|---:|---:|---:|---|
| desktop-dock | 1441 | 12.00 | 11.75 | 0.07 | growth |

## Slopes (KiB/h) and levels (MiB)

| series | metric | window | buckets | slope KiB/h | 95 % CI KiB/h | slope MiB/h | first | last | min | max |
|---|---|---|---:|---:|---|---:|---:|---:|---:|---:|
| desktop-dock | Pss_Anon | whole | 141 | 11,144 | [9,645, 12,642] | 10.882 | 160.32 | 283.20 | 160.32 | 283.38 |
| desktop-dock | Pss_Anon | first half | 141 | 0 | [-0, 0] | 0.000 | - | - | - | - |
| desktop-dock | Pss_Anon | second half | 141 | 30,099 | [27,301, 32,896] | 29.393 | - | - | - | - |
| desktop-dock | Rss | whole | 141 | 12,470 | [10,764, 14,175] | 12.177 | 368.47 | 506.32 | 368.47 | 506.38 |
| desktop-dock | Rss | first half | 141 | 0 | [-0, 0] | 0.000 | - | - | - | - |
| desktop-dock | Rss | second half | 141 | 33,871 | [30,580, 37,163] | 33.077 | - | - | - | - |
| desktop-dock | Pss_File | whole | 141 | 633 | [503, 763] | 0.618 | 63.55 | 73.05 | 59.47 | 73.05 |
| desktop-dock | Pss_File | first half | 141 | 0 | [0, 0] | 0.000 | - | - | - | - |
| desktop-dock | Pss_File | second half | 141 | 2,031 | [1,668, 2,393] | 1.983 | - | - | - | - |

## Verdicts

- **desktop-dock**: growth. whole-run Pss_Anon (column Pss_Anon) slope 11,144 KiB/h (10.88 MiB/h), 95 % CI [9,645, 12,642] KiB/h; +100 MiB in 9.2 h at this slope

## GPU memory at the snapshots (nvidia-smi, MiB)

| series | 0 min | 15 min | 75 min | end |
|---|---:|---:|---:|---:|
| desktop-dock | 61 | 61 | 61 | 70 |

## Mappings: 15-minute snapshot to end

Per object (file path, or anonymous mappings of one permission set), largest absolute change in Pss first. Totals sum all objects. MiB.

### desktop-dock

Total Pss 223.46 -> 355.90 MiB (delta +132.44); Anonymous delta +122.88; Rss delta +137.86; 584 objects.

| object | mappings 15 min -> end | delta Pss | delta Anonymous | delta Rss |
|---|---:|---:|---:|---:|
| [heap] | 1 -> 1 | +78.27 | +78.27 | +78.27 |
| [anon rw-p] | 98 -> 104 | +39.98 | +39.98 | +39.98 |
| /dev/nvidiactl | 26 -> 27 | +7.01 | +0.00 | +7.01 |
| /memfd:JSGCHeap:QtQml (deleted) | 23 -> 42 | +4.02 | +4.02 | +4.02 |
| /home/ruizhi/.cache/plasma_theme_klassy-light.kcache | 1 -> 0 | -4.00 | +0.00 | -4.00 |
| /home/ruizhi/.cache/plasma_theme_klassy-light.kcache (deleted) | 0 -> 1 | +3.63 | +0.00 | +3.63 |
| /usr/share/fonts/sarasa-gothic/Sarasa-Regular.ttc | 1 -> 1 | +1.67 | +0.00 | +4.88 |
| /usr/lib64/libKF6BreezeIcons.so.6.31.0 | 5 -> 5 | +0.62 | +0.00 | +0.64 |
| /usr/share/fonts/noto/NotoSans-Regular.ttf | 0 -> 1 | +0.28 | +0.00 | +0.28 |
| /memfd:unknown-usage:QtQml (deleted) | 99 -> 169 | +0.27 | +0.27 | +0.27 |


## Sampler events

- nested: reason "duration reached"; wall 2026-11-09T11:58:44 to 2026-11-09T23:58:44; 1441 sample ticks; new latte-dock-ng coredumps: 0
