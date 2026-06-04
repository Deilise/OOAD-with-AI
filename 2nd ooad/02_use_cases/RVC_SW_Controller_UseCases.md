# Roomba RVC — Use cases

Companion to **`RVC_SW_Controller_SRS.md`** (IEEE 830–style SRS, **v0.6.0**). Each use case below uses the same section layout so you can drop them into reports or trace matrices as-is.

### System boundary (important for actors)

| | |
|--|--|
| **System under design** | **Roomba automatic cleaning control software** — the embedded logic that reads sensor-related inputs, decides maneuvers and cleaning level, and sends **high-level** motion/cleaning commands (as in the SRS). |
| **Actors** | People or **physical / external subsystems outside that software** that supply stimuli or consume commands: user, schedule source, direct **front / left obstacle sensors**, **dust sensor**, **drive / wheel motors**, **vacuum / brush / mop hardware**, etc. There is no dedicated right-side obstacle sensor in SRS v0.6.0; right-side status is inferred by a right-side probe using available sensing and high-level re-orientation. |
| **Not actors** | SRS **objects** such as `NavigationAndEscapeCoordinator` or `SurfaceCleaningController` are **internal classes/modules** — they belong in **class** and **sequence** diagrams **inside** the system rectangle, **not** in the **Actor** field of a use case. |

### Three-layer architecture (how this document fits)

Your stack: **UI layer (top)** → **application / domain layer — “our layer” (middle)** → **OS / platform layer (bottom)**.

| Layer | Role w.r.t. these use cases |
|--------|-----------------------------|
| **UI layer** | Presents **start / stop / status** (and any fault annunciation). **Primary human interaction** for **UC-01** lives here. UI forwards user intent **down** to the middle layer; it does **not** implement obstacle fusion or navigation policy. |
| **Our layer (middle)** | This is the **System** in this document: session state, obstacle fusion and right-side probe interpretation (**UC-09**), navigation and escape (**UC-02**–**UC-05**, **UC-07**), cleaning policy and dust boost (**UC-06**), safe degradation (**UC-08**). **Shall** requirements in the SRS target this layer. |
| **OS / platform layer (bottom)** | Timers, scheduling, drivers, ISRs, IPC, HAL: delivers direct **front / left sensor samples** up and carries **motor / cleaning commands** down, including high-level re-orientation commands used by the right-side probe. Use case **actors** such as **obstacle sensor subsystem** and **drive motors** are realized **through** this layer from the middle layer’s point of view (middle calls **abstractions** that the OS layer implements). |

**Suitability:** Yes. Treat each use case’s **Typical Courses** as **middle-layer** behavior; map **UI** only where user intent or notifications appear (**UC-01**, optional steps in **UC-08**); map direct **front / left sensors**, re-orientation/motor execution, and cleaning hardware to **OS + drivers + HAL** in design and sequence diagrams, not as C++ classes in the middle of business logic. Right-side checking is modeled as a high-level probe behavior, not low-level hardware control.

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
- **Supporting:** Direct **front / left obstacle sensors** (provide obstacle state the **System** reads). Right-side information is only obtained through a probe when a later maneuver needs it.  
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
1. Direct **front / left obstacle sensors** continue to supply readings; **System** evaluates fused obstacle state: forward path **clear**.  
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
- **Primary:** **Obstacle sensor subsystem** — direct **front** and **left** sensing reports “something close” so the **System** reacts; the **right** side is checked only through a right-side probe using available sensing.  
- **Supporting:** **Drive motors / wheel subsystem** (executes turn and forward commands).  
- **Supporting:** **Cleaning hardware** (vacuum / brushes / mop — **System** suspends then resumes cleaning).  
- **Supporting:** **Home user** (beneficiary; not pressing stop during maneuver).  

**Purpose**  
When **forward** is blocked but the robot is **not** fully surrounded, maneuver around the obstacle. Because there is no dedicated right-side sensor, the **System** performs a right-side probe when needed, prefers a right turn if the probe says right is viable, otherwise turns left, then resumes forward cleaning.  

