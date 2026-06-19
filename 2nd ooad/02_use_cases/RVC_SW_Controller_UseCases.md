# Roomba RVC — Use cases

Companion to **`RVC_SW_Controller_SRS.md`** (IEEE 830–style SRS, **v0.7.2**). Each use case below uses the same section layout so you can drop them into reports or trace matrices as-is.

### System boundary (important for actors)

| | |
|--|--|
| **System under design** | **Roomba automatic cleaning control software** — the embedded logic that reads sensor-related inputs, decides maneuvers and cleaning level, and sends **high-level** motion/cleaning commands (as in the SRS). |
| **Actors** | People or **physical / external subsystems outside that software** that supply stimuli or consume commands: user, schedule source, direct **front / left / back obstacle sensors**, **dust sensor**, **drive / wheel motors**, **vacuum / brush / mop hardware**, etc. There is no dedicated right-side obstacle sensor; right-side status is inferred by a **right-side probe** using the **front** sensor when `travelToggle` is Forward and the **back** sensor when `travelToggle` is Backward. The **back** sensor is the **leading travel edge** when toggled Backward during normal cruise; during surrounded-escape reverse it is **not** the leading sector but **is** used for right-side probe when toggled Backward. Back is **fully ignored** during dust maneuver. |
| **Not actors** | SRS **objects** such as `NavigationAndEscapeCoordinator` or `SurfaceCleaningController` are **internal classes/modules** — they belong in **class** and **sequence** diagrams **inside** the system rectangle, **not** in the **Actor** field of a use case. |

### Three-layer architecture (how this document fits)

Your stack: **UI layer (top)** → **application / domain layer — “our layer” (middle)** → **OS / platform layer (bottom)**.

| Layer | Role w.r.t. these use cases |
|--------|-----------------------------|
| **UI layer** | Presents **start / stop / status** (and any fault annunciation). **Primary human interaction** for **UC-01** lives here. UI forwards user intent **down** to the middle layer; it does **not** implement obstacle fusion or navigation policy. |
| **Our layer (middle)** | This is the **System** in this document: session state and `travelToggle`, obstacle fusion and right-side probe interpretation (**UC-09**), navigation and escape (**UC-02**–**UC-05**, **UC-07**), **Normal / Boost / Suspended** cleaning and dust maneuver (**UC-06**), safe degradation (**UC-08**). **Shall** requirements in the SRS target this layer. |
| **OS / platform layer (bottom)** | Timers, scheduling, drivers, ISRs, IPC, HAL: delivers direct **front / left / back sensor samples** up and carries **motor / cleaning commands** down, including high-level re-orientation commands used by the right-side probe and dust-maneuver spin. Use case **actors** such as **obstacle sensor subsystem** and **drive motors** are realized **through** this layer from the middle layer’s point of view (middle calls **abstractions** that the OS layer implements). |

**Suitability:** Yes. Treat each use case’s **Typical Courses** as **middle-layer** behavior; map **UI** only where user intent or notifications appear (**UC-01**, optional steps in **UC-08**); map direct **front / left / back sensors**, re-orientation/motor execution, and cleaning hardware to **OS + drivers + HAL** in design and sequence diagrams, not as C++ classes in the middle of business logic. Right-side checking is modeled as a high-level probe behavior, not low-level hardware control. **`travelToggle`** swaps which body sector is travel-forward (front when Forward, back when Backward).

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
The user or an automated schedule requests a cleaning session. The **System** marks the session **active**, initializes **`travelToggle` to Forward** and **Normal** cleaning (cleaner on, normal power — not Boost), and enables the autonomous cleaning behavior chain until the session is stopped.  

**Cross Reference**  
SRS §3.2.1 **AutomaticCleaningSession** (OBJ1-FR-1, OBJ1-FR-2, **OBJ1-FR-4**); §3.2.2 **SurfaceCleaningController** OBJ2-FR-1; §3.1.1 UI-1, UI-2. Legacy: FR-001, FR-015, FR-016.  

**Pre-Requisites**  
- Robot is powered and not in a factory-locked state that forbids cleaning (**TBD** product rule).  
- Session **stop** is not asserted.  

**Typical Courses of Events**  
1. **Home user** (or **Scheduler**) issues start-cleaning intent (button, app, schedule — product-defined).  
2. **System** records session **active**, sets **`travelToggle` = Forward**, **Normal** cleaning mode, and enables autonomous cleaning + navigation policy per SRS §2.2.  
3. Use case ends successfully; normal operation continues (e.g. **UC-02**).  

