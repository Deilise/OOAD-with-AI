# Roomba RVC — Use cases

Companion to **`RVC_SW_Controller_SRS.md`** (IEEE 830–style SRS, **v0.5.6**). Each use case below uses the same section layout so you can drop them into reports or trace matrices as-is.

### System boundary (important for actors)

| | |
|--|--|
| **System under design** | **Roomba automatic cleaning control software** — the embedded logic that reads sensor-related inputs, decides maneuvers and cleaning level, and sends **high-level** motion/cleaning commands (as in the SRS). |
| **Actors** | People or **physical / external subsystems outside that software** that supply stimuli or consume commands: user, schedule source, **obstacle sensors**, **dust sensor**, **drive / wheel motors**, **vacuum / brush / mop hardware**, etc. |
| **Not actors** | SRS **objects** such as `NavigationAndEscapeCoordinator` or `SurfaceCleaningController` are **internal classes/modules** — they belong in **class** and **sequence** diagrams **inside** the system rectangle, **not** in the **Actor** field of a use case. |

### Three-layer architecture (how this document fits)

Your stack: **UI layer (top)** → **application / domain layer — “our layer” (middle)** → **OS / platform layer (bottom)**.

| Layer | Role w.r.t. these use cases |
|--------|-----------------------------|
| **UI layer** | Presents **start / stop / status** (and any fault annunciation). **Primary human interaction** for **UC-01** lives here. UI forwards user intent **down** to the middle layer; it does **not** implement obstacle fusion or navigation policy. |
| **Our layer (middle)** | This is the **System** in this document: session state, obstacle fusion (**UC-09**), navigation and escape (**UC-02**–**UC-05**, **UC-07**), cleaning policy and dust boost (**UC-06**), safe degradation (**UC-08**). **Shall** requirements in the SRS target this layer. |
| **OS / platform layer (bottom)** | Timers, scheduling, drivers, ISRs, IPC, HAL: delivers **sensor samples** up and carries **motor / cleaning commands** down. Use case **actors** such as **obstacle sensor subsystem** and **drive motors** are realized **through** this layer from the middle layer’s point of view (middle calls **abstractions** that the OS layer implements). |

**Suitability:** Yes. Treat each use case’s **Typical Courses** as **middle-layer** behavior; map **UI** only where user intent or notifications appear (**UC-01**, optional steps in **UC-08**); map **sensors / motors / cleaning hardware** to **OS + drivers + HAL** in design and sequence diagrams, not as C++ classes in the middle of business logic.

In **Typical Courses of Events**, the word **System** means the software product above (unless a step explicitly names hardware).  

**Field legend**

| Field | Meaning |
|--------|---------|
| **Use Case Name** | Short title |
| **Actor** | **External** people or subsystems **outside** the cleaning control software that interact with it (UML-style actors) |
| **Purpose** | Business / technical goal |
| **Overview** | One-paragraph summary |
| **Cross Reference** | Pointers into the SRS (objects in §3.2, plus legacy FR/NFR where useful) |
| **Pre-Requisites** | What must already be true |
| **Typical Courses of Events** | Happy-path step sequence |
| **Alternative Courses of Events** | Valid branches (not failures) |
| **Exceptional Courses of Events** | Errors, timeouts, unsafe conditions |

---

## UC-01 — Start automatic cleaning session

**Use Case Name**  
Start automatic cleaning session  

**Actor**  
- **Primary:** **Home user** (starts cleaning from UI or physical control — product-defined).  
- **Primary (alternate):** **Scheduler / clock** (if the product supports timed start — same intent as user start).  
- **Supporting:** **UI / panel firmware** (delivers start/stop intent into software — optional actor if you model UI separately from user).  
*(The cleaning control **software** is the **System**, not an actor.)*  

**Purpose**  
Put the robot into **automatic cleaning** mode so vacuum/mop navigation behaviors are allowed to run. *(Achieved by the **System** once the session is active.)*  

