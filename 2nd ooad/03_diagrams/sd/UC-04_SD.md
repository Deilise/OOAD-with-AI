# UC-04 — Avoid obstacle when right turn is blocked (SD)

[← SD index](RVC_SD_Index.md) · [SSD index](../ssd/RVC_SSD_Index.md) · [Domain model](../domain/RVC_Domain_Diagram.md) · Source: `UC04_sequence.puml`

This sequence diagram shows the inside behavior when the right-side probe finds right not viable and the coordinator turns left. Resume uses **`forward`** or **`reverse`** per preserved **`travelToggle`**.

**Frames:** `[typical turn left Forward toggle]` · `[typical turn left Backward toggle]` · `[A1 neither lateral viable]` · `[A2 right-side probe cannot complete]` · `[E1 oscillation left and right]`

![UC-04 sequence diagram](UC04_sequence.png)
