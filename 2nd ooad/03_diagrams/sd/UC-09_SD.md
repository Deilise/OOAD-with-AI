# UC-09 — Build consistent fused obstacle picture (SD)

[← SD index](RVC_SD_Index.md) · [SSD index](../ssd/RVC_SSD_Index.md) · [Domain model](../domain/RVC_Domain_Diagram.md) · Source: `UC09_sequence.puml`

This sequence diagram shows **`ObstaclePerceptionContext`** fusing direct front/left/back updates and right-side probe-pose observations (probe sensor per **`travelToggle`**) into **`FusedObstacleSnapshot`** for navigation decisions.

**Frames:** `[typical Forward toggle fusion]` · `[typical Backward toggle fusion]` · surrounded classification (front/back probe) · conflicting readings

![UC-09 sequence diagram](UC09_sequence.png)