**Overview**  
The user or an automated schedule requests a cleaning session. The **System** marks the session **active** and enables the autonomous cleaning behavior chain (cruise, obstacle handling, dust boost) until the session is stopped.  

**Cross Reference**  
SRS §3.2.1 **AutomaticCleaningSession** (OBJ1-FR-1, OBJ1-FR-2); §3.2.2 **SurfaceCleaningController** OBJ2-FR-1 (cleaning enabled once session is active — legacy **FR-001**); §3.1.1 UI-1, UI-2. Legacy: FR-001, FR-015, FR-016.  

**Pre-Requisites**  
- Robot is powered and not in a factory-locked state that forbids cleaning (**TBD** product rule).  
- Session **stop** is not asserted.  

**Typical Courses of Events**  
1. **Home user** (or **Scheduler**) issues start-cleaning intent (button, app, schedule — product-defined).  
2. **System** records session **active** and enables autonomous cleaning + navigation policy per SRS §2.2.  
3. Use case ends successfully; normal operation continues (e.g. UC-02).  

**Alternative Courses of Events**  
- **A1. Start ignored:** Session start is ignored if a higher-priority safety interlock is active (**TBD**); document outcome in product SRS addendum.  
- **A2. Resume after pause:** If “pause” exists as a product feature, resume path **TBD**—may re-use same use case with different pre-requisite.  

**Exceptional Courses of Events**  
- **E1. Command lost:** If start command is not acknowledged by firmware, behavior **TBD** (retry, error LED, etc.)—outside current SRS unless specified.  

---

## UC-02 — Cruise forward while cleaning

**Use Case Name**  
Cruise forward while cleaning  

**Actor**  
- **Primary:** **Home user** (beneficiary — wants floor coverage; implicitly “in play” once UC-01 has started a session).  
- **Supporting:** **Front / left / right obstacle sensors** (provide obstacle state the **System** reads).  
- **Supporting:** **Drive motors / wheel subsystem** (receives forward / turn / reverse commands from the **System**).  
- **Supporting:** **Cleaning hardware** — **vacuum motor**, **main brush**, **side brush**, **mop actuator** (as applicable; receive on/off / intensity from the **System**).  

**Purpose**  
Cover floor area by **driving straight forward** with **cleaning active** when nothing else demands a maneuver.  

**Overview**  
With an active session and no obstacle or surrounded condition, the **System** commands forward motion and keeps cleaning on. This is the default “happy path” between events.  

**Cross Reference**  
SRS §3.2.4 **NavigationAndEscapeCoordinator** OBJ4-FR-1; §3.2.2 **SurfaceCleaningController** OBJ2-FR-1. Legacy: FR-001, FR-003.  

**Pre-Requisites**  
- UC-01 (session active) **or** equivalent `sessionActive` **true**.  
- No maneuver in progress; fused obstacle state allows cruise: **OBJ4-FR-4** (surrounded) is **false** per policy, and forward is clear enough that **OBJ4-FR-1** applies (not in forward-blocked avoidance per **OBJ4-FR-3**).  
- Cleaning not intentionally suspended for another use case.  

**Typical Courses of Events**  
1. **Obstacle sensors** continue to supply readings; **System** evaluates fused obstacle state: forward path **clear**.  
2. **System** commands **straight forward** to **drive motors**.  
3. **System** keeps **cleaning hardware** active at normal power (OBJ2-FR-1).  
4. Loop continues until obstacle, surrounded, session stop, or sensor fault use case applies.  

**Alternative Courses of Events**  
- **A1. Dust boost concurrent:** While cruising, **UC-06** may raise power level; motion remains forward unless obstacle logic preempts.  

**Exceptional Courses of Events**  
- **E1. Stale / missing obstacle data:** See **UC-08**.  

---

## UC-03 — Avoid obstacle when forward is blocked (not surrounded)

**Use Case Name**  
Avoid obstacle when forward is blocked (not surrounded)  