**Alternative Courses of Events**  
- **A1. Start ignored:** Session start is ignored if a higher-priority safety interlock is active (**TBD**); document outcome in product SRS addendum.  
- **A2. Resume after pause:** If “pause” exists as a product feature, resume path **TBD**—may re-use same use case with different pre-requisite.  

**Exceptional Courses of Events**  
- **E1. Command lost:** If start command is not acknowledged by firmware, behavior **TBD** (retry, error LED, etc.)—outside current SRS unless specified.  

---

## UC-02 — Cruise in normal mode while cleaning

**Use Case Name**  
Cruise in normal mode while cleaning  

**Actor**  
- **Primary:** **Home user** (beneficiary — wants floor coverage; implicitly “in play” once UC-01 has started a session).  
- **Supporting:** Direct **front / left / back obstacle sensors** (back applies only when `travelToggle` is Backward). Right-side information is obtained through a probe when a later maneuver needs it.  
- **Supporting:** **Drive motors / wheel subsystem** (receives forward / reverse / turn commands from the **System**).  
- **Supporting:** **Cleaning hardware** — **vacuum motor**, **main brush**, **side brush**, **mop actuator** (as applicable; receive Normal power from the **System**).  

**Purpose**  
Cover floor area by driving straight along the **leading travel sector** with **Normal** cleaning (on, normal power — not Boost) when nothing else demands a maneuver.  

**Overview**  
With an active session and no obstacle, surrounded, or dust-maneuver condition, the **System** commands cruise per **`travelToggle`**: **forward** motion when Forward (front leads), **reverse** motion when Backward (back leads). Cleaning stays at **Normal** power.  

**Cross Reference**  
SRS §1.3 **Leading travel sector**, **Travel-direction toggle**; §3.2.4 **NavigationAndEscapeCoordinator** OBJ4-FR-1; §3.2.2 **SurfaceCleaningController** OBJ2-FR-1. Legacy: FR-001, FR-003.  

**Pre-Requisites**  
- UC-01 (session active) **or** equivalent `sessionActive` **true**.  
- No maneuver in progress; fused obstacle state allows cruise: **OBJ4-FR-4** (surrounded) is **false** per policy, and the **leading travel sector** is clear enough that **OBJ4-FR-1** applies (not in leading-sector-blocked avoidance per **OBJ4-FR-3**).  
- Cleaning in **Normal** mode; not suspended or in Boost for another use case.  

**Typical Courses of Events**  
1. Direct **front / left** (and **back** when toggled Backward) obstacle sensors supply readings; **System** evaluates fused obstacle state: **leading travel sector** **clear**.  
2. **System** commands straight cruise to **drive motors**: **forward** if `travelToggle` is Forward, **reverse** if Backward (OBJ4-FR-1).  
3. **System** keeps **cleaning hardware** at **Normal** power (OBJ2-FR-1).  
4. Loop continues until obstacle, surrounded, dust maneuver, session stop, or sensor fault use case applies.  

**Alternative Courses of Events**  
- **A1. Toggled-backward cruise:** Same steps with `travelToggle` = Backward; **back** is the leading travel edge; **reverse** command maintains world travel direction per SRS §1.3.  
- **A2. After dust maneuver:** `travelToggle` may be Backward; cruise resumes in reverse command with back leading (**UC-06** tail).  

**Exceptional Courses of Events**  
- **E1. Stale / missing obstacle data:** See **UC-08**.  

---

## UC-03 — Avoid obstacle when leading sector blocked (not surrounded)

**Use Case Name**  
Avoid obstacle when leading sector blocked (not surrounded)  

**Actor**  
- **Primary:** **Obstacle sensor subsystem** — **front** (when `travelToggle` Forward) or **back** (when `travelToggle` Backward) plus **left** sensing reports “something close”; **right** is checked through a right-side probe when needed.  
- **Supporting:** **Drive motors / wheel subsystem** (executes turn and cruise commands).  
- **Supporting:** **Cleaning hardware** (vacuum / brushes / mop — **System** suspends then resumes **Normal** cleaning).  
- **Supporting:** **Home user** (beneficiary; not pressing stop during maneuver).  

