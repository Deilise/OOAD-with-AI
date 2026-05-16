# UC-01 — Start Automatic Cleaning Session (SD)

[← SD index](RVC_SD_Index.md) · [SSD index](../RVC_SSD_Index.md) · [Domain model](../RVC_Domain_Diagram.md) · Source: `sd/UC01_sequence.puml`

This sequence diagram specifies what happens inside the SSD black box using the domain-model objects:

- `AutomaticCleaningSession`
- `NavigationAndEscapeCoordinator`
- `SurfaceCleaningController`

It avoids showing internal self-processing inside each object; only object-to-object messages are shown.

**Frames:** `[typical start accepted]` · `[A1 start ignored]` · `[A2 resume after pause]` · `[E1 command lost]`

![UC-01 sequence diagram](UC01_sequence.png)
