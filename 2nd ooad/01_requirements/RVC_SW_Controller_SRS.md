# Software Requirements Specification  
## Roomba RVC Software Controller

| Field | Value |
|--------|--------|
| **Document title** | Software Requirements Specification (SRS) |
| **Software product** | Roomba Robotic Vacuum Cleaner (RVC) — **Software Controller** (automatic cleaning behavior) |
| **Organization / project** | *(TBD)* |
| **SRS version** | 0.7.2 |
| **Issue date** | 2026-06-19 |
| **Status** | Preliminary |
| **Notation** | Requirements use **SHALL** / **SHOULD** per convention; attributes use descriptive statements. |
| **Template basis** | IEEE Std 830-1998, **Annex A.4** — *Template of SRS Section 3 organized by object* (informative), plus Clause 5 overall SRS structure. |

---

# Table of contents

1. [Introduction](#1-introduction)  
   1.1 [Purpose](#11-purpose)  
   1.2 [Scope](#12-scope)  
   1.3 [Definitions, acronyms, and abbreviations](#13-definitions-acronyms-and-abbreviations)  
   1.4 [References](#14-references)  
   1.5 [Overview](#15-overview)  
2. [Overall description](#2-overall-description)  
   2.1 [Product perspective](#21-product-perspective)  
   2.2 [Product functions](#22-product-functions)  
   2.3 [User characteristics](#23-user-characteristics)  
   2.4 [Constraints](#24-constraints)  
   2.5 [Assumptions and dependencies](#25-assumptions-and-dependencies)  
   2.6 [Apportioning of requirements](#26-apportioning-of-requirements)  
3. [Specific requirements](#3-specific-requirements)  
   3.1 [External interface requirements](#31-external-interface-requirements)  
   3.2 [Classes / objects](#32-classes--objects)  
   3.3 [Performance requirements](#33-performance-requirements)  
   3.4 [Design constraints](#34-design-constraints)  
   3.5 [Software system attributes](#35-software-system-attributes)  
   3.6 [Other requirements](#36-other-requirements)  
[Appendix A — Change history](#appendix-a--change-history)  
[Appendix B — Traceability (legacy requirement IDs)](#appendix-b--traceability-legacy-requirement-ids)
[Appendix C — Human-readable change summary for v0.6.0](#appendix-c--human-readable-change-summary-for-v060)  
[Appendix D — Human-readable change summary for v0.7.0](#appendix-d--human-readable-change-summary-for-v070)  
[Appendix E — Human-readable change summary for v0.7.1](#appendix-e--human-readable-change-summary-for-v071)  
[Appendix F — Human-readable change summary for v0.7.2](#appendix-f--human-readable-change-summary-for-v072)

---

# 1. Introduction

## 1.1 Purpose

This SRS delineates **what** the Roomba RVC **Software Controller** shall do to implement **automatic cleaning**: session control, **Normal** and **Boost** cleaning modes, **travel-direction toggle** (forward vs toggled-backward normal cruise), use of direct obstacle information (**front**, **left**, and **back** when toggled backward), inferred **right-side** obstacle status obtained by a right-side probe maneuver using available sensing, navigation and escape maneuvers (including right-side checking before **prefer-right** avoidance and, when surrounded, **reverse until a lateral opening** — **left** or **right** — then lateral escape), and **dust maneuver** behavior (stop, **540°** spin, Boost clean, toggle travel direction).  

**Intended audience:** product owners, systems engineers, software architects, implementers (e.g., C++ developers), verification engineers, and maintainers.

## 1.2 Scope

**In scope**

- Software-visible **behavior** for automatic vacuuming/mopping sessions: **Normal** forward cruise at session start, **travel-direction toggle**, obstacle reactions (including **back** sensor when toggled backward), surrounded escape, **dust maneuver** (540° spin + Boost clean + toggle), and safe handling of bad/stale sensor data at the behavioral level.
- **Logical** inputs (front, left, and back obstacle state, dust signal), **derived** right-side obstacle status from a high-level right-side probe, and **logical** outputs (high-level motion and cleaning commands). Detailed motor control, register-level drivers, exact rotation angles beyond the stated **540°** dust spin, and electrical interfaces are **out of scope** for this SRS (see §2.4).

**Out of scope**

- Detailed hardware design, manufacturing test, mobile app UX, cloud services, mapping (SLAM), cliff detection, and stuck recovery unless added by a future revision.

**Benefits / objectives**

- Single baseline for implementation and test.
- Traceable requirements for evolution and change control.

## 1.3 Definitions, acronyms, and abbreviations

| Term / acronym | Definition |
|----------------|------------|
| **RVC** | Robotic vacuum cleaner |
| **SRS** | Software requirements specification (this document) |
| **Software Controller** | The software module (or set of modules) that implements the behaviors in §3 |
| **Cleaning (Normal)** | Vacuum and/or mop treatment is commanded **on** at **normal** power (not Boost). Default cleaning mode when no maneuver applies. |
| **Cleaning (Boost)** | Elevated cleaning power commanded during a **dust maneuver** (§1.3). |
| **Cleaning (suspended)** | Treatment is commanded **off** while the robot performs an obstacle or escape maneuver (e.g., forward/back leading-sector avoidance). **Not** suspended during the dust maneuver itself. |
| **Obstacle (sector)** | Indication that **front**, **left**, **back**, or **right** is blocked or too close per product thresholds. **Front**, **left**, and **back** are directly sensed when applicable to the current travel mode; **right** is inferred by a right-side probe because no dedicated right-side obstacle sensor is available. |
| **Obstacle state (per sector)** | Fused obstacle/clearance for a sector. Front, left, and back may be raw or fused direct inputs; right is the result of the right-side probe. Delivery of direct sensor samples (poll, interrupt, message, etc.) is **not** prescribed by this SRS. |
| **Travel-direction toggle** (`travelToggle`) | Persistent system state: **Forward** or **Backward**. The toggle **swaps which body sector is the leading travel edge**: when **Forward**, the **front** sector leads normal cruise (forward motion command); when **Backward**, the **back** sector leads normal cruise (reverse motion command) — i.e., back is **travel-forward** for that mode. Initialized to **Forward** at session start. The system toggles Forward ↔ Backward automatically when a **dust maneuver** completes. **Not** changed by surrounded-escape reverse segments. |
| **Leading travel sector** | The obstacle sector in the direction of normal cruise for the current `travelToggle`: **front** when `travelToggle` is Forward; **back** when `travelToggle` is Backward. Obstacle avoidance and post-maneuver resume use this sector — not always the physical front sensor. |
| **Toggled-backward normal cruise** | Normal cleaning cruise while `travelToggle` is **Backward**. The **back** sector is the leading travel edge; the Software Controller commands **reverse** motion so that, from an external observer’s view, the RVC continues along the **same world travel direction** as before the most recent dust-maneuver toggle. Distinct from a **surrounded-escape reverse segment**. |
| **Surrounded-escape reverse segment** | Temporary continuous reverse commanded by **OBJ4-FR-5** when surrounded. **Does not** set or reflect `travelToggle`. The **back** sensor is **not** used as the **leading travel sector** during this segment, but **is** used for **right-side probe** when `travelToggle` is **Backward** (see **Right-side probe**). |
| **Dust maneuver** | High-priority-below-escape behavior on dust detection during normal cruise: **stop**, rotate **540°** (**clockwise** if `travelToggle` is Forward, **counter-clockwise** if Backward), command **Boost** cleaning, and **repeat** the 540° spin while dust remains above threshold. The **back** sensor is **ignored** during the dust maneuver (including for probe). On completion (dust cleared), the system toggles `travelToggle` and resumes **Normal** cruise in the new toggle direction. |
| **Right-side probe** | A high-level maneuver used to infer right-side obstacle status without a dedicated right-side sensor. The probe sensor depends on `travelToggle`: when **Forward**, the **System** re-orients so the **front** obstacle sensor evaluates the right side; when **Backward**, the **System** re-orients so the **back** obstacle sensor evaluates the right side. Exact motor control, rotation angle, timing, and sensor hardware details are **TBD** / out of scope. |
| **Prefer-right** | When a lateral turn is needed, the system **SHALL** prefer **turning right** if the right-side probe confirms that a right turn is viable; otherwise **turn left** if left is viable. |
| **Lateral opening** | During **surrounded escape** (§3.2.4 **OBJ4-FR-4**–**OBJ4-FR-6**), fused obstacle state where **left** direct sensing **OR** right-side probe result (or **both**) indicates enough clearance to **begin a lateral turn** (thresholds **TBD**). **Forward-only** clearance **does not** satisfy this while **both** sides remain “closed” for a lateral start per fusion/probe. |
| **HAL** | Hardware abstraction layer (optional); not mandated by this SRS |
| **TBD** | To be determined — see IEEE 830 guidance: each TBD **SHALL** be resolved with owner and target date in project records |

## 1.4 References

| ID | Document | Note |
|----|-----------|------|
| R-1 | IEEE Std 830-1998, *IEEE Recommended Practice for Software Requirements Specifications* | Template structure (Annex **A.4**, Section 3 by object) |
| R-2 | `RVC_SW_Controller_UseCases.md` (same workspace) | Use-case companion; align revision with SRS **v0.7.2** |
| R-3 | Project preliminary goals (captured in §1.5 / §3.2) | Source narrative for automatic cleaning |

*(Issuing organization, contract IDs, and revision-controlled repository locations: **TBD**.)*

## 1.5 Overview

This SRS follows IEEE 830 **Figure 1** outline:

- **Section 1** introduces purpose, scope, definitions, references, and this overview.
- **Section 2** describes the product at a high level (context, functions summary, users, constraints, assumptions, deferred work).
- **Section 3** states **specific** requirements. **Section 3.2** uses **Annex A.4**: requirements are grouped under **software objects** (each with **attributes**, **functions** stated as *shall* requirements, and **messages**). Global performance, constraints, and quality attributes appear in §3.3–§3.6.

Legacy IDs **FR-xxx** / **NFR-xxx** from SRS v0.1–0.4 are mapped in **Appendix B** for continuity.

---

# 2. Overall description

## 2.1 Product perspective

The Software Controller is a **component** of the Roomba RVC product. It sits between:

- **Higher-level inputs:** session commands (start/stop automatic cleaning), direct **front**, **left**, and **back** obstacle signals, and dust signals (**implementation-defined** direct-sample delivery). The Software Controller derives right-side status by commanding a right-side probe and evaluating available sensor data. **Back** sensing applies only during **toggled-backward** normal cruise (see §1.3).
- **Lower-level consumers:** motion and cleaning subsystems consume **high-level commands** (e.g., forward, reverse, turn left/right, cleaning intensity normal/boost/off); mapping to actuators is **outside** this SRS.

**Conceptual context (textual)**

```text
[ User / scheduler ] ──► [ CleaningSession ]
                              │
[ Obstacle + dust inputs ] ──►│──► [ Navigation + cleaning logic ]
                              │
                              └──► [ High-level motion / cleaning commands ] ──► [ Platform / HAL ]
```

## 2.2 Product functions

Summary of major functions (detail in §3.2):

1. **Session control** — start/stop automatic cleaning session; session begins in **Normal** cleaning and **Forward** travel toggle.  
2. **Default cruise** — straight motion per `travelToggle` (forward command when Forward, reverse when Backward) while **Normal** cleaning when no higher-priority maneuver applies.  
3. **Leading-sector obstacle avoidance** — when the **leading travel sector** is blocked (**front** when Forward toggle, **back** when Backward toggle): suspend cleaning; if right status is needed, perform a right-side probe; **prefer-right** lateral turn when viable, otherwise turn left if viable; orient the **leading travel edge** toward the clear path; resume cruise and **Normal** cleaning per `travelToggle` (forward command when Forward, reverse when Backward).  
4. **Surrounded escape** — when **body-fixed** front+left+right are blocked (right from probe); **reverse until a lateral opening**; then lateral turn and resume travel along the **leading travel sector** per preserved `travelToggle`. Uses a **surrounded-escape reverse segment** (distinct from toggled-backward cruise; back is **not** the leading sector during escape reverse, but **is** the probe sensor when `travelToggle` is Backward). **Forward-only** clearance while **both** sides remain closed **does not** end the surrounded-escape reverse **segment**; **maximum backup** + **fallback** if no lateral opening (**TBD**).  
5. **Dust maneuver** — on dust during normal cruise (after obstacle/escape maneuvers complete): stop, **540°** spin (CW if Forward toggle, CCW if Backward toggle), **Boost** clean, repeat spin while dust persists; on clear, toggle `travelToggle` and resume **Normal** cruise so world travel direction is preserved from an observer’s view.  
6. **Safe degradation** — conservative behavior on missing/invalid/stale obstacle data.  
7. **Consistent multi-sector decisions** — fusion/snapshot rules for combined front/left/back (when applicable) direct state and right-side probe results.

## 2.3 User characteristics

Operators are **home users** or **technicians**; no specialized training is assumed for routine automatic cleaning. Session start/stop is expected via product UI (exact UI **TBD**). This SRS does **not** specify screen layouts.

## 2.4 Constraints

- **Behavioral focus only:** the SRS SHALL **not** mandate module partitioning, class diagrams, or scheduling algorithms beyond externally visible behavior and stated performance bounds.
- **No direct right-side sensor:** this revision assumes there is no dedicated right-side obstacle sensor. Right-side obstacle status SHALL be inferred by a high-level right-side probe using available sensing and commanded re-orientation. The SRS does not prescribe motor-control details for the probe.
- **Back sensor scope:** the **back** sensor is the **leading travel sector** when `travelToggle` is **Backward** during normal cruise and post-maneuver resume. It is also the **right-side probe sensor** whenever `travelToggle` is **Backward** (including during surrounded escape). It SHALL **not** be used as the leading sector during **surrounded-escape reverse** segments. It SHALL be **ignored** during **dust maneuvers** (including for probe).
- **No prescribed direct sensor acquisition:** front/left/back obstacle signaling MAY use any combination of polling, interrupts, or messages, provided §3.2 and §3.3 are satisfied.
- **Implementation language** (e.g., C++) is **TBD** at organizational level; not a behavioral requirement of the product behavior itself.
- Regulatory, safety certification, and standards compliance beyond this document are **TBD**.

## 2.5 Assumptions and dependencies

- **A-1:** Obstacle sectors **front**, **left**, **back**, and **right** are meaningful for behavior. **Back** is the leading sector and/or probe sensor when `travelToggle` is Backward (see §1.3 **Right-side probe**). **Back** is ignored during dust maneuvers. The **right** sector is evaluated through a right-side probe; exact zones and probe pose/angle are **TBD**.  
- **A-2:** A **dust** or debris-load signal exists and can be thresholded; definition **TBD**.  
- **A-3:** A lower layer delivers direct front/left/back obstacle samples with bounded latency once hardware is fixed; the platform also supports high-level re-orientation commands needed for the right-side probe and the **540°** dust spin. Numeric targets are **TBD** (see §3.3).  
- **A-4:** Mopping vs vacuum interaction (parallel vs sequential) **TBD**.  
- **A-5:** **Local obstacle sectors alone** cannot always distinguish a **true exit** from a **long straight opening** with no global way out. Surrounded escape therefore **SHALL NOT** end its reverse **segment** on **forward-only** clearance while **left** direct sensing and right-side probe result remain insufficient for a lateral start (**OBJ4-FR-5**). **Global** disambiguation for all geometries remains deferred (see §2.6).  
- **Dependencies:** platform provides **timestamps** or equivalent so freshness/staleness rules can be applied (**TBD** if wall-clock vs tick count).

## 2.6 Apportioning of requirements

The following are **explicitly deferred** (not required in the initial release covered by this SRS unless marked otherwise):

- SLAM / mapping, cliff sensors, explicit stuck detection, user-facing diagnostics, cloud analytics.  
- **Long-tube / symmetric-corridor trap:** **Lateral-opening-first** backup (**OBJ4-FR-5**) reduces the case where **forward** alone reads clear along a tube but **both** sides are still walls. **Residual** traps (e.g. symmetric Y-junction, odometry-free loops) may still need **odometry**, **cycle / stuck detection**, **mapping**, or **user intervention** — **TBD** beyond **maximum backup**, **fallback** (**OBJ4-FR-5**), and debouncing (**PERF-3**).

---

# 3. Specific requirements

*This section follows IEEE 830 **Annex A.4** for §3.2 (objects). Subsections §3.1 and §3.3–§3.6 complete Section 3 per IEEE 830 Clause 5.*

## 3.1 External interface requirements

### 3.1.1 User interfaces

| UI-1 | The system SHALL expose a means to **start** and **stop** an automatic cleaning session (exact control surface **TBD** — physical button, app, schedule, etc.). |
| UI-2 | While the session is **stopped**, the Software Controller SHALL NOT execute autonomous navigation/cleaning behaviors described in §3.2. *(see §3.2.1 **AutomaticCleaningSession**)* |

### 3.1.2 Hardware interfaces

Logical only (no pin-level specification):

| HI-1 | The Software Controller SHALL consume direct **front** and **left** obstacle state as logical inputs (boolean or enumerated proximity per design). |
| HI-1A | The Software Controller SHALL derive **right** obstacle state through a right-side probe maneuver using available sensing; a dedicated right-side obstacle sensor is not required and is not assumed. |
| HI-1B | The Software Controller SHALL consume direct **back** obstacle state when the **back** sector is the **leading travel sector** or the **right-side probe sensor** (`travelToggle` is **Backward**). During **surrounded-escape reverse**, back samples SHALL be used **only** for right-side probe / inferred-right updates, **not** as the leading travel sector. During **dust maneuvers**, back obstacle input SHALL be **ignored** for all navigation and probe decisions. |
| HI-2 | The Software Controller SHALL consume a **dust / debris load** signal (scalar or level per design). |
| HI-3 | The Software Controller SHALL output **high-level motion commands** (e.g., drive forward, reverse, rotate left/right) and **cleaning actuation commands** (Normal/Boost/off). Physical realization **TBD**. |

### 3.1.3 Software interfaces

| SI-1 | Direct front/left/back obstacle and dust inputs SHALL be obtainable from a **sensor interface layer** (name/API **TBD**); acquisition mechanism (poll vs push) is **not** constrained by this SRS. |
| SI-2 | Motion and cleaning commands SHALL be submitted to a **command sink** (HAL or motion service — **TBD**). |

### 3.1.4 Communications interfaces

| CI-1 | No network or radio behavior is required by this revision of the SRS unless extended by a future specification (**N/A** unless **TBD** product variant). |

---

## 3.2 Classes / objects

*Per Annex A.4: each object lists **attributes**, **functions** (functional requirements), and **messages** (communications received or sent).*

*Design trace note:* Class diagrams and sequence diagrams may show the four SRS objects below as software classes, plus supporting design artifacts such as a controller composition root, value/message payload types, enums, and external port/sink interfaces. Those supporting artifacts are design structure for traceability; they do **not** add product behavior beyond the requirements in this SRS.

### 3.2.1 Object: **AutomaticCleaningSession**

#### 3.2.1.1 Attributes (direct or inherited)

| ID | Attribute | Description |
|----|-----------|-------------|
| AT-1.1 | `sessionActive` | Boolean: automatic cleaning session is **on** or **off** |
| AT-1.2 | `sessionSource` | Optional: user, schedule, remote — **TBD** |

#### 3.2.1.2 Functions (services / methods)

| ID | Functional requirement |
|----|-------------------------|
| OBJ1-FR-1 | When `sessionActive` becomes **true**, the Software Controller SHALL enable the automatic cleaning behavior chain (cleaning + navigation policy) consistent with **§3.2.2** (*SurfaceCleaningController*) and **§3.2.4** (*NavigationAndEscapeCoordinator*). *(legacy **FR-001**, **FR-015**)* |
| OBJ1-FR-2 | When `sessionActive` is **false**, the Software Controller SHALL NOT perform autonomous obstacle avoidance, surrounded escape, dust maneuver, or autonomous normal cruise described in §3.2.2 through §3.2.4. *(legacy **FR-016**)* |
| OBJ1-FR-3 | Starting and stopping a session SHALL be deterministic with respect to the session interface (no “half on” state without definition — state machine **TBD** in design). |
| OBJ1-FR-4 | When `sessionActive` becomes **true**, the Software Controller SHALL initialize `travelToggle` to **Forward** and `cleaningMode` to **Normal** (cleaner on, normal power — not Boost). |

#### 3.2.1.3 Messages (communications received or sent)

| Direction | Message | Description |
|-----------|---------|---------------|
| Received | `StartSession` | Asserts `sessionActive` |
| Received | `StopSession` | Negates `sessionActive` |
| Received | `ResumeSession` | Resumes a paused session if the product defines pause/resume; policy **TBD** (see UC-01 A2) |
| Received | `requestServiceOrReset` | User service/reset intent when a persistent fault does not recover; handling **TBD** (see UC-08 E1) |
| Sent | *(internal)* | Notifies **SurfaceCleaningController** and **NavigationAndEscapeCoordinator** of session state changes |

---

### 3.2.2 Object: **SurfaceCleaningController**

#### 3.2.2.1 Attributes

| ID | Attribute | Description |
|----|-----------|-------------|
| AT-2.1 | `cleaningMode` | `Normal`, `Boost`, or `Suspended` — exact enum **TBD** |
| AT-2.2 | `powerLevel` | Normal vs Boost — **TBD** quantization |
| AT-2.3 | `dustSignal` | Input mirror of dust/debris signal |

#### 3.2.2.2 Functions

| ID | Functional requirement |
|----|-------------------------|
| OBJ2-FR-1 | While `sessionActive` is true and no maneuver forces suspension or Boost, the Software Controller SHALL command **Normal** cleaning (vacuum/mop on at normal power per product configuration). *(legacy **FR-001**)* |
| OBJ2-FR-2 | The system SHALL distinguish **Normal**, **Boost**, and **Suspended** cleaning modes. *(legacy **FR-002**)* |
| OBJ2-FR-3 | When leading-sector obstacle handling or surrounded escape requires suspension, the Software Controller SHALL command **cleaning suspended** for the duration of that maneuver. *(legacy **FR-005**)* |
| OBJ2-FR-4 | During a **dust maneuver**, the Software Controller SHALL command **Boost** cleaning (elevated power). *(legacy **FR-011**, **FR-012**)* |
| OBJ2-FR-5 | When a dust maneuver completes because `dustSignal` falls below threshold **TBD**, the Software Controller SHALL return to **Normal** power. *(legacy **FR-013**)* |
| OBJ2-FR-6 | A dust maneuver SHALL **not** suspend cleaning; Boost remains active for the spin-and-clean sequence until dust clears. |

#### 3.2.2.3 Messages

| Direction | Message | Description |
|-----------|---------|---------------|
| Received | `DustSignalUpdated` | New dust sample |
| Received | `SuspendCleaningForManeuver` | From **NavigationAndEscapeCoordinator** |
| Received | `ResumeCleaningAfterManeuver` | From **NavigationAndEscapeCoordinator** |
| Sent | `CleaningCommand` | On/off/intensity to command sink |

---

### 3.2.3 Object: **ObstaclePerceptionContext**

#### 3.2.3.1 Attributes

| ID | Attribute | Description |
|----|-----------|-------------|
| AT-3.1 | `frontObstacle` | Forward sector blocked / clear |
| AT-3.2 | `leftObstacle` | Left sector |
| AT-3.3 | `backObstacle` | Rear sector — leading travel edge or right-side probe sensor when `travelToggle` is Backward; not leading sector during surrounded-escape reverse; ignored during dust maneuver |
| AT-3.4 | `rightObstacleInferred` | Right sector blocked / clear as determined by the right-side probe |
| AT-3.5 | `rightProbeStatus` | Pending, valid, invalid, or stale right-side probe result — exact enum **TBD** |
| AT-3.6 | `lastUpdateTime` (per direct sector, right probe, or fused) | For staleness — structure **TBD** |

#### 3.2.3.2 Functions

| ID | Functional requirement |
|----|-------------------------|
| OBJ3-FR-1 | The Software Controller SHALL obtain **front** obstacle state with latency sufficient for safe stop/avoidance and for the right-side probe when `travelToggle` is **Forward** (numeric budget **TBD**). Acquisition mechanism is **implementation-defined**. *(legacy **FR-017**)* |
| OBJ3-FR-2 | The Software Controller SHALL obtain **left** obstacle state and right-side probe results with freshness sufficient for avoidance and surrounded logic (rate/probe validity window **TBD**, documented in design). *(legacy **FR-018**)* |
| OBJ3-FR-3 | For decisions needing **combined** front, left, right, and (when applicable) **back** state, the Software Controller SHALL apply a **consistent snapshot or fusion rule** that combines direct front/left/back samples with the right-side probe result (ordering, debounce, staleness, and probe validity **TBD**) so behavior is not undefined. *(legacy **FR-019**)* |
| OBJ3-FR-4 | If obstacle data for any required sector is **missing**, **invalid**, or **stale** beyond timeout **TBD**, the system SHALL enter **safe behavior** (e.g., stop motion, suspend autonomous cleaning commands) per **NFR-001** in §3.5. |
| OBJ3-FR-5 | When direct right-side obstacle data is unavailable, the Software Controller SHALL determine right-side blockage by commanding a high-level right-side probe and evaluating the right side with the probe sensor for the current `travelToggle`: **front** sensor when Forward, **back** sensor when Backward. This applies during avoidance, surrounded escape, and surrounded-escape reverse when a fresh right result is needed. The probe SHALL restore or account for the original travel heading before the final avoidance/escape decision; exact motion-control implementation is **TBD**. |
| OBJ3-FR-6 | When `travelToggle` is **Backward** during normal cruise (not dust maneuver, not using back as leading sector during surrounded-escape reverse), the Software Controller SHALL obtain **back** obstacle state with latency sufficient for safe stop/avoidance as the **leading travel sector** (numeric budget **TBD**). When `travelToggle` is **Backward** during any maneuver requiring right-side probe, the **back** sensor SHALL also support probe sampling per **OBJ3-FR-5**. |
| OBJ3-FR-7 | During a **dust maneuver**, the Software Controller SHALL **ignore** back obstacle samples for **all** navigation and probe decisions. During a **surrounded-escape reverse segment**, the Software Controller SHALL **not** treat back as the **leading travel sector**, but **shall** use back for **right-side probe** when `travelToggle` is **Backward** per **OBJ3-FR-5**. |

#### 3.2.3.3 Messages

| Direction | Message | Description |
|-----------|---------|---------------|
| Received | `ObstacleStateChanged` | Direct front/left/back push notification, periodic read completion, **or** probe-pose observation such as `probePose=right` using available sensing — **implementation-defined** |
| Sent | `FusedObstacleSnapshot` | To **NavigationAndEscapeCoordinator** |
| Sent | `RequestRightSideProbe` | Requests high-level re-orientation/probe when right-side status is required |

---

### 3.2.4 Object: **NavigationAndEscapeCoordinator**

#### 3.2.4.1 Attributes

| ID | Attribute | Description |
|----|-----------|-------------|
| AT-4.1 | `motionState` | e.g., `ForwardCruise`, `BackwardCruise`, `Avoiding`, `RightSideProbing`, `SurroundedReversing`, `DustManeuvering`, `Turning` — **TBD** |
| AT-4.2 | `backupDistanceRemaining` | For bounded surrounded-escape reverse — **TBD** units |
| AT-4.3 | `travelToggle` | **Forward** or **Backward** — persistent normal-cruise travel mode; see §1.3 |

#### 3.2.4.2 Functions

| ID | Functional requirement |
|----|-------------------------|
| OBJ4-FR-1 | While **Normal** cleaning is active (not suspended) and no higher-priority maneuver applies, the Software Controller SHALL command straight normal cruise along the **leading travel sector** per `travelToggle`: **forward** motion command when **Forward** (front leads), **reverse** motion command when **Backward** (back leads). *(legacy **FR-003**)* |
| OBJ4-FR-2 | After completing avoidance, surrounded escape, or a right-side probe, the Software Controller SHALL resume normal cruise along the **leading travel sector** per the **current** `travelToggle` when that leading sector is safe. *(legacy **FR-004**)* |
| OBJ4-FR-3 | When the **leading travel sector** is blocked/unsafe but **not** surrounded (i.e., the condition of **OBJ4-FR-4** does **not** hold per fusion/probe policy) — **front** when `travelToggle` is Forward, **back** when `travelToggle` is Backward — the Software Controller SHALL initiate avoidance: **suspend cleaning** (via message to **SurfaceCleaningController**, §3.2.2), determine right-side viability by right-side probe when no fresh valid right result exists, then execute a **lateral turn**, **preferring right** if the probe indicates right is viable, else **left** if left is viable, orienting the **leading travel edge** toward the clear path; then when the leading sector is safe, resume normal cruise and **Normal** cleaning per `travelToggle`. *(legacy **FR-005**, **FR-006**, **FR-007**)* |
| OBJ4-FR-4 | When **body-fixed** `frontObstacle` ∧ `leftObstacle` ∧ `rightObstacleInferred` is true per fusion/probe policy, the Software Controller SHALL treat the robot as **surrounded** and SHALL **not** command normal-cruise travel along the **leading travel sector** until resolved. **Back** obstacle state SHALL **not** participate in this surrounded condition. *(legacy **FR-008**)* |
| OBJ4-FR-5 | When **OBJ4-FR-4** first holds, the Software Controller SHALL start a **surrounded-escape reverse segment** and SHALL command **continuous reverse** in that segment **until** a **lateral opening** exists (**left** direct sensing and/or **right** probe result indicates clearance to **begin a lateral turn** per fusion/probe — thresholds **TBD**) **or** **maximum backup** per attempt **TBD** is consumed without such an opening. **During this segment**, the Software Controller **SHALL NOT** end reverse **solely** because **forward** reads clear if **neither** **left** nor **right** yet satisfies that lateral-opening predicate — **including** when that forward-only clearance causes the **front∧left∧right** surrounded condition of **OBJ4-FR-4** to become false **only** because **forward** cleared. The surrounded-escape reverse segment SHALL **not** change `travelToggle`, SHALL **not** use **back** as the **leading travel sector**, and SHALL **not** be replaced by a 180-degree turn followed by forward travel. Right-side probe during this segment SHALL use the **front** sensor when `travelToggle` is Forward and the **back** sensor when `travelToggle` is Backward (**OBJ3-FR-5**). When **maximum backup** is reached without a lateral opening, execute **fallback** **TBD**. *(legacy **FR-009**, **NFR-003**)* |
| OBJ4-FR-6 | When the **OBJ4-FR-5** surrounded-escape reverse **segment** ends because a **lateral opening** exists, the Software Controller SHALL execute a **lateral turn** using the same **prefer-right** rule as **OBJ4-FR-3**: if **both** sides are open for a turn, **prefer right** when the right-side probe confirms viability else **left**; if only **one** side is open, turn **toward** that side. The turn SHALL orient the **leading travel edge** (per preserved `travelToggle`) toward the opening. When the leading travel sector is thereafter safe, the Software Controller SHALL resume normal cruise along that leading sector and **Normal** cleaning: **forward** motion command when `travelToggle` is **Forward** (front leads out), **reverse** motion command when `travelToggle` is **Backward** (back leads out). `travelToggle` SHALL remain **unchanged** by surrounded escape. If the segment ends **only** because **maximum backup** was reached (**no** lateral opening), behavior is defined solely by the **fallback** in **OBJ4-FR-5** (**TBD**). *(legacy **FR-010**)* |
| OBJ4-FR-7 | The Software Controller SHALL NOT emit contradictory motion commands in the same control cycle (e.g., forward and reverse together). *(legacy **NFR-002**)* |
| OBJ4-FR-8 | A right-side probe SHALL be represented as a temporary high-level maneuver distinct from the final avoidance/escape turn. The system SHALL use the probe to evaluate right-side clearance and then execute the selected avoidance/escape command; the probe itself SHALL NOT be counted as the required surrounded-escape backward movement. |
| OBJ4-FR-9 | Obstacle avoidance, right-side probe, and surrounded escape SHALL take **priority** over dust-maneuver initiation. Dust handling SHALL begin only during **normal cruise** when no higher-priority maneuver is active and the robot is **not** in a surrounded-escape reverse segment. |
| OBJ4-FR-10 | When `dustSignal` exceeds threshold **TBD** and **OBJ4-FR-9** permits, the Software Controller SHALL enter a **dust maneuver**: command **stop**, then rotate **540°** — **clockwise** if `travelToggle` is **Forward**, **counter-clockwise** if **Backward** — while commanding **Boost** cleaning per **§3.2.2**. |
| OBJ4-FR-11 | If `dustSignal` remains above threshold **TBD** after a **540°** spin in a dust maneuver, the Software Controller SHALL execute **another** **540°** spin in the **same** direction (clockwise for Forward toggle, counter-clockwise for Backward toggle) and continue **Boost** cleaning until dust clears. |
| OBJ4-FR-12 | When a dust maneuver completes because `dustSignal` falls below threshold **TBD**, the Software Controller SHALL **toggle** `travelToggle` (Forward ↔ Backward), return to **Normal** cleaning, and resume normal cruise in the new toggle direction such that the RVC’s **world travel direction** (as seen by an external observer) is **unchanged** from immediately before the dust maneuver began. |
| OBJ4-FR-13 | `travelToggle` SHALL remain unchanged between dust-maneuver completions and SHALL **only** be toggled by the system upon dust-maneuver completion per **OBJ4-FR-12**. Surrounded-escape reverse segments SHALL **not** toggle `travelToggle`. |

#### 3.2.4.3 Messages

| Direction | Message | Description |
|-----------|---------|---------------|
| Received | `FusedObstacleSnapshot` | From **ObstaclePerceptionContext** (§3.2.3) |
| Sent | `MotionCommand` | Forward / reverse / turn / right-side probe re-orientation / dust-maneuver spin |
| Sent | `SuspendCleaningForManeuver` / `ResumeCleaningAfterManeuver` | To **SurfaceCleaningController** (§3.2.2) — obstacle and escape maneuvers only |
| Sent | `StartBoostCleaning` / `EndBoostCleaning` | To **SurfaceCleaningController** (§3.2.2) — dust maneuver |
| Sent | *(internal)* | `ToggleTravelDirection` — toggles `travelToggle` on dust-maneuver completion |

---

## 3.3 Performance requirements

| ID | Requirement |
|----|-------------|
| PERF-1 | From **validated** obstacle state change to issuing the corresponding maneuver command, latency SHALL NOT exceed **TBD** milliseconds (per sector budgets may differ — **TBD**). *(legacy **NFR-004**)* |
| PERF-2 | The decision task SHALL run at minimum rate **TBD** Hz **or** shall react to equivalent pushed updates without violating PERF-1. *(legacy **NFR-005**)* |
| PERF-3 | Transient sensor noise SHALL be filtered so steady-state tests do not false-trigger avoidance or surrounded escape beyond defined debounce **TBD**. *(legacy **NFR-006**)* |
| PERF-4 | The right-side probe SHALL complete within a bounded time **TBD** or the system SHALL enter safe behavior / fallback rather than assume right-side clearance. |
| PERF-5 | The **540°** dust-maneuver spin SHALL complete within a bounded time **TBD** or the system SHALL enter safe behavior per **OBJ3-FR-4**. |

## 3.4 Design constraints

| ID | Constraint |
|----|------------|
| DC-1 | This SRS SHALL NOT be interpreted to require a specific programming language; organizational choice (e.g., C++) is **TBD**. |
| DC-2 | Direct front/left/back obstacle acquisition (interrupt vs poll vs other) is **not** a design constraint imposed by this SRS. Back-sensor use is constrained to toggled-backward normal cruise per **HI-1B**. Right-side probe implementation details remain out of scope except for the required behavior in §3.2. |
| DC-3 | Standards compliance (safety, EMC, etc.) **TBD** by product line. |

## 3.5 Software system attributes

| ID | Attribute type | Requirement |
|----|----------------|-------------|
| REL-1 | Reliability | After recoverable sensor dropout, the system SHALL follow a **defined recovery sequence** **TBD** without user intervention where feasible. *(legacy **NFR-007**)* |
| MAINT-1 | Maintainability | Obstacle thresholds, debounce windows, right-side probe parameters, max backup, dust threshold and **540°** spin parameters, `travelToggle` policy, and latency budgets SHALL be **configurable** (compile-time or runtime) per **NFR-011** legacy intent. |
| VER-1 | Verifiability | §3.2 behaviors SHALL be testable via simulation or HIL with **injected** obstacle and dust timelines. *(legacy **NFR-012**)* |
| RES-1 | Resource usage | CPU and memory budgets **TBD** for target platform. *(legacy **NFR-008**)* |
| SAFE-1 | Safe state | See **OBJ3-FR-4** and legacy **NFR-001**. |

## 3.6 Other requirements

| ID | Requirement |
|----|-------------|
| OTH-1 | Each **TBD** in this SRS SHALL be tracked in project configuration management with owner and target resolution date (IEEE 830 completeness practice). |
| OTH-2 | Requirements SHALL remain **backward-traceable** to preliminary goals and **forward-traceable** to design/test artifacts (matrices **TBD**). *(legacy **NFR-010**)* |
| OTH-3 | Detailed hardware control (PWM, drivers, mechanical design) remains **out of scope**; see legacy **FR-014**. |
| OTH-4 | Design artifacts, code, tests, and simulator scenarios SHALL model the absence of a dedicated right-side obstacle sensor and SHALL trace right-side decisions to the right-side probe behavior introduced in v0.6.0. |
| OTH-5 | Design artifacts, code, tests, and simulator scenarios SHALL distinguish **toggled-backward normal cruise**, **surrounded-escape reverse**, and **dust maneuver** motion; SHALL model **back** sensor use per **HI-1B**; and SHALL implement **travel-direction toggle** and **540°** dust spin behavior per §3.2. |

---

# Appendix A — Change history

| Version | Date | Summary |
|---------|------|---------|
| **0.7.2** | **2026-06-19** | Right-side probe sensor follows `travelToggle`: **front** when Forward, **back** when Backward (including during surrounded escape). Back no longer blanket-ignored during surrounded-escape reverse — ignored only as **leading sector**; still used for probe when toggled Backward. Dust maneuver still ignores back entirely. |
| **0.7.1** | **2026-06-19** | Clarified that `travelToggle` **swaps the leading travel edge** (front vs back). Obstacle avoidance and surrounded-escape exit resume travel along the **leading travel sector** per toggle — not always physical forward. Added **Leading travel sector** definition; updated **OBJ4-FR-3**, **OBJ4-FR-4**, **OBJ4-FR-6**, **HI-1B**. |
| **0.7.0** | **2026-06-19** | Added **back** obstacle sensor (toggled-backward cruise only). Introduced **Normal** vs **Boost** cleaning, **travel-direction toggle**, and **dust maneuver** (540° spin, Boost clean, toggle on completion). Replaced prior bounded dust-boost policy. Clarified surrounded-escape reverse vs toggled-backward cruise; back sensor ignored during dust maneuver and surrounded escape. Right-side probe unchanged. |
| **0.6.0** | **2026-06-02** | Removed the dedicated right-side obstacle sensor assumption. Added right-side probe behavior using available sensing/re-orientation, preserved the same front+left+right surrounded meaning, and clarified that surrounded escape requires backward movement rather than a 180-degree turn followed by forward travel. |
| **0.5.6** | **2026-05-16** | Added design-trace note for class/sequence diagrams and aligned **AutomaticCleaningSession** message list with UC-01/UC-08 SDs: `ResumeSession` and `requestServiceOrReset` (**TBD** policies). |
| 0.1–0.4 | 2026-05-15 | Prior flat SRS (FR/NFR numbering); iterative refinements (Roomba name, sensors, prefer-right, backup-until-turn, acquisition agnostic). |
| **0.5.5** | **2026-05-15** | **OBJ4-FR-5** / **UC-05** step 3: **surrounded-escape reverse segment** continues through **forward-only** clear if sides still closed (avoids tying reverse to “while surrounded” boolean after **front** drops). |
| **0.5.4** | **2026-05-15** | **Surrounded escape rule:** reverse until **lateral opening** (left **or** right); **forward-only** clearance insufficient while both sides closed (**OBJ4-FR-5**, **OBJ4-FR-6**); new term **Lateral opening** (§1.3); **§2.2**, **A-5**, **§2.6** updated. |
| **0.5.3** | **2026-05-15** | **Assumption A-5** and **§2.6** note: long tube / symmetric corridor where forward “opens” but no true exit — **global** escape **TBD** (mapping, stuck heuristics, odometry). |
| **0.5.2** | **2026-05-15** | **Correspondence / clarity:** **OBJ1-FR-1** and navigation messages use explicit §3.2.x object names (replaced informal **OBJ-2** / **OBJ-3**); **OBJ4-FR-3** suspend-cleaning target named **SurfaceCleaningController**. |
| **0.5.1** | **2026-05-15** | **Correspondence fixes:** **OBJ4-FR-3** referenced “not surrounded” vs **OBJ4-FR-4** (was incorrect **OBJ4-FR-6**); **OBJ1-FR-2** section range **§3.2.2–§3.2.4** (was non-existent **§3.2.3–§3.2.5**); **UI-2** cross-ref to **§3.2.1** (was **OBJ-1**). |
| **0.5** | **2026-05-15** | **Restructured** to **IEEE Std 830-1998** document outline and **Annex A.4** (Section 3 by **object**); legacy IDs mapped in Appendix B. |

---

# Appendix B — Traceability (legacy requirement IDs)

| Legacy ID | New location (primary) |
|-----------|-------------------------|
| FR-001 | OBJ2-FR-1 |
| FR-002 | OBJ2-FR-2 |
| FR-003 | OBJ4-FR-1 |
| FR-004 | OBJ4-FR-2 |
| FR-005 | OBJ2-FR-3, OBJ4-FR-3 |
| FR-006 | OBJ4-FR-3 |
| FR-007 | OBJ4-FR-3 |
| FR-008 | OBJ4-FR-4 |
| FR-009 | OBJ4-FR-5 |
| FR-010 | OBJ4-FR-6 |
| FR-011 | OBJ2-FR-4, OBJ4-FR-10 |
| FR-012 | OBJ2-FR-4, OBJ4-FR-10, OBJ4-FR-11 |
| FR-013 | OBJ2-FR-5, OBJ4-FR-12 |
| FR-014 | §3.6 OTH-3 |
| FR-015 | OBJ1-FR-1 |
| FR-016 | OBJ1-FR-2 |
| FR-017 | OBJ3-FR-1 |
| FR-018 | OBJ3-FR-2 |
| FR-019 | OBJ3-FR-3, OBJ3-FR-5 |
| NFR-001 | OBJ3-FR-4, §3.5 SAFE-1 |
| NFR-002 | OBJ4-FR-7 |
| NFR-003 | OBJ4-FR-5 |
| NFR-004 | PERF-1 |
| NFR-005 | PERF-2 |
| NFR-006 | PERF-3 |
| NFR-007 | REL-1 |
| NFR-008 | RES-1 |
| NFR-009 | *(covered by OBJ4-FR-10–**OBJ4-FR-11** dust spin repeat — energy policy **TBD**)* |
| NFR-010 | OTH-2 |
| NFR-011 | MAINT-1 |
| NFR-012 | VER-1 |

---

# Appendix C — Human-readable change summary for v0.6.0

## Changed

- The SRS no longer assumes a dedicated right-side obstacle sensor.
- The right side is still part of the behavior model, but its blocked/clear state is now inferred through a high-level right-side probe using available sensing, such as rotating the robot so the front obstacle sensor can inspect the right side.
- Surrounded still means **front**, **left**, and **right** are blocked. The right part of that condition is now based on the right-side probe result.
- Prefer-right avoidance now means “prefer right only after the right-side probe confirms the right turn is viable.”
- Combined obstacle decisions now combine direct front/left data with the right-side probe result.
- Configurability and performance requirements now include right-side probe timing and parameters.
- Probe-pose observations are represented through `ObstacleStateChanged(probePose=right, ...)` instead of a separate right-side sensor message.

## Added

- A new definition for **Right-side probe**.
- New hardware-interface requirement **HI-1A** for deriving right obstacle status without a dedicated right sensor.
- New obstacle-perception attributes `rightObstacleInferred` and `rightProbeStatus`.
- New functional requirement **OBJ3-FR-5** requiring the software to determine right-side blockage through a high-level probe when right-side knowledge is needed.
- New navigation requirement **OBJ4-FR-8** separating the right-side probe from the final avoidance/escape maneuver.
- New performance requirement **PERF-4** requiring the right-side probe to complete within a bounded time or fall back safely.
- New traceability requirement **OTH-4** requiring downstream design, code, tests, and simulator artifacts to reflect the missing right sensor.

## Deleted / Removed

- Removed the requirement that the Software Controller consume direct right obstacle state as a logical input.
- Removed the assumption that front/left/right obstacle states are all obtained directly from sensors.
- Removed wording that treated “right sensor-valid” as direct right-side sensing; right viability now comes from the probe.
- Removed the separate `RightSideProbeResult` received message name; the probe observation is carried as an `ObstacleStateChanged` event taken in the probe pose.

## Preserved Behavior

- The RVC automatically cleans/mops household surfaces.
- It goes straight forward while cleaning when no maneuver applies.
- It stops/suspends cleaning when avoiding an obstacle, turns left or right, then resumes forward cleaning.
- If front, left, and right are blocked, it moves backward first, then turns aside left or right, then goes forward.
- Dust detection still increases cleaning power for a bounded period.
- Detailed hardware control remains out of scope.
- The SRS still focuses only on automatic cleaning behavior.

---

# Appendix D — Human-readable change summary for v0.7.0

## Changed

- Session start now explicitly initializes **Normal** cleaning (on, normal power) and **Forward** travel toggle.
- Default cruise follows `travelToggle` (forward or reverse command), not always forward.
- Leading-sector avoidance uses **front** when Forward and **back** when toggled Backward.
- Dust handling **replaces** the prior bounded power-boost policy with a full **dust maneuver** sequence.
- Cleaning modes renamed/clarified: **Normal**, **Boost**, **Suspended**.
- Surrounded escape explicitly **does not** use the back sensor and **does not** change `travelToggle`.

## Added

- **Back** obstacle sensor (**HI-1B**, **OBJ3-FR-6**) — active only during toggled-backward normal cruise.
- **Travel-direction toggle** (`travelToggle`) — system toggles on dust-maneuver completion; persistent until next dust event.
- **Dust maneuver** — stop, **540°** spin (CW if Forward, CCW if Backward), Boost clean, repeat while dust persists; back sensor ignored during maneuver.
- Definitions: **Normal** / **Boost** cleaning, **toggled-backward normal cruise**, **surrounded-escape reverse segment**, **dust maneuver**.
- Requirements **OBJ1-FR-4**, **OBJ2-FR-6**, **OBJ3-FR-6**, **OBJ3-FR-7**, **OBJ4-FR-9** through **OBJ4-FR-13**, **PERF-5**, **OTH-5**.

## Deleted / Removed

- Removed unqualified “dust boost for bounded duration” behavior (**OBJ2-FR-4** / **OBJ2-FR-5** prior wording).
- Removed assumption that only front and left are directly sensed (**A-1** now includes back when applicable).

## Preserved Behavior

- Right-side probe and **prefer-right** avoidance unchanged from v0.6.0.
- Surrounded condition still **front ∧ left ∧ right** (right from probe).
- Surrounded escape still reverses until lateral opening, then lateral turn, then resume along the **leading travel sector** per `travelToggle`.
- Obstacle maneuvers take priority over dust handling.
- Detailed hardware control remains out of scope.

---

# Appendix E — Human-readable change summary for v0.7.1

## Changed

- `travelToggle` now explicitly **swaps front and back** as the leading travel edge (toggle Backward ⇒ back is travel-forward).
- **OBJ4-FR-6** surrounded-escape exit no longer always resumes physical forward; it resumes along the **leading travel sector** (front-led forward when Forward toggle, back-led reverse when Backward toggle).
- **OBJ4-FR-3** and **OBJ4-FR-4** aligned to **leading travel sector** wording for consistency with toggled-backward avoidance and escape.

## Added

- Definition **Leading travel sector**.

## Preserved Behavior

- Surrounded **trigger** remains body-fixed front ∧ left ∧ right (right from probe).
- Surrounded-escape **reverse segment** unchanged; back not used as **leading sector** during that segment (v0.7.2 adds back for probe when toggled Backward).
- Prefer-right probe and lateral-turn logic unchanged.

---

# Appendix F — Human-readable change summary for v0.7.2

## Changed

- **Right-side probe** now explicitly uses **front** sensor when `travelToggle` is Forward and **back** sensor when Backward — including during **UC-05 / surrounded escape**.
- **OBJ3-FR-7** / **HI-1B**: back is **not** blanket-ignored during surrounded-escape reverse; ignored only as **leading travel sector**, not for probe when toggled Backward.

## Preserved

- Dust maneuver still **fully ignores** back sensor.
- Surrounded trigger still body-fixed front + left + right (right from probe).

---

*End of SRS*