**Purpose**  
When the **leading travel sector** is blocked but the robot is **not** fully surrounded, maneuver around the obstacle. The **System** performs a right-side probe when needed, prefers a right turn if the probe says right is viable, otherwise turns left, orients the **leading travel edge** toward the clear path, then resumes normal cruise per **`travelToggle`**.  

**Overview**  
Cleaning is **suspended** for the maneuver. The robot may temporarily re-orient to probe the right side, executes lateral avoidance (prefer right when viable, else left), then resumes cruise along the leading sector: **forward** command when Forward toggle, **reverse** when Backward toggle.  

**Cross Reference**  
SRS §1.3 **Leading travel sector**; §3.1.2 HI-1, HI-1A, **HI-1B**; §3.2.4 OBJ4-FR-3, OBJ4-FR-8; §3.2.2 OBJ2-FR-2, OBJ2-FR-3; §3.2.3 OBJ3-FR-1–OBJ3-FR-7; §3.3 PERF-4. Legacy: FR-002, FR-005, FR-006, FR-007, FR-017.  

**Pre-Requisites**  
- Session active (`sessionActive` **true**).  
- Fused/probed state: **leading travel sector** obstacle / unsafe (**front** when Forward toggle, **back** when Backward toggle); direct **left** state available; **right** from probe or probed before final turn. Situation is **not** body-fixed front+left+right surrounded per OBJ4-FR-4.  

**Typical Courses of Events**  
1. Leading-sector sensing indicates blocked path (**front** if Forward toggle, **back** if Backward toggle); **System** checks **left** and determines whether right-side probe is needed.  
2. **System** commands **cleaning hardware** to **suspend** cleaning (OBJ2-FR-3).  
3. **System** performs right-side probe if needed: re-orients so the **front** sensor evaluates right when `travelToggle` is Forward, or the **back** sensor when Backward (OBJ3-FR-5); restore or account for heading.  
4. **System** classifies **not surrounded**; plans lateral turn — **prefer right** if viable, else **left** if viable (OBJ4-FR-3).  
5. **System** commands **drive motors** for the turn, orienting the **leading travel edge** toward the clear path.  
6. When leading sector is safe, **System** resumes cruise per `travelToggle` (**forward** or **reverse** command) and **Normal** cleaning on **cleaning hardware**.  
7. Use case ends; flow returns to **UC-02**.  

**Alternative Courses of Events**  
- **A1. Prefer right unavailable:** **System** turns **left** when left is viable; continues from step 6.  
- **A2. Toggled Backward:** Leading sector is **back**; same sequence with **reverse** resume (OBJ4-FR-3).  
- **A3. Repeated adjustments:** Debounce per SRS PERF-3 / design **TBD**.  

**Exceptional Courses of Events**  
- **E1. Probe / sensor ambiguity:** Product-defined safe policy **TBD** (may treat as UC-05 or UC-08).  

---

## UC-04 — Avoid obstacle when right turn is blocked

**Use Case Name**  
Avoid obstacle when right turn is blocked  

**Actor**  
- **Primary:** **Obstacle sensor subsystem** plus right-side probe result (direct front/left sensing remains available; right-side blockage is inferred by re-orienting and using available sensing — same physical actors as UC-03).  
- **Supporting:** **Drive motors / wheel subsystem**; **Cleaning hardware**; **Home user** (beneficiary).  

**Purpose**  
Same as UC-03, but validates that **prefer-right** is overridden when the right-side probe shows **right** is not a viable turn.  

**Overview**  
Prefer-right is a **default bias**, not a hard rule. The **System** probes the right side using the **front** sensor when Forward toggle, or the **back** sensor when Backward toggle. When the probe shows the right side does not support a safe right turn, the **System** chooses a **left** turn and completes the avoidance.  

**Cross Reference**  
SRS §1.3 **Right-side probe** and **Prefer-right**; §3.1.2 HI-1A; §3.2.3 OBJ3-FR-5; §3.2.4 OBJ4-FR-3, OBJ4-FR-8; §3.3 PERF-4. Legacy: FR-006.  

**Pre-Requisites**  
- Same as UC-03.  
- Additionally: right-side probe indicates **right** turn is **not** viable (e.g., probed right sector blocked), while direct **left** state remains viable (**TBD** clearance rules).  