**Overview**  
Cleaning is suspended for the maneuver. The robot may temporarily re-orient to probe the right side with available sensing, restores or accounts for the original heading, executes a lateral avoidance (right if the probe says viable, else left if viable), then drives forward again with cleaning resumed.  

**Cross Reference**  
SRS §3.1.2 HI-1A; §3.2.4 OBJ4-FR-3, OBJ4-FR-8; §3.2.2 OBJ2-FR-2, OBJ2-FR-3; §3.2.3 OBJ3-FR-1–OBJ3-FR-5; §3.3 PERF-4. Legacy: FR-002, FR-005, FR-006, FR-007, FR-017.  

**Pre-Requisites**  
- Session active (`sessionActive` **true**).  
- Fused/probed state: **front** obstacle / unsafe forward; direct **left** state is available; **right** state is available from a fresh right-side probe or can be probed before the final avoidance decision. The situation is **not** simultaneous front+left+right surrounded per OBJ4-FR-4.  

**Typical Courses of Events**  
1. Direct **front** sensing indicates forward blocked; **System** checks direct **left** state and determines that right-side knowledge is needed before final turn selection.  
2. **System** commands **cleaning hardware** to **suspend** cleaning for the maneuver.  
3. **System** performs a high-level right-side probe: it commands a temporary re-orientation so available sensing, such as the front obstacle sensor, can evaluate the right side; then it restores or accounts for the original travel heading.  
4. **System** classifies the situation as **not surrounded** and plans lateral turn: **prefer right** if the right-side probe indicates viable; otherwise use **left** if left is viable (OBJ4-FR-3).  
5. **System** commands **drive motors** to perform the selected turn; obstacle inputs update; when forward is safe, **System** commands **forward** and **resumes** **cleaning hardware**.  
6. Use case ends; flow returns to **UC-02**.  

**Alternative Courses of Events**  
- **A1. Prefer right unavailable:** At step 4, if the right-side probe indicates **right** turn is not viable, **System** executes **left** turn instead when left is viable; then continues from step 5.  
- **A2. Multiple small adjustments:** If forward clears briefly then blocks again, sequence may repeat; debounce per SRS PERF-3 / design **TBD**.  

**Exceptional Courses of Events**  
- **E1. Probe / sensor ambiguity:** If direct sensing plus right-side probe cannot classify surrounded vs not-surrounded, product-defined safe policy **TBD** (may treat as UC-05 or UC-08).  

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
Prefer-right is a **default bias**, not a hard rule. With no dedicated right-side sensor, the **System** probes the right side using available sensing. When the probe shows the right side does not support a safe right turn, the **System** chooses a **left** turn and completes the avoidance.  

**Cross Reference**  
SRS §1.3 **Right-side probe** and **Prefer-right**; §3.1.2 HI-1A; §3.2.3 OBJ3-FR-5; §3.2.4 OBJ4-FR-3, OBJ4-FR-8; §3.3 PERF-4. Legacy: FR-006.  

**Pre-Requisites**  
- Same as UC-03.  
- Additionally: right-side probe indicates **right** turn is **not** viable (e.g., probed right sector blocked), while direct **left** state remains viable (**TBD** clearance rules).  

**Typical Courses of Events**  
1–3. Same as UC-03 steps 1–3 (**System** suspends **cleaning hardware** and performs the right-side probe).  
4. **System** evaluates the right-side probe result: **right** turn is **not** viable.  
5. **System** commands **drive motors** for **left** turn.  
6. When obstacle inputs show forward safe: **System** commands forward + resumes **cleaning hardware**.  
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
When **forward**, **left**, and inferred **right** all indicate blockage, **reverse** until **at least one lateral side** (**left** direct sensing or **right** probe result) opens enough to **start a turn** (SRS **Lateral opening**), then perform a lateral escape and resume forward cleaning when safe. **Forward** alone reading clear **does not** stop the surrounded-escape reverse **segment** while **both** sides are still “closed” for a lateral start. The surrounded response must move backward when required; it is not replaced by a 180-degree turn and forward drive. Respect **maximum backup** and **fallback** if no lateral opening appears.  

