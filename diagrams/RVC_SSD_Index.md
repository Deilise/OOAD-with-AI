# Roomba RVC — System sequence diagram (SSD) gallery

Companion to **`RVC_SW_Controller_UseCases.md`**. Each use case has **one SSD** with **typical**, **alternative**, and **exceptional** courses in **`alt` / `else` / `loop`** frames (PlantUML).

**Domain model (middle layer):** [RVC_Domain_Diagram.md](RVC_Domain_Diagram.md)

**Sequence diagrams (SD):** [SD gallery](sd/RVC_SD_Index.md)

**Class diagram:** [RVC_Class_Diagram.md](class/RVC_Class_Diagram.md)

**Source:** `plantuml/UCxx_system_sequence.puml` · **Re-render:** `powershell -NoProfile -ExecutionPolicy Bypass -File .\diagrams\render-diagrams.ps1`

## By use case

| UC | Use case | Diagram |
|----|----------|---------|
| UC-01 | Start automatic cleaning session | [UC-01 SSD](ssd/UC-01_SSD.md) |
| UC-02 | Cruise forward while cleaning | [UC-02 SSD](ssd/UC-02_SSD.md) |
| UC-03 | Avoid obstacle (forward blocked, not surrounded) | [UC-03 SSD](ssd/UC-03_SSD.md) |
| UC-04 | Avoid obstacle when right turn is blocked | [UC-04 SSD](ssd/UC-04_SSD.md) |
| UC-05 | Escape when surrounded | [UC-05 SSD](ssd/UC-05_SSD.md) |
| UC-06 | Boost cleaning when dust is high | [UC-06 SSD](ssd/UC-06_SSD.md) |
| UC-07 | Resume forward cleaning after a maneuver | [UC-07 SSD](ssd/UC-07_SSD.md) |
| UC-08 | Handle missing / invalid / stale obstacle data | [UC-08 SSD](ssd/UC-08_SSD.md) |
| UC-09 | Build consistent fused obstacle picture | [UC-09 SSD](ssd/UC-09_SSD.md) |

## Frame legend (on diagrams)

| Label prefix | Meaning |
|--------------|---------|
| `[typical]` | Typical course of events |
| `[A1]`, `[A2]` | Alternative courses |
| `[E1]`, `[E2]`, … | Exceptional courses |

Cross–use-case handoffs appear in **notes** only (e.g. `next: UC-02`).