**Typical Courses of Events**  
1–3. Same as UC-03 steps 1–3 (**System** suspends **cleaning hardware** and performs the right-side probe).  
4. **System** evaluates the right-side probe result: **right** turn is **not** viable.  
5. **System** commands **drive motors** for **left** turn, orienting the **leading travel edge** toward the clear path.  
6. When leading sector is safe: **System** resumes cruise per `travelToggle` + **Normal** **cleaning hardware**.  
7. Use case ends; **UC-02**.  

**Alternative Courses of Events**  
- **A1. Neither lateral turn viable but not “surrounded”:** Behavior **TBD** (may degenerate to small backup, re-scan, or escalate toward UC-05 per product policy).  
- **A2. Right-side probe cannot complete:** If the right-side probe does not complete within the bounded time **TBD**, flow falls through to **UC-08** or product fallback per SRS PERF-4.  

**Exceptional Courses of Events**  
- **E1. Oscillation between left/right:** Mitigated by debounce (SRS **PERF-3**) and product-defined maneuver limits **TBD** (see also **OBJ4-FR-7** — no contradictory motion commands).  

---

## UC-05 — Escape when surrounded (front, left, right blocked)

**Use Case Name**  
Escape when surrounded (front, left, right blocked)  

**Actor**  
- **Primary:** **Obstacle sensor subsystem** (direct front + left sensing) plus right-side probe result. Together they indicate the “surrounded” pattern: front blocked, left blocked, and inferred right blocked.  
- **Supporting:** **Drive motors / wheel subsystem** (reverse, then turn / forward); **Cleaning hardware** (may stay suspended during escape per policy **TBD**); **Home user** (beneficiary).  

**Purpose**  
When **body-fixed forward**, **left**, and inferred **right** all indicate blockage, **reverse** until **at least one lateral side** opens enough to **start a turn**, then perform a lateral escape and resume travel along the **leading travel sector** per preserved **`travelToggle`** when safe. **Forward-only** clear does **not** stop the surrounded-escape reverse segment while both sides remain closed for a lateral start. The surrounded response uses a **surrounded-escape reverse segment** (distinct from toggled-backward cruise; back is **not** the leading sector during escape reverse, but **is** the probe sensor when toggled Backward). Respect **maximum backup** and **fallback** if no lateral opening appears.  

**Overview**  
The robot does not command normal-cruise travel along the leading sector while surrounded. The **System** confirms right blockage through a right-side probe (**front** sensor if Forward toggle, **back** sensor if Backward toggle). It backs until **left** and/or **right** show a **lateral opening**, turns (preferring right when both qualify), orients the **leading travel edge** toward the opening, then exits along that leading sector. **`travelToggle` is unchanged** by surrounded escape.  

**Why reverse (and why this is not “wiggle forever”)**  
- **Reverse** is used because **forward**, **left**, and probed **right** read blocked at entry (**OBJ4-FR-4**): there is no safe forward or lateral move from that pose. Moving **backward** changes geometry so **left** or **right** can become open first — that **lateral opening** is the intended **normal exit** from the reverse segment (**OBJ4-FR-5**).  
- **Forward-only** “clear” along a **long tube** is **ignored** for ending reverse while **both** sides still block a lateral start, so the robot does **not** immediately charge forward down the same blind channel. A 180-degree turn followed by forward travel does not satisfy the required surrounded-escape reverse segment (**SRS OBJ4-FR-5**).  
- **Maximum backup** + **fallback** and **debouncing** (**PERF-3**) still cap pathological cases; **§2.6** lists **global** aids (mapping, stuck heuristics, odometry) as **TBD** for harder traps.  

**Cross Reference**  
SRS §1.3 **Leading travel sector**, **Surrounded-escape reverse segment**, **Lateral opening**; §2.2 item 4; §2.5 **A-5**; §2.6; §3.1.2 HI-1A, **HI-1B**; §3.2.3 OBJ3-FR-5, **OBJ3-FR-7**; §3.2.4 OBJ4-FR-4, OBJ4-FR-5, OBJ4-FR-6, OBJ4-FR-8, **OBJ4-FR-13**; §3.3 PERF-*; §3.5 SAFE-1. Legacy: FR-008, FR-009, FR-010, NFR-003.  

**Pre-Requisites**  
- Session active.  
- Fused/probed obstacle state satisfies **surrounded** condition (OBJ4-FR-4): direct front blocked, direct left blocked, and right-side probe indicates inferred right blocked.  