**Actor**  
- **Primary:** **Obstacle sensor subsystem** — **front**, **left**, and **right** obstacle sensing (physical or opto-mechanical + electronics) that reports “something close” so the **System** reacts.  
- **Supporting:** **Drive motors / wheel subsystem** (executes turn and forward commands).  
- **Supporting:** **Cleaning hardware** (vacuum / brushes / mop — **System** suspends then resumes cleaning).  
- **Supporting:** **Home user** (beneficiary; not pressing stop during maneuver).  

**Purpose**  
When **forward** is blocked but the robot is **not** fully surrounded, maneuver around the obstacle **preferring a right turn**, then resume forward cleaning.  

**Overview**  
Cleaning is suspended for the maneuver. The robot executes a lateral avoidance (right if valid, else left), then drives forward again with cleaning resumed.  

**Cross Reference**  
SRS §3.2.4 OBJ4-FR-3; §3.2.2 OBJ2-FR-2, OBJ2-FR-3; §3.2.3 OBJ3-FR-1–OBJ3-FR-3. Legacy: FR-002, FR-005, FR-006, FR-007, FR-017.  

**Pre-Requisites**  
- Session active (`sessionActive` **true**).  
- Fused state: **front** obstacle / unsafe forward; **not** simultaneous front+left+right surrounded per OBJ4-FR-4.  

**Typical Courses of Events**  
1. **Obstacle sensors** indicate forward blocked; **System** classifies situation as **not** surrounded.  
2. **System** commands **cleaning hardware** to **suspend** cleaning for the maneuver.  
3. **System** plans lateral turn: **prefer right** if sensor-valid (OBJ4-FR-3).  
4. **System** commands **drive motors** to perform **right** turn; **obstacle sensors** update; when forward is safe, **System** commands **forward** and **resumes** **cleaning hardware**.  
5. Use case ends; flow returns to **UC-02**.  

**Alternative Courses of Events**  
- **A1. Prefer right invalid:** At step 3, if **right** turn is not sensor-valid, **System** executes **left** turn instead; then continues from step 4.  
- **A2. Multiple small adjustments:** If forward clears briefly then blocks again, sequence may repeat; debounce per SRS PERF-3 / design **TBD**.  

**Exceptional Courses of Events**  
- **E1. Sensor ambiguity:** If fusion cannot classify surrounded vs not-surrounded, product-defined safe policy **TBD** (may treat as UC-05 or UC-08).  

---

## UC-04 — Avoid obstacle when right turn is blocked

**Use Case Name**  
Avoid obstacle when right turn is blocked  

**Actor**  
- **Primary:** **Obstacle sensor subsystem** (readings must show **right** side does not allow a safe right turn, while left still allows avoidance — same physical actors as UC-03).  
- **Supporting:** **Drive motors / wheel subsystem**; **Cleaning hardware**; **Home user** (beneficiary).  

**Purpose**  
Same as UC-03, but validates that **prefer-right** is overridden when **right** is not a viable turn.  

**Overview**  
Prefer-right is a **default bias**, not a hard rule. When the right side does not support a safe right turn, the **System** chooses a **left** turn and completes the avoidance.  

**Cross Reference**  
SRS §3.2.4 OBJ4-FR-3 (prefer-right clause); SRS **Prefer-right** definition §1.3. Legacy: FR-006.  

**Pre-Requisites**  
- Same as UC-03.  
- Additionally: fused state indicates **right** turn is **not** sensor-valid (e.g., right obstacle tight), while **left** remains viable (**TBD** clearance rules).  

**Typical Courses of Events**  
1–2. Same as UC-03 steps 1–2 (**System** suspends **cleaning hardware**).  
3. **System** evaluates right turn: **not** valid.  
4. **System** commands **drive motors** for **left** turn.  
5. When **obstacle sensors** show forward safe: **System** commands forward + resumes **cleaning hardware**.  
6. Use case ends; **UC-02**.  

