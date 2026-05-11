# Executive presentation: Formal Verification for Firmware

Two Beamer decks, both centred on the F-1 finding in NVIDIA OpenSMA
(`verification/REPORT.md`). F-1 was
[confirmed and fixed upstream by NVIDIA's OpenSMA
team](https://github.com/NVIDIA/OpenSMA/issues/1#issuecomment-4417902007)
on 2026-05-11.

| Deck | File | Slides | Use |
|---|---|:-:|---|
| Executive | `fv_f1_executive.tex` / `.pdf` | 14 + 2 backup | 10–15 min walkthrough for firmware leadership |
| Summary   | `fv_f1_summary.tex` / `.pdf`   | 4 | Quick-scan link for social/blog/email |

## Audience

Senior technical leadership (potentially VP) with limited prior exposure to
formal verification. Optimised for a 10–15 minute walkthrough.

## Build

```sh
pdflatex fv_f1_executive.tex && pdflatex fv_f1_executive.tex   # 16 pages
pdflatex fv_f1_summary.tex   && pdflatex fv_f1_summary.tex     #  4 pages
```

Both decks are 16:9.

## Summary deck outline (`fv_f1_summary.tex`)

| # | Slide                                          |
|--:|------------------------------------------------|
| 1 | Title + report link                            |
| 2 | The bug — a four-line view                     |
| 3 | What ESBMC proved — in three states            |
| 4 | Outcome — confirmed and fixed upstream         |

## Executive deck outline (`fv_f1_executive.tex`)

| #  | Slide                                                             |
|---:|-------------------------------------------------------------------|
|  1 | Title                                                             |
|  2 | Executive summary                                                 |
|  3 | The F-1 bug — a four-line view                                    |
|  4 | Why traditional testing missed F-1                                |
|  5 | What is formal verification, in engineering terms?                |
|  6 | How ESBMC proved F-1 — in three states                            |
|  7 | Formal verification vs. traditional testing                       |
|  8 | The OpenSMA pilot at a glance                                     |
|  9 | Tangible value: ROI for firmware                                  |
| 10 | Risk reduction & strategic differentiation                        |
| 11 | Where formal verification fits in the firmware workflow           |
| 12 | Tooling investment & ecosystem position                           |
| 13 | Recommendation                                                    |
| 14 | Summary in three lines                                            |
| 15 | Backup — The validator gap, in numbers                            |
| 16 | Backup — Why ESBMC, specifically                                  |