**Typical Courses of Events**  
1. Direct sensors report **front** blocked and **left** blocked; **System** performs or uses a fresh right-side probe (**front** sensor if Forward toggle, **back** sensor if Backward toggle — OBJ3-FR-5).  
2. Right-side probe indicates inferred **right** blocked; **System** classifies **surrounded** and ensures no normal-cruise command along the leading sector.  
3. **System** commands **reverse** (surrounded-escape reverse segment — **not** toggled-backward cruise); reads **front / left** and probe results. **Back** is **not** used as leading sector; when toggled Backward, **back** **is** used for right-side probe updates during reverse (OBJ3-FR-7).  
4. **During** the **surrounded-escape reverse segment**, **System** **continues reverse** until a **lateral opening** or **maximum backup** (**OBJ4-FR-5**). Forward-only clear with both sides still closed does **not** end the segment.  
5. **Lateral opening:** **System** executes lateral turn (**OBJ4-FR-6**): prefer **right** when viable else **left**; orients **leading travel edge** (per preserved `travelToggle`) toward the opening.  
6. When leading sector is safe, **System** resumes normal cruise along that leading sector (**forward** command if Forward toggle, **reverse** if Backward toggle) + **Normal** **cleaning hardware** → **UC-02**. **`travelToggle` unchanged.**  
7. Use case ends successfully.  

**Alternative Courses of Events**  
- **A1. Prefer right blocked when both sides appear open:** If the right-side probe does not confirm a viable right turn, **System** turns **left** (same spirit as **UC-04**).  
- **A2. Toggled Backward during escape:** Right-side probe uses **back** sensor during reverse segment; step 6 exits with **back** leading (**reverse** command) per OBJ4-FR-6.  
- **A3. Intermittent surrounded / sensor flicker:** Debounce per **PERF-3**; typical course may repeat until stable **lateral opening** or **fallback**.  

**Exceptional Courses of Events**  
- **E1. Max backup reached without lateral opening:** **Fallback** **TBD** (stop, alert, service/reset request, re-probe, etc.) — **OBJ4-FR-5**, **NFR-003** intent. **Forward-only** clear at this point does **not** by itself satisfy normal exit; **fallback** defines what happens next.  
- **E2. Sensor/probe dropout during reverse:** **UC-08**.  
- **E3. Residual symmetric trap:** Fusion never reports a valid **lateral opening** before limits — still possible; see SRS **§2.6** (**TBD** global mitigations).  

---

## UC-06 — Perform dust maneuver (spin, boost, toggle travel)

**Use Case Name**  
Perform dust maneuver (spin, boost, toggle travel)  

**Actor**  
- **Primary:** **Dust / debris sensor** (or equivalent — product **TBD**) reports elevated dirt load.  
- **Supporting:** **Drive motors / wheel subsystem** (stop, **540°** spin).  
- **Supporting:** **Cleaning hardware** (**Boost** power during maneuver).  
- **Supporting:** **Home user** (beneficiary).  

**Purpose**  
When dust exceeds threshold during **normal cruise** (and no higher-priority maneuver is active), **stop**, spin **540°**, clean in **Boost** mode, repeat spin while dust persists, then on clear return to **Normal** power, **toggle `travelToggle`**, and resume cruise preserving world travel direction.  

**Overview**  
Obstacle avoidance, probe, and surrounded escape (**UC-03**–**UC-05**) take **priority** — dust handling waits. During the maneuver the **back sensor is ignored**. Spin direction: **clockwise** if `travelToggle` Forward, **counter-clockwise** if Backward.  

**Cross Reference**  
SRS §1.3 **Dust maneuver**, **Travel-direction toggle**; §3.2.2 OBJ2-FR-4, OBJ2-FR-5, **OBJ2-FR-6**; §3.2.4 OBJ4-FR-9–**OBJ4-FR-12**; §3.2.3 **OBJ3-FR-7**; §3.1.2 HI-2; §3.3 **PERF-5**. Legacy: FR-011, FR-012, FR-013.  

**Pre-Requisites**  
- Session active.  
- **Normal** cruise (not in UC-03/UC-04/UC-05 maneuver; not in surrounded-escape reverse segment).  
- Valid `dustSignal` above threshold **TBD**.  

