# UC-03 — Avoid obstacle (leading sector blocked, not surrounded) (SD)

[← SD index](RVC_SD_Index.md) · [SSD index](../ssd/RVC_SSD_Index.md) · [Domain model](../domain/RVC_Domain_Diagram.md) · Source: `UC03_sequence.puml`

This sequence diagram shows leading-sector blocking, right-side probe request/re-orientation (probe uses **front** sensor when Forward toggle, **back** when Backward), maneuver selection, cleaning suspension, and resume with **`MotionCommand(forwardOrReversePerToggle)`**.

**Frames:** `[E1 probe / sensor ambiguity]` · `[A2 multiple adjustments]` · `[single maneuver]` with prefer-right / left branches per toggle

![UC-03 sequence diagram](UC03_sequence.png)
