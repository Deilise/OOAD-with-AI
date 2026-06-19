# UC-08 — Handle missing / invalid / stale obstacle or probe data (SD)

[← SD index](RVC_SD_Index.md) · [SSD index](../ssd/RVC_SSD_Index.md) · [Domain model](../domain/RVC_Domain_Diagram.md) · Source: `UC08_sequence.puml`

This sequence diagram shows invalid or stale **front**, **back**, **leading-sector**, or **probe-pose** observations flowing to safe stop/suspend behavior, with recovery resuming per **`travelToggle`**.

**Frames:** missing front (Forward) · missing back (Backward) · stale leading sector · stale probe (front/back) · data recovers · persistent invalid

![UC-08 sequence diagram](UC08_sequence.png)