**Typical Courses of Events**  
1. **Dust / debris sensor** reports level **above** threshold; **System** confirms no higher-priority maneuver active (OBJ4-FR-9).  
2. **System** commands **stop** on **drive motors**.  
3. **System** commands **540°** spin: **clockwise** if `travelToggle` Forward, **counter-clockwise** if Backward (OBJ4-FR-10). **Back sensor ignored.**  
4. **System** commands **Boost** on **cleaning hardware** (OBJ2-FR-4, OBJ2-FR-6).  
5. If dust still above threshold: **System** repeats **540°** spin (same direction) and continues Boost (OBJ4-FR-11).  
6. When dust falls below threshold: **System** returns **cleaning hardware** to **Normal** (OBJ2-FR-5).  
7. **System** **toggles** `travelToggle` Forward ↔ Backward (OBJ4-FR-12, OBJ4-FR-13).  
8. **System** resumes **UC-02** normal cruise along new leading sector, preserving world travel direction.  
9. Use case ends.  

**Alternative Courses of Events**  
- **A1. Deferred by maneuver:** If UC-03/UC-04/UC-05 active, dust maneuver waits until maneuver completes (OBJ4-FR-9).  
- **A2. Multiple spin cycles:** Step 5 repeats until dust clears.  

**Exceptional Courses of Events**  
- **E1. Invalid dust signal:** Safe handling **TBD** (no maneuver or **UC-08**).  
- **E2. Spin timeout:** Safe behavior per **PERF-5** / **OBJ3-FR-4** **TBD**.  

---

## UC-07 — Resume normal cruise after a maneuver

**Use Case Name**  
Resume normal cruise after a maneuver  

**Actor**  
- **Primary:** **Obstacle sensor subsystem** (reports that the **leading travel sector** is again **clear** / safe per fusion rules; right-side status may be inferred by probe if needed).  
- **Supporting:** **Drive motors / wheel subsystem** (execute cruise command per `travelToggle`).  
- **Supporting:** **Cleaning hardware** (resume **Normal** cleaning).  
- **Supporting:** **Home user** (beneficiary).  

**Purpose**  
After avoidance, surrounded escape, or probe completes and the **leading travel sector** is viable, return to **normal cruise** per **`travelToggle`** with **Normal** cleaning — not remain in maneuver state indefinitely.  

**Overview**  
Tail of **UC-03**, **UC-04**, or **UC-05**: once fusion/probe says the **leading sector** is safe, the **System** resumes cruise along that sector (**forward** if Forward toggle, **reverse** if Backward toggle) at **Normal** power.  

**Cross Reference**  
SRS §1.3 **Leading travel sector**; §3.2.4 OBJ4-FR-1, OBJ4-FR-2, OBJ4-FR-3, OBJ4-FR-6; §3.2.2 OBJ2-FR-1. Legacy: FR-001, FR-004, FR-007.  

**Pre-Requisites**  
- Session active.  
- A maneuver (avoidance or surrounded escape) has completed or leading-sector clearance is granted per fusion **TBD**.  
- **Leading travel sector** evaluated **safe**.  

**Typical Courses of Events**  
1. Obstacle sensors plus probe/fusion indicate **leading travel sector** **safe** after maneuver.  
2. **System** commands straight cruise on **drive motors** per `travelToggle`: **forward** (front leads) or **reverse** (back leads) (OBJ4-FR-2).  
3. **System** ensures **cleaning hardware** is **Normal** (OBJ2-FR-1) unless another rule suspends it.  
4. Continues as **UC-02**.  

**Alternative Courses of Events**  
- **A1. Immediate re-obstacle:** Leading sector blocks again; flow returns to **UC-03** or **UC-05** per fusion.  
- **A2. Toggled Backward:** Step 2 uses **reverse** command with **back** as leading edge.  

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
Avoid unsafe assumptions when direct obstacle inputs or right-side probe results are **missing**, **invalid**, or **stale** beyond allowed time.  

**Overview**  
The **System** enters **conservative safe behavior** (e.g., stop autonomous motion to **drive motors**, suspend **cleaning hardware**) rather than assuming a clear path or right-side clearance, until inputs/probe results recover or the user intervenes (**TBD**).  

**Cross Reference**  
SRS §3.2.3 **ObstaclePerceptionContext** OBJ3-FR-4, OBJ3-FR-5, **OBJ3-FR-6**, **OBJ3-FR-7**; §3.5 SAFE-1; §3.5 REL-1; §3.3 PERF-*; §3.1.2 HI-1, HI-1A, **HI-1B**. Legacy: NFR-001, NFR-007.  