**Overview**  
The robot does not command forward while surrounded. Since there is no right-side sensor, the **System** confirms right blockage through a right-side probe. It backs until **left** direct sensing or **right** probe result (or both) show a **lateral opening**, then turns (preferring right when both qualify and right probe confirms viability), then goes forward again when fusion allows. If nothing opens laterally before the backup limit, **fallback** applies (**TBD**).  

**Why reverse (and why this is not “wiggle forever”)**  
- **Reverse** is used because **forward**, **left**, and probed **right** read blocked at entry (**OBJ4-FR-4**): there is no safe forward or lateral move from that pose. Moving **backward** changes geometry so **left** or **right** can become open first — that **lateral opening** is the intended **normal exit** from the reverse segment (**OBJ4-FR-5**).  
- **Forward-only** “clear” along a **long tube** is **ignored** for ending reverse while **both** sides still block a lateral start, so the robot does **not** immediately charge forward down the same blind channel. A 180-degree turn followed by forward travel does not satisfy the required surrounded-escape reverse segment (**SRS OBJ4-FR-5**).  
- **Maximum backup** + **fallback** and **debouncing** (**PERF-3**) still cap pathological cases; **§2.6** lists **global** aids (mapping, stuck heuristics, odometry) as **TBD** for harder traps.  

**Cross Reference**  
SRS §1.3 **Right-side probe** and **Lateral opening**; §2.2 item 4; §2.5 **A-5**; §2.6; §3.1.2 HI-1A; §3.2.3 OBJ3-FR-5; §3.2.4 OBJ4-FR-4, OBJ4-FR-5, OBJ4-FR-6, OBJ4-FR-8; §3.3 PERF-*; §3.5 SAFE-1. Legacy: FR-008, FR-009, FR-010, NFR-003.  

**Pre-Requisites**  
- Session active.  
- Fused/probed obstacle state satisfies **surrounded** condition (OBJ4-FR-4): direct front blocked, direct left blocked, and right-side probe indicates inferred right blocked.  

**Typical Courses of Events**  
1. Direct sensors report **front** blocked and **left** blocked; **System** performs or uses a fresh right-side probe.  
2. Right-side probe indicates inferred **right** blocked; **System** classifies the situation as **surrounded** and ensures **no forward** command to **drive motors**.  
3. **System** commands **reverse** to **drive motors**; reads direct obstacle sensors and repeats/updates right-side probe information as needed by policy **TBD** while reversing.  
4. **During** the **surrounded-escape reverse segment** (entered when step 2 held), **System** **continues reverse** until a **lateral opening** (**left** direct state and/or **right** probe result per fusion — **TBD** thresholds) **or** **maximum backup** is reached (**OBJ4-FR-5**). If **forward** reads clear but **neither** side shows a lateral opening yet, **System** **keeps reversing** (same segment — forward-only clear does **not** end it while both sides are still “closed” for a lateral start, even if the strict **front∧left∧right** pattern momentarily breaks **only** because **forward** cleared).  
5. **Lateral opening:** **System** executes lateral turn via **drive motors** (**OBJ4-FR-6**): both sides open → prefer **right** when the right-side probe confirms viability else **left**; only one side open → turn **toward** that side.  
6. When forward is safe after the turn, **System** resumes **forward** + **cleaning hardware** → **UC-02**.  
7. Use case ends successfully.  

**Alternative Courses of Events**  
- **A1. Prefer right blocked when both sides appear open:** If the right-side probe does not confirm a viable right turn, **System** turns **left** (same spirit as **UC-04**).  
- **A2. Intermittent surrounded / sensor flicker:** Debounce per **PERF-3**; typical course may repeat until stable **lateral opening** or **fallback**.  

**Exceptional Courses of Events**  
- **E1. Max backup reached without lateral opening:** **Fallback** **TBD** (stop, alert, service/reset request, re-probe, etc.) — **OBJ4-FR-5**, **NFR-003** intent. **Forward-only** clear at this point does **not** by itself satisfy normal exit; **fallback** defines what happens next.  
- **E2. Sensor/probe dropout during reverse:** **UC-08**.  
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
- **Primary:** **Obstacle sensor subsystem** (reports that **forward** is again **clear** / safe per fusion rules; right-side status may be inferred by probe if needed).  
- **Supporting:** **Drive motors / wheel subsystem** (execute forward command).  
- **Supporting:** **Cleaning hardware** (resume or maintain cleaning on).  
- **Supporting:** **Home user** (beneficiary).  