**Alternative Courses of Events**  
- **A1. Neither lateral turn viable but not “surrounded”:** Behavior **TBD** (may degenerate to small backup, re-scan, or escalate toward UC-05 per product policy).  

**Exceptional Courses of Events**  
- **E1. Oscillation between left/right:** Mitigated by debounce (SRS **PERF-3**) and product-defined maneuver limits **TBD** (see also **OBJ4-FR-7** — no contradictory motion commands).  

---

## UC-05 — Escape when surrounded (front, left, right blocked)

**Use Case Name**  
Escape when surrounded (front, left, right blocked)  

**Actor**  
- **Primary:** **Obstacle sensor subsystem** (front + left + right simultaneously indicate close obstacles — “surrounded” pattern).  
- **Supporting:** **Drive motors / wheel subsystem** (reverse, then turn / forward); **Cleaning hardware** (may stay suspended during escape per policy **TBD**); **Home user** (beneficiary).  

**Purpose**  
When **forward**, **left**, and **right** all indicate blockage, **reverse** until **at least one lateral side** (**left** or **right**) opens enough to **start a turn** (SRS **Lateral opening**), then perform a **prefer-right** lateral escape and resume forward cleaning when safe. **Forward** alone reading clear **does not** stop the surrounded-escape reverse **segment** while **both** sides are still “closed” for a lateral start. Respect **maximum backup** and **fallback** if no lateral opening appears.  

**Overview**  
The robot does not command forward while surrounded. It backs until **left** or **right** (or both) show a **lateral opening**, then turns (preferring right when both qualify), then goes forward again when fusion allows. If nothing opens laterally before the backup limit, **fallback** applies (**TBD**).  

**Why reverse (and why this is not “wiggle forever”)**  
- **Reverse** is used because **forward and both sides** read blocked at entry (**OBJ4-FR-4**): there is no safe move from that pose. Moving **backward** changes geometry so **left** or **right** can become open first — that **lateral opening** is the intended **normal exit** from the reverse segment (**OBJ4-FR-5**).  
- **Forward-only** “clear” along a **long tube** is **ignored** for ending reverse while **both** sides still block a lateral start, so the robot does **not** immediately charge forward down the same blind channel (**SRS OBJ4-FR-5**).  
- **Maximum backup** + **fallback** and **debouncing** (**PERF-3**) still cap pathological cases; **§2.6** lists **global** aids (mapping, stuck heuristics, odometry) as **TBD** for harder traps.  

**Cross Reference**  
SRS §1.3 **Lateral opening**; §2.2 item 4; §2.5 **A-5**; §2.6; §3.2.4 OBJ4-FR-4, OBJ4-FR-5, OBJ4-FR-6; §3.3 PERF-*; §3.5 SAFE-1. Legacy: FR-008, FR-009, FR-010, NFR-003.  

**Pre-Requisites**  
- Session active.  
- Fused obstacle state satisfies **surrounded** condition (OBJ4-FR-4).  

**Typical Courses of Events**  
1. **Obstacle sensors** report surrounded pattern; **System** ensures **no forward** command to **drive motors**.  
2. **System** commands **reverse** to **drive motors**; reads **obstacle sensors** while reversing.  
3. **During** the **surrounded-escape reverse segment** (entered when step 1 held), **System** **continues reverse** until a **lateral opening** (**left** and/or **right** per fusion — **TBD** thresholds) **or** **maximum backup** is reached (**OBJ4-FR-5**). If **forward** reads clear but **neither** side shows a lateral opening yet, **System** **keeps reversing** (same segment — forward-only clear does **not** end it while both sides are still “closed” for a lateral start, even if the strict **front∧left∧right** pattern momentarily breaks **only** because **forward** cleared).  
4. **Lateral opening:** **System** executes **prefer-right** lateral turn via **drive motors** (**OBJ4-FR-6**): both sides open → prefer **right** if valid else **left**; only one side open → turn **toward** that side.  
5. When forward is safe after the turn, **System** resumes **forward** + **cleaning hardware** → **UC-02**.  
6. Use case ends successfully.  