**Pre-Requisites**  
- Session may be active or inactive; safe behavior applies whenever autonomous motion would otherwise rely on bad data (**TBD** whether session auto-stops).  

**Typical Courses of Events**  
1. **System** detects missing/invalid/stale direct obstacle state or missing/invalid/stale right-side probe result for any required sector beyond timeout **TBD**.  
2. **System** invokes **safe behavior**: e.g., stop **drive motors**, suspend autonomous **cleaning hardware** commands (exact mapping **TBD**).  
3. Optional: **System** requests **UI / annunciator** to notify **home user** / set fault flag **TBD**.  
4. When **obstacle sensor subsystem** again provides valid fresh data, **recovery sequence** **TBD** (SRS REL-1).  

**Alternative Courses of Events**  
- **A1. Partial staleness:** Only one direct sector, back sector when toggled Backward, or right-side probe stale — policy **TBD**.  
- **A2. Back sensor ignored contexts:** During dust maneuver, back data ignored entirely. During surrounded-escape reverse, back ignored as **leading sector** only; still used for probe when toggled Backward (OBJ3-FR-7).  

**Exceptional Courses of Events**  
- **E1. Never recovers:** Requires user reset or service **TBD**—hardware issue, out of SRS behavioral scope beyond “do not proceed unsafely.”  

---

## UC-09 — Build consistent fused obstacle picture

**Use Case Name**  
Build consistent fused obstacle picture  

**Actor**  
- **Primary:** **Obstacle sensor subsystem** (supplies direct **raw front / left / back** readings or events; back used when toggled Backward during normal cruise). Right-side status from probe result.  
- **Supporting:** **MCU / sampling path** (optional actor: provides periodic tick or DMA completion if that is how samples arrive — **TBD**; omit in diagrams if folded into sensors).  
- **Supporting:** **Home user** (indirect beneficiary of correct fusion).  

**Purpose**  
When combining direct **front**, direct **left**, direct **back** (when applicable), and inferred **right** status, the **System** applies a **consistent snapshot or fusion/probe rule** so decisions are not undefined.  

**Overview**  
The **System** produces a coherent fused view for navigation by combining front/left/back (per `travelToggle` and maneuver context) with right-side probe results — critical for **UC-05**, **UC-03** vs **UC-05**, and toggled-backward leading-sector logic.  

**Cross Reference**  
SRS §3.2.3 **ObstaclePerceptionContext** OBJ3-FR-1, OBJ3-FR-2, OBJ3-FR-3, OBJ3-FR-5, **OBJ3-FR-6**, **OBJ3-FR-7**; §1.3 (**Leading travel sector**, **Right-side probe**, **Lateral opening**); §3.2.4 **OBJ4-FR-4**, **OBJ4-FR-5**, **OBJ4-FR-8**; §2.5 assumptions. Legacy: FR-018, FR-019.  

**Pre-Requisites**  
- Raw direct front/left/back updates arrive from **obstacle sensor subsystem** (back gated by toggle and maneuver context per HI-1B / OBJ3-FR-7).  
- Fusion/probe parameters configured per design **TBD**.  

**Typical Courses of Events**  
1. **Obstacle sensor subsystem** delivers raw direct update(s) to **System**.  
2. When right-side status is needed, **System** requests probe using **front** sensor if Forward toggle, **back** sensor if Backward toggle (OBJ3-FR-5).  
3. When `travelToggle` is Backward during normal cruise, **System** incorporates **back** as leading sector (OBJ3-FR-6). During surrounded-escape reverse, back is **not** leading sector but **is** probe sensor when Backward (OBJ3-FR-7). Back **fully ignored** during dust maneuver.  
4. **System** applies fusion / snapshot rule (OBJ3-FR-3, OBJ3-FR-5).  
5. **System** exposes fused state to navigation logic.  
6. Outcomes feed **UC-02**–**UC-06** branching.  

**Alternative Courses of Events**  
- **A1. Asynchronous updates:** Snapshot aligns timestamps and right-side probe validity so “surrounded” is not flickered by ordering/probe artifacts beyond debounce **TBD**.  

**Exceptional Courses of Events**  
- **E1. Fusion/probe cannot produce coherent state:** Fall through to **UC-08**.  

---

## Summary matrix