**Purpose**  
After a turn or backup escape completes and the path ahead is viable, return to **forward** travel with **cleaning** on—not remain in maneuver state indefinitely.  

**Overview**  
This use case often appears as the **tail** of UC-03, UC-04, or UC-05: once direct obstacle sensors plus any required right-side probe/fusion say the path is safe, the **System** prefers forward progress and normal cleaning.  

**Cross Reference**  
SRS §3.2.4 OBJ4-FR-2, OBJ4-FR-3, OBJ4-FR-6; §3.2.2 OBJ2-FR-1. Legacy: FR-001, FR-004, FR-007.  

**Pre-Requisites**  
- Session active.  
- A maneuver (avoidance or surrounded escape) has completed or forward clearance is granted per fusion **TBD**.  
- Forward path evaluated **safe**.  

**Typical Courses of Events**  
1. Direct obstacle sensors plus any required probe/fusion indicate forward **safe** after maneuver.  
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
Avoid unsafe assumptions when direct obstacle inputs or right-side probe results are **missing**, **invalid**, or **stale** beyond allowed time.  

**Overview**  
The **System** enters **conservative safe behavior** (e.g., stop autonomous motion to **drive motors**, suspend **cleaning hardware**) rather than assuming a clear path or right-side clearance, until inputs/probe results recover or the user intervenes (**TBD**).  

**Cross Reference**  
SRS §3.2.3 **ObstaclePerceptionContext** OBJ3-FR-4, OBJ3-FR-5; §3.5 SAFE-1; §3.5 REL-1 (recovery after dropout); §3.3 PERF-* (debounce / timing / probe bound); §3.1.2 HI-1, HI-1A. Legacy: NFR-001, NFR-007.  

**Pre-Requisites**  
- Session may be active or inactive; safe behavior applies whenever autonomous motion would otherwise rely on bad data (**TBD** whether session auto-stops).  

**Typical Courses of Events**  
1. **System** detects missing/invalid/stale direct obstacle state or missing/invalid/stale right-side probe result for any required sector beyond timeout **TBD**.  
2. **System** invokes **safe behavior**: e.g., stop **drive motors**, suspend autonomous **cleaning hardware** commands (exact mapping **TBD**).  
3. Optional: **System** requests **UI / annunciator** to notify **home user** / set fault flag **TBD**.  
4. When **obstacle sensor subsystem** again provides valid fresh data, **recovery sequence** **TBD** (SRS REL-1).  

**Alternative Courses of Events**  
- **A1. Partial staleness:** Only one direct sector or the right-side probe result is stale — policy may degrade to slow stop vs full fault **TBD**.  

**Exceptional Courses of Events**  
- **E1. Never recovers:** Requires user reset or service **TBD**—hardware issue, out of SRS behavioral scope beyond “do not proceed unsafely.”  

---

## UC-09 — Build consistent fused obstacle picture

**Use Case Name**  
Build consistent fused obstacle picture  

**Actor**  
- **Primary:** **Obstacle sensor subsystem** (continuously supplies direct **raw front / left** readings or events into the **System**). Right-side status is supplied by the **System** through a right-side probe result using available sensing.  
- **Supporting:** **MCU / sampling path** (optional actor: provides periodic tick or DMA completion if that is how samples arrive — **TBD**; omit in diagrams if folded into sensors).  
- **Supporting:** **Home user** (indirect beneficiary of correct fusion).  

**Purpose**  
When combining direct **front**, direct **left**, and inferred **right** status (especially for surrounded logic), the **System** applies a **consistent snapshot or fusion/probe rule** so decisions are not undefined.  

**Overview**  
Regardless of whether direct inputs are polled, pushed, or mixed at the hardware level, the **System** produces a coherent fused view for navigation decisions by combining front/left samples with a right-side probe result — critical for **UC-05** (**surrounded** + **lateral opening** per §1.3) and for distinguishing **UC-03** vs **UC-05**. *(SRS classes such as `ObstaclePerceptionContext` implement this **inside** the **System** — they are **not** actors.)*  