**Alternative Courses of Events**  
- **A1. Prefer right blocked when both sides open:** If **right** is not sensor-valid for the turn, **System** turns **left** (same spirit as **UC-04**).  
- **A2. Intermittent surrounded / sensor flicker:** Debounce per **PERF-3**; typical course may repeat until stable **lateral opening** or **fallback**.  

**Exceptional Courses of Events**  
- **E1. Max backup reached without lateral opening:** **Fallback** **TBD** (stop, alert, brief forward probe, etc.) — **OBJ4-FR-5**, **NFR-003** intent. **Forward-only** clear at this point does **not** by itself satisfy normal exit; **fallback** defines what happens next.  
- **E2. Sensor dropout during reverse:** **UC-08**.  
- **E3. Residual symmetric trap:** Fusion never reports a valid **lateral opening** before limits — still possible; see SRS **§2.6** (**TBD** global mitigations).  

---

## UC-06 — Boost cleaning when dust is high

**Use Case Name**  
Boost cleaning when dust is high  

**Actor**  
- **Primary:** **Dust / debris sensor** (or equivalent sensing path — product **TBD**) that reports elevated dirt load to the **System**.  
- **Supporting:** **Cleaning hardware** (**vacuum motor**, **brushes**, **mop** — receives higher power / duty commands).  
- **Supporting:** **Home user** (beneficiary).  

**Purpose**  
Temporarily **increase cleaning power** when debris/dust load exceeds a threshold, for a **bounded** duration or until the signal clears per policy.  

**Overview**  
Dust signal crosses threshold → **System** elevates power to **cleaning hardware** → return to normal when conditions end, unless still above threshold.  

**Cross Reference**  
SRS §3.2.2 OBJ2-FR-4, OBJ2-FR-5; §3.1.2 HI-2. Legacy: FR-011, FR-012, FR-013.  

**Pre-Requisites**  
- Session active.  
- Valid `dustSignal` input and configured threshold (**TBD**).  

**Typical Courses of Events**  
1. **Dust / debris sensor** reports level **above** threshold to **System**.  
2. **System** commands **increased cleaning power** on **cleaning hardware** (OBJ2-FR-4).  
3. Boost persists for **bounded** time **or** until signal below hysteresis per policy **TBD**.  
4. **System** returns **cleaning hardware** to **normal** power if signal no longer demands boost (OBJ2-FR-5).  
5. Use case ends; motion may continue per **UC-02** or concurrent maneuvers.  

**Alternative Courses of Events**  
- **A1. Boost during maneuver:** If cleaning is suspended for UC-03/UC-05, boost may be **deferred** or **ignored** per product policy **TBD** (not fixed in current SRS).  

**Exceptional Courses of Events**  
- **E1. Invalid dust signal:** Safe handling **TBD** (treat as no boost or UC-08).  

---

## UC-07 — Resume forward cleaning after a maneuver

**Use Case Name**  
Resume forward cleaning after a maneuver  

**Actor**  
- **Primary:** **Obstacle sensor subsystem** (reports that **forward** is again **clear** / safe per fusion rules).  
- **Supporting:** **Drive motors / wheel subsystem** (execute forward command).  
- **Supporting:** **Cleaning hardware** (resume or maintain cleaning on).  
- **Supporting:** **Home user** (beneficiary).  

**Purpose**  
After a turn or backup escape completes and the path ahead is viable, return to **forward** travel with **cleaning** on—not remain in maneuver state indefinitely.  

**Overview**  
This use case often appears as the **tail** of UC-03, UC-04, or UC-05: once **obstacle sensors** and fusion say the path is safe, the **System** prefers forward progress and normal cleaning.  