| ID | Use Case Name | Primary actor (external) |
|----|----------------|---------------------------|
| UC-01 | Start automatic cleaning session | Home user / Scheduler |
| UC-02 | Cruise in normal mode while cleaning | Home user *(beneficiary)* |
| UC-03 | Avoid obstacle (leading sector blocked, not surrounded) | Obstacle sensor subsystem |
| UC-04 | Avoid obstacle when right turn is blocked | Obstacle sensor subsystem |
| UC-05 | Escape when surrounded | Obstacle sensor subsystem |
| UC-06 | Perform dust maneuver (spin, boost, toggle travel) | Dust / debris sensor |
| UC-07 | Resume normal cruise after maneuver | Obstacle sensor subsystem |
| UC-08 | Handle bad/stale obstacle data | Obstacle sensor subsystem *(faulty path)* |
| UC-09 | Build consistent fused obstacle picture | Obstacle sensor subsystem |

---

## Traceability (use case → SRS)

| Use case | Legacy FR / NFR | SRS v0.7.0 / v0.7.1 additions |
|----------|-----------------|-------------------------------|
| UC-01 | FR-001, FR-015, FR-016 | **OBJ1-FR-4** (Forward toggle + Normal at start) |
| UC-02 | FR-001, FR-003 | **Leading travel sector**, OBJ4-FR-1 |
| UC-03 | FR-002, FR-005–FR-007, FR-017 | **HI-1B**, OBJ3-FR-6/7, leading-sector OBJ4-FR-3 |
| UC-04 | FR-006 | HI-1A, OBJ3-FR-5, OBJ4-FR-8, PERF-4 |
| UC-05 | FR-008–FR-010, NFR-003 | **OBJ4-FR-6** leading-sector exit, **OBJ4-FR-13**, OBJ3-FR-7 |
| UC-06 | FR-011–FR-013 | **OBJ4-FR-9–12**, OBJ2-FR-6, **PERF-5**, dust maneuver |
| UC-07 | FR-001, FR-004, FR-007 | **Leading travel sector**, OBJ4-FR-2 |
| UC-08 | NFR-001, NFR-007 | OBJ3-FR-6/7, HI-1B, PERF-4 |
| UC-09 | FR-018, FR-019 | **Back** in fusion, OBJ3-FR-6/7, HI-1B |

---

## Out of scope

Motor control, electrical design, exact probe/spin angles beyond stated **540°**, UI wireframes, cloud, SLAM, cliff and stuck recovery—same boundary as the SRS unless you add follow-on documents.

---

## Change summary for SRS v0.7.2 alignment

### Changed

- **Right-side probe** uses **front** sensor when Forward toggle, **back** sensor when Backward — in **UC-03**, **UC-04**, **UC-05**, **UC-09**.
- **UC-05**: during surrounded-escape reverse, back is **not** leading sector but **is** probe sensor when toggled Backward (replaces blanket “ignore back”).

### Preserved

- Dust maneuver still **fully ignores** back sensor.
- Surrounded trigger still body-fixed front + left + right.

---

## Change summary for SRS v0.7.1 alignment

### Changed

- Document version aligned to SRS **v0.7.1**.
- **UC-02** renamed and expanded: normal cruise per **`travelToggle`** (forward or reverse command), **Normal** cleaning only.
- **UC-03** / **UC-07**: **leading travel sector** (front or back) replaces forward-only wording; toggled-backward avoidance and resume supported.
- **UC-05**: escape exit along **leading travel sector** per preserved toggle; back **not** leading sector during escape reverse (v0.7.2: back **is** probe sensor when toggled Backward); **`travelToggle` unchanged**.
- **UC-06** rewritten: **dust maneuver** (540° spin, Boost, toggle) replaces bounded boost.
- **UC-08** / **UC-09**: **back** sensor and ignore rules during dust maneuver / surrounded escape.

### Added

- **`travelToggle`** and **leading travel sector** in system boundary and architecture notes.
- Toggled-backward alternatives in UC-02, UC-03, UC-05, UC-07.
- Dust-maneuver priority and spin-direction rules in UC-06.

### Preserved

- UC IDs **UC-01**–**UC-09** unchanged (no split/merge).
- Right-side probe and prefer-right logic (UC-03, UC-04, UC-05).
- Surrounded trigger still body-fixed front + left + right (right from probe).

### Prior change summary (SRS v0.6.0)

- Right-side probe replaced dedicated right sensor; surrounded escape uses reverse segment. See prior revision notes in git history.

If you need **postconditions** or **UML extend/include** relationships added under each use case, say which notation your course expects.
