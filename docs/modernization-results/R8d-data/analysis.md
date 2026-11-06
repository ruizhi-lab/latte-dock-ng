# R8d idle memory analysis

Warm-up dropped: the first 15 minutes of each series. Buckets: 5-minute medians of at least 5 samples. Slopes: least squares of bucket medians against hours; intervals are 95 % (Student t, n-2 degrees of freedom). Levels are bucket medians in MiB.

Verdict rule, `Pss_Anon` of the series: no growth when the whole-run slope interval includes 0 or its upper bound is below 1 MiB/h; plateau when the first-half slope exceeds 1 MiB/h and the second-half slope does not; otherwise growth.

## Series

| series | samples | span (h) | post-warm-up span (h) | mean CPU % | verdict |
|---|---:|---:|---:|---:|---|
| desktop-dock | 301 | 2.50 | 2.25 | 0.24 | no growth |
| nested-dock | 301 | 2.50 | 2.25 | 0.00 | no growth |
| nested-kwin | 301 | 2.50 | 2.25 | 0.16 | no growth |

## Slopes (KiB/h) and levels (MiB)

| series | metric | window | buckets | slope KiB/h | 95 % CI KiB/h | slope MiB/h | first | last | min | max |
|---|---|---|---:|---:|---|---:|---:|---:|---:|---:|
| desktop-dock | Pss_Anon | whole | 27 | 704 | [454, 954] | 0.687 | 416.71 | 419.16 | 416.71 | 419.16 |
| desktop-dock | Pss_Anon | first half | 27 | 920 | [318, 1,522] | 0.898 | - | - | - | - |
| desktop-dock | Pss_Anon | second half | 27 | 1,431 | [719, 2,142] | 1.397 | - | - | - | - |
| desktop-dock | Rss | whole | 27 | 704 | [454, 954] | 0.687 | 695.21 | 697.66 | 695.21 | 697.66 |
| desktop-dock | Rss | first half | 27 | 920 | [318, 1,522] | 0.898 | - | - | - | - |
| desktop-dock | Rss | second half | 27 | 1,431 | [719, 2,142] | 1.397 | - | - | - | - |
| desktop-dock | Pss_File | whole | 27 | 77 | [21, 133] | 0.076 | 57.32 | 57.70 | 57.32 | 57.70 |
| desktop-dock | Pss_File | first half | 27 | 294 | [75, 514] | 0.288 | - | - | - | - |
| desktop-dock | Pss_File | second half | 27 | 0 | [-0, 1] | 0.000 | - | - | - | - |
| nested-dock | Pss_Anon | whole | 27 | 0 | [0, 0] | 0.000 | 71.48 | 71.48 | 71.48 | 71.48 |
| nested-dock | Pss_Anon | first half | 27 | 0 | [0, 0] | 0.000 | - | - | - | - |
| nested-dock | Pss_Anon | second half | 27 | 0 | [0, 0] | 0.000 | - | - | - | - |
| nested-dock | Rss | whole | 27 | 0 | [0, 0] | 0.000 | 272.35 | 272.35 | 272.35 | 272.35 |
| nested-dock | Rss | first half | 27 | 0 | [0, 0] | 0.000 | - | - | - | - |
| nested-dock | Rss | second half | 27 | 0 | [0, 0] | 0.000 | - | - | - | - |
| nested-dock | Pss_File | whole | 27 | 78 | [19, 136] | 0.076 | 30.47 | 30.87 | 30.47 | 30.87 |
| nested-dock | Pss_File | first half | 27 | 300 | [66, 535] | 0.293 | - | - | - | - |
| nested-dock | Pss_File | second half | 27 | 0 | [0, 0] | 0.000 | - | - | - | - |
| nested-kwin | Pss_Anon (from RssAnon) | whole | 27 | 0 | [0, 0] | 0.000 | 77.32 | 77.32 | 77.32 | 77.32 |
| nested-kwin | Pss_Anon (from RssAnon) | first half | 27 | 0 | [0, 0] | 0.000 | - | - | - | - |
| nested-kwin | Pss_Anon (from RssAnon) | second half | 27 | 0 | [0, 0] | 0.000 | - | - | - | - |
| nested-kwin | Rss (from VmRSS) | whole | 27 | 0 | [0, 0] | 0.000 | 362.81 | 362.81 | 362.81 | 362.81 |
| nested-kwin | Rss (from VmRSS) | first half | 27 | 0 | [0, 0] | 0.000 | - | - | - | - |
| nested-kwin | Rss (from VmRSS) | second half | 27 | 0 | [0, 0] | 0.000 | - | - | - | - |
| nested-kwin | Pss_File (from RssFile) | whole | 27 | 0 | [0, 0] | 0.000 | 227.12 | 227.12 | 227.12 | 227.12 |
| nested-kwin | Pss_File (from RssFile) | first half | 27 | 0 | [0, 0] | 0.000 | - | - | - | - |
| nested-kwin | Pss_File (from RssFile) | second half | 27 | 0 | [0, 0] | 0.000 | - | - | - | - |