**Cross Reference**  
SRS §3.2.3 **ObstaclePerceptionContext** OBJ3-FR-1, OBJ3-FR-2, OBJ3-FR-3, OBJ3-FR-5 (timely front, fresh left, right-side probe, consistent fusion); §1.3 (**Right-side probe**, **Lateral opening**, **Prefer-right**); §3.2.4 **OBJ4-FR-4**, **OBJ4-FR-5**, **OBJ4-FR-8** (navigation consumes fusion/probe results for **surrounded** and **lateral opening** per §1.3 — thresholds **TBD**); §2.5 assumptions. Legacy: FR-018, FR-019. *(FR-017 / OBJ3-FR-1 is primary in **UC-03** reaction path but also underpins any use of obstacle inputs.)*  

**Pre-Requisites**  
- Raw direct front/left obstacle updates arrive from **obstacle sensor subsystem** (implementation-defined).  
- Fusion/probe parameters (debounce, ordering, timestamps, probe validity window) configured per design **TBD**.  

**Typical Courses of Events**  
1. **Obstacle sensor subsystem** delivers raw direct front/left update(s) to **System**.  
2. When right-side status is needed, **System** requests a right-side probe, receives a probe result, and treats it as inferred right obstacle state.  
3. **System** applies fusion / snapshot / probe-validity rule (OBJ3-FR-3, OBJ3-FR-5).  
4. **System** exposes fused obstacle state to navigation decision logic (internal to software).  
5. Navigation outcomes feed **UC-02**–**UC-05** branching.  

**Alternative Courses of Events**  
- **A1. Asynchronous updates:** Snapshot aligns timestamps and right-side probe validity so “surrounded” is not flickered by ordering/probe artifacts beyond debounce **TBD**.  

**Exceptional Courses of Events**  
- **E1. Fusion/probe cannot produce coherent state:** Fall through to **UC-08**.  

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

| Use case | Legacy FR / NFR (see SRS Appendix B) | Current SRS v0.6.0 additions |
|----------|--------------------------------------|-----------------------------|
| UC-01 | FR-001, FR-015, FR-016 | — |
| UC-02 | FR-001, FR-003 | — |
| UC-03 | FR-002, FR-005, FR-006, FR-007, FR-017 | HI-1A, OBJ3-FR-5, OBJ4-FR-8, PERF-4 |
| UC-04 | FR-006 | HI-1A, OBJ3-FR-5, OBJ4-FR-8, PERF-4 |
| UC-05 | FR-008, FR-009, FR-010, NFR-003 | HI-1A, OBJ3-FR-5, OBJ4-FR-8, PERF-4 |
| UC-06 | FR-011, FR-012, FR-013 | — |
| UC-07 | FR-001, FR-004, FR-007 | — |
| UC-08 | NFR-001, NFR-007 | OBJ3-FR-5, PERF-4 |
| UC-09 | FR-018, FR-019 | HI-1A, OBJ3-FR-5, OBJ4-FR-8 |

---

## Out of scope

Motor control, electrical design, exact right-side probe rotation angle/timing, UI wireframes, cloud, SLAM, cliff and stuck recovery—same boundary as the SRS unless you add follow-on documents.

---

## Change summary for SRS v0.6.0 alignment

### Changed

- Replaced direct right-side obstacle sensor wording with a **right-side probe** using available sensing.
- Updated UC-03, UC-04, UC-05, UC-08, and UC-09 so right-side decisions come from the probe result.
- Kept the meaning of **surrounded** as front + left + right blocked, with right now inferred.
- Clarified that surrounded escape must use backward movement when required and must not be replaced by a 180-degree turn followed by forward travel.

### Added

- Right-side probe steps in obstacle avoidance and surrounded escape.
- Probe failure / timeout paths to UC-08 or product fallback.
- Current SRS v0.6.0 traceability additions for HI-1A, OBJ3-FR-5, OBJ4-FR-8, and PERF-4.

### Removed

- Direct front / left / right obstacle sensor assumptions.
- Wording that treated right-turn viability as directly sensor-valid from a right sensor.

If you need **postconditions** or **UML extend/include** relationships added under each use case, say which notation your course expects.