**Cross Reference**  
SRS §3.2.4 OBJ4-FR-2, OBJ4-FR-3, OBJ4-FR-6; §3.2.2 OBJ2-FR-1. Legacy: FR-001, FR-004, FR-007.  

**Pre-Requisites**  
- Session active.  
- A maneuver (avoidance or surrounded escape) has completed or forward clearance is granted per fusion **TBD**.  
- Forward path evaluated **safe**.  

**Typical Courses of Events**  
1. **Obstacle sensors** + fusion indicate forward **safe** after maneuver.  
2. **System** commands **straight forward** on **drive motors** (OBJ4-FR-2).  
3. **System** ensures **cleaning hardware** is **active** at appropriate power (OBJ2-FR-1) unless another rule suspends it.  
4. Continues as **UC-02**.  

**Alternative Courses of Events**  
- **A1. Immediate re-obstacle:** Forward becomes blocked again immediately; flow returns to **UC-03** or **UC-05** per fusion.  

**Exceptional Courses of Events**  
- **E1. Contradictory motion/cleaning state:** Prevented by OBJ4-FR-7 and design rules **TBD**.  

---

## UC-08 — Handle missing, invalid, or stale obstacle data

**Use Case Name**  
Handle missing, invalid, or stale obstacle data  

**Actor**  
- **Primary:** **Obstacle sensor subsystem** (and, if modeled separately, **sensor electronics / front-end** that can fail, drop samples, or go stale — still **outside** the control software).  
- **Supporting:** **Drive motors / wheel subsystem** (receive stop / safe commands).  
- **Supporting:** **Cleaning hardware** (may be suspended).  
- **Supporting:** **Home user** (may need to reset or move the robot if fault persists).  

**Purpose**  
Avoid unsafe assumptions when obstacle inputs are **missing**, **invalid**, or **stale** beyond allowed time.  

**Overview**  
The **System** enters **conservative safe behavior** (e.g., stop autonomous motion to **drive motors**, suspend **cleaning hardware**) rather than assuming a clear path, until inputs recover or the user intervenes (**TBD**).  

**Cross Reference**  
SRS §3.2.3 **ObstaclePerceptionContext** OBJ3-FR-4; §3.5 SAFE-1; §3.5 REL-1 (recovery after dropout); §3.3 PERF-* (debounce / timing); §3.1.2 HI-1. Legacy: NFR-001, NFR-007.  

**Pre-Requisites**  
- Session may be active or inactive; safe behavior applies whenever autonomous motion would otherwise rely on bad data (**TBD** whether session auto-stops).  

**Typical Courses of Events**  
1. **System** detects missing/invalid/stale obstacle state from **obstacle sensor path** for any required sector beyond timeout **TBD**.  
2. **System** invokes **safe behavior**: e.g., stop **drive motors**, suspend autonomous **cleaning hardware** commands (exact mapping **TBD**).  
3. Optional: **System** requests **UI / annunciator** to notify **home user** / set fault flag **TBD**.  
4. When **obstacle sensor subsystem** again provides valid fresh data, **recovery sequence** **TBD** (SRS REL-1).  

**Alternative Courses of Events**  
- **A1. Partial staleness:** Only one sector stale—policy may degrade to slow stop vs full fault **TBD**.  

**Exceptional Courses of Events**  
- **E1. Never recovers:** Requires user reset or service **TBD**—hardware issue, out of SRS behavioral scope beyond “do not proceed unsafely.”  

---

## UC-09 — Build consistent fused obstacle picture

**Use Case Name**  
Build consistent fused obstacle picture  

**Actor**  
- **Primary:** **Obstacle sensor subsystem** (continuously supplies **raw** front / left / right readings or events into the **System**).  
- **Supporting:** **MCU / sampling path** (optional actor: provides periodic tick or DMA completion if that is how samples arrive — **TBD**; omit in diagrams if folded into sensors).  
- **Supporting:** **Home user** (indirect beneficiary of correct fusion).  