## Verdicts

- **desktop-dock**: no growth. whole-run Pss_Anon (column Pss_Anon) slope 704 KiB/h (0.69 MiB/h), 95 % CI [454, 954] KiB/h
- **nested-dock**: no growth. whole-run Pss_Anon (column Pss_Anon) slope 0 KiB/h (0.00 MiB/h), 95 % CI [0, 0] KiB/h
- **nested-kwin**: no growth. whole-run Pss_Anon (column RssAnon) slope 0 KiB/h (0.00 MiB/h), 95 % CI [0, 0] KiB/h

## GPU memory at the snapshots (nvidia-smi, MiB)

| series | 0 min | 15 min | 75 min | end |
|---|---:|---:|---:|---:|
| desktop-dock | 204 | 206 | 199 | 200 |
| nested-dock | 0 | 28 | 28 | 28 |
| nested-kwin | 29 | 29 | 29 | 29 |

## Mappings: 15-minute snapshot to end

Per object (file path, or anonymous mappings of one permission set), largest absolute change in Pss first. Totals sum all objects. MiB.

### desktop-dock

Total Pss 502.53 -> 505.37 MiB (delta +2.84); Anonymous delta +2.45; Rss delta +2.45; 594 objects.

| object | mappings 15 min -> end | delta Pss | delta Anonymous | delta Rss |
|---|---:|---:|---:|---:|
| [heap] | 1 -> 1 | +1.32 | +1.32 | +1.32 |
| [anon rw-p] | 107 -> 107 | +0.81 | +0.81 | +0.81 |
| /memfd:JSGCHeap:QtQml (deleted) | 31 -> 27 | +0.31 | +0.31 | +0.31 |
| /usr/lib64/libnvidia-gpucomp.so.595.104.02 | 5 -> 5 | +0.07 | +0.00 | +0.00 |
| /usr/lib64/libnvidia-glcore.so.595.104.02 | 8 -> 8 | +0.05 | +0.00 | +0.00 |
| /usr/lib64/libQt6Core.so.6.11.2 | 5 -> 5 | +0.02 | +0.00 | +0.00 |
| /usr/lib64/libQt6Gui.so.6.11.2 | 5 -> 5 | +0.02 | +0.00 | +0.00 |
| /usr/lib64/libQt6Quick.so.6.11.2 | 5 -> 5 | +0.02 | +0.00 | +0.00 |
| /run/user/1000/cache/ksycoca6_zh_efhnsPHbmVa5cgn6MGZm5gcvZ30= | 1 -> 1 | +0.02 | +0.00 | +0.00 |
| /usr/lib64/libKF6Service.so.6.29.0 | 5 -> 5 | +0.01 | +0.00 | +0.00 |

### nested-dock

Total Pss 124.66 -> 125.06 MiB (delta +0.40); Anonymous delta +0.00; Rss delta +0.00; 560 objects.

| object | mappings 15 min -> end | delta Pss | delta Anonymous | delta Rss |
|---|---:|---:|---:|---:|
| /usr/lib64/libnvidia-gpucomp.so.595.104.02 | 5 -> 5 | +0.08 | +0.00 | +0.00 |
| /usr/lib64/libnvidia-glcore.so.595.104.02 | 8 -> 8 | +0.05 | +0.00 | +0.00 |
| /usr/lib64/libQt6Core.so.6.11.2 | 5 -> 5 | +0.02 | +0.00 | +0.00 |
| /usr/lib64/libQt6Gui.so.6.11.2 | 5 -> 5 | +0.02 | +0.00 | +0.00 |
| /usr/lib64/libQt6Quick.so.6.11.2 | 5 -> 5 | +0.02 | +0.00 | +0.00 |
| /usr/lib64/libQt6Qml.so.6.11.2 | 5 -> 5 | +0.01 | +0.00 | +0.00 |
| /usr/lib64/libKF6Service.so.6.29.0 | 5 -> 5 | +0.01 | +0.00 | +0.00 |
| /usr/lib64/libQt6Widgets.so.6.11.2 | 5 -> 5 | +0.01 | +0.00 | +0.00 |
| /usr/lib64/libQt6WaylandClient.so.6.11.2 | 5 -> 5 | +0.01 | +0.00 | +0.00 |
| /usr/lib64/libQt6QuickTemplates2.so.6.11.2 | 5 -> 5 | +0.01 | +0.00 | +0.00 |

- nested-kwin: no mapping table (smaps is not readable by this user (non-dumpable process))

## Sampler events

- nested: reason "duration reached"; wall 2026-11-06T10:06:31 to 2026-11-06T12:36:35; 301 sample ticks; new latte-dock-ng coredumps: 0
- desktop: reason "duration reached"; wall 2026-11-06T10:06:31 to 2026-11-06T12:36:31; 301 sample ticks; new latte-dock-ng coredumps: 0
