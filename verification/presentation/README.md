# Executive presentation: Formal Verification for Firmware

A 14-slide Beamer deck (plus 2 backup slides) briefing NVIDIA firmware
leadership on the value of formal verification, using the confirmed F-1
finding from `verification/REPORT.md` as the motivating example.

## Audience

Senior technical leadership (potentially VP) with limited prior exposure to
formal verification. Optimised for a 10–15 minute walkthrough.

## Build

```sh
pdflatex fv_f1_executive.tex
pdflatex fv_f1_executive.tex   # second pass for cross-references
```

Output: `fv_f1_executive.pdf` (16 pages, 16:9).

## Outline

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