**Purpose**  
When combining **front**, **left**, and **right** readings (especially for surrounded logic), the **System** applies a **consistent snapshot or fusion rule** so decisions are not undefined.  

**Overview**  
Regardless of whether inputs are polled, pushed, or mixed at the hardware level, the **System** produces a coherent fused view for navigation decisions—critical for **UC-05** (**surrounded** + **lateral opening** per §1.3) and for distinguishing **UC-03** vs **UC-05**. *(SRS classes such as `ObstaclePerceptionContext` implement this **inside** the **System** — they are **not** actors.)*  

**Cross Reference**  
SRS §3.2.3 **ObstaclePerceptionContext** OBJ3-FR-1, OBJ3-FR-2, OBJ3-FR-3 (timely front, fresh sides, consistent fusion); §1.3 (**Lateral opening**, **Prefer-right**); §3.2.4 **OBJ4-FR-4**, **OBJ4-FR-5** (navigation consumes fusion for **surrounded** and **lateral opening** per §1.3 — thresholds **TBD**); §2.5 assumptions. Legacy: FR-018, FR-019. *(FR-017 / OBJ3-FR-1 is primary in **UC-03** reaction path but also underpins any use of obstacle inputs.)*  

**Pre-Requisites**  
- Raw obstacle updates arrive from **obstacle sensor subsystem** (implementation-defined).  
- Fusion parameters (debounce, ordering, timestamps) configured per design **TBD**.  

**Typical Courses of Events**  
1. **Obstacle sensor subsystem** delivers raw update(s) to **System**.  
2. **System** applies fusion / snapshot rule (OBJ3-FR-3).  
3. **System** exposes fused obstacle state to navigation decision logic (internal to software).  
4. Navigation outcomes feed **UC-02**–**UC-05** branching.  

**Alternative Courses of Events**  
- **A1. Asynchronous updates:** Snapshot aligns timestamps so “surrounded” is not flickered by ordering artifacts beyond debounce **TBD**.  

**Exceptional Courses of Events**  
- **E1. Fusion cannot produce coherent state:** Fall through to **UC-08**.  

---

## Summary matrix

| ID | Use Case Name | Primary actor (external) |
|----|----------------|---------------------------|
| UC-01 | Start automatic cleaning session | Home user / Scheduler |
| UC-02 | Cruise forward while cleaning | Home user *(beneficiary)* |
| UC-03 | Avoid obstacle (forward blocked, not surrounded) | Obstacle sensor subsystem |
| UC-04 | Avoid obstacle when right turn is blocked | Obstacle sensor subsystem |
| UC-05 | Escape when surrounded | Obstacle sensor subsystem |
| UC-06 | Boost cleaning when dust is high | Dust / debris sensor |
| UC-07 | Resume forward cleaning after maneuver | Obstacle sensor subsystem |
| UC-08 | Handle bad/stale obstacle data | Obstacle sensor subsystem *(faulty path)* |
| UC-09 | Build consistent fused obstacle picture | Obstacle sensor subsystem |

---

## Traceability (use case → SRS legacy IDs)

| Use case | Legacy FR / NFR (see SRS Appendix B) |
|----------|--------------------------------------|
| UC-01 | FR-001, FR-015, FR-016 |
| UC-02 | FR-001, FR-003 |
| UC-03 | FR-002, FR-005, FR-006, FR-007, FR-017 |
| UC-04 | FR-006 |
| UC-05 | FR-008, FR-009, FR-010, NFR-003 |
| UC-06 | FR-011, FR-012, FR-013 |
| UC-07 | FR-001, FR-004, FR-007 |
| UC-08 | NFR-001, NFR-007 |
| UC-09 | FR-018, FR-019 |

---

## Out of scope

Motor control, electrical design, UI wireframes, cloud, SLAM, cliff and stuck recovery—same boundary as the SRS unless you add follow-on documents.

If you need **postconditions** or **UML extend/include** relationships added under each use case, say which notation your course expects.
