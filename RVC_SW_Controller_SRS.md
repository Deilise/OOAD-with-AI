# Software Requirements Specification  
## Roomba RVC Software Controller

| Field | Value |
|--------|--------|
| **Document title** | Software Requirements Specification (SRS) |
| **Software product** | Roomba Robotic Vacuum Cleaner (RVC) — **Software Controller** (automatic cleaning behavior) |
| **Organization / project** | *(TBD)* |
| **SRS version** | 0.5.6 |
| **Issue date** | 2026-05-15 |
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

---

# 1. Introduction

## 1.1 Purpose

This SRS delineates **what** the Roomba RVC **Software Controller** shall do to implement **automatic cleaning**: session control, surface cleaning behavior, use of obstacle information (front / left / right), navigation and escape maneuvers (including **prefer-right** avoidance and, when surrounded, **reverse until a lateral opening** — **left** or **right** — then lateral escape), and dust-driven cleaning power.  

**Intended audience:** product owners, systems engineers, software architects, implementers (e.g., C++ developers), verification engineers, and maintainers.

## 1.2 Scope

**In scope**

- Software-visible **behavior** for automatic vacuuming/mopping sessions: default forward cleaning, obstacle reactions, surrounded escape, dust boost, and safe handling of bad/stale sensor data at the behavioral level.
- **Logical** inputs (obstacle state per sector, dust signal) and **logical** outputs (high-level motion and cleaning commands). Detailed motor control, register-level drivers, and electrical interfaces are **out of scope** for this SRS (see §2.4).

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
| **Cleaning (active)** | Vacuum and/or mop treatment is commanded **on** for the current segment of operation |
| **Cleaning (suspended)** | Treatment is commanded **off** while the robot performs a maneuver (e.g., after forward obstacle) |
| **Obstacle (sector)** | Indication that **front**, **left**, or **right** is blocked or too close per product thresholds |
| **Obstacle state (per sector)** | Fused or raw obstacle/clearance for a sector, as supplied to the Software Controller; **how** it is obtained (poll, interrupt, message, etc.) is **not** prescribed by this SRS |
| **Prefer-right** | When a lateral turn is needed, the system **SHALL** prefer **turning right** if a right turn is **sensor-valid**; otherwise **turn left** |
| **Lateral opening** | During **surrounded escape** (§3.2.4 **OBJ4-FR-4**–**OBJ4-FR-6**), fused obstacle state where **left** **OR** **right** (or **both**) indicates enough clearance to **begin a lateral turn** (thresholds **TBD**). **Forward-only** clearance **does not** satisfy this while **both** sides remain “closed” for a lateral start per fusion. |
| **HAL** | Hardware abstraction layer (optional); not mandated by this SRS |
| **TBD** | To be determined — see IEEE 830 guidance: each TBD **SHALL** be resolved with owner and target date in project records |

## 1.4 References

| ID | Document | Note |
|----|-----------|------|
| R-1 | IEEE Std 830-1998, *IEEE Recommended Practice for Software Requirements Specifications* | Template structure (Annex **A.4**, Section 3 by object) |
| R-2 | `RVC_SW_Controller_UseCases.md` (same workspace) | Use-case companion; align revision with SRS **v0.5.5** |
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

- **Higher-level inputs:** session commands (start/stop automatic cleaning), and **logical** obstacle and dust signals from firmware, drivers, or a sensor service (**implementation-defined** delivery).
- **Lower-level consumers:** motion and cleaning subsystems consume **high-level commands** (e.g., forward, turn left/right, reverse, cleaning intensity on/off); mapping to actuators is **outside** this SRS.

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

1. **Session control** — start/stop automatic cleaning session.  
2. **Default cruise** — straight forward while cleaning when no maneuver applies.  
3. **Forward obstacle avoidance** — suspend cleaning; **prefer-right** lateral turn; resume forward and cleaning.  
4. **Surrounded escape** — no forward while front+left+right blocked; **reverse until a lateral opening** (**left** and/or **right**); then **prefer-right** lateral turn; **forward-only** clearance while **both** sides remain closed **does not** end the surrounded-escape reverse **segment**; **maximum backup** + **fallback** if no lateral opening (**TBD**).  
5. **Dust boost** — increase cleaning power for a **bounded** time or until signal clears per policy.  
6. **Safe degradation** — conservative behavior on missing/invalid/stale obstacle data.  
7. **Consistent multi-sector decisions** — fusion/snapshot rules for combined front/left/right state.

## 2.3 User characteristics

Operators are **home users** or **technicians**; no specialized training is assumed for routine automatic cleaning. Session start/stop is expected via product UI (exact UI **TBD**). This SRS does **not** specify screen layouts.

## 2.4 Constraints

- **Behavioral focus only:** the SRS SHALL **not** mandate module partitioning, class diagrams, or scheduling algorithms beyond externally visible behavior and stated performance bounds.
- **No prescribed sensor acquisition:** front/left/right obstacle signaling MAY use any combination of polling, interrupts, or messages, provided §3.2 and §3.3 are satisfied.
- **Implementation language** (e.g., C++) is **TBD** at organizational level; not a behavioral requirement of the product behavior itself.
- Regulatory, safety certification, and standards compliance beyond this document are **TBD**.

## 2.5 Assumptions and dependencies

- **A-1:** Three obstacle sectors (**front**, **left**, **right**) are meaningful for the deployed sensor geometry; exact zones **TBD**.  
- **A-2:** A **dust** or debris-load signal exists and can be thresholded; definition **TBD**.  
- **A-3:** A lower layer delivers obstacle state with bounded latency once hardware is fixed; numeric targets **TBD** (see §3.3).  
- **A-4:** Mopping vs vacuum interaction (parallel vs sequential) **TBD**.  
- **A-5:** **Local obstacle sectors alone** cannot always distinguish a **true exit** from a **long straight opening** with no global way out. Surrounded escape therefore **SHALL NOT** end its reverse **segment** on **forward-only** clearance while **both** **left** and **right** remain insufficient for a lateral start (**OBJ4-FR-5**). **Global** disambiguation for all geometries remains deferred (see §2.6).  
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

| HI-1 | The Software Controller SHALL consume **obstacle state** for **front**, **left**, and **right** as logical inputs (boolean or enumerated proximity per design). |
| HI-2 | The Software Controller SHALL consume a **dust / debris load** signal (scalar or level per design). |
| HI-3 | The Software Controller SHALL output **high-level motion commands** (e.g., drive forward, reverse, rotate left/right) and **cleaning actuation commands** (on/off/intensity). Physical realization **TBD**. |

### 3.1.3 Software interfaces

| SI-1 | Obstacle and dust inputs SHALL be obtainable from a **sensor interface layer** (name/API **TBD**); acquisition mechanism (poll vs push) is **not** constrained by this SRS. |
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
| OBJ1-FR-2 | When `sessionActive` is **false**, the Software Controller SHALL NOT perform autonomous obstacle avoidance, surrounded escape, dust-boost navigation, or autonomous forward cruise described in §3.2.2 through §3.2.4. *(legacy **FR-016**)* |
| OBJ1-FR-3 | Starting and stopping a session SHALL be deterministic with respect to the session interface (no “half on” state without definition — state machine **TBD** in design). |

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
| AT-2.1 | `cleaningMode` | e.g., `Active`, `Suspended`, `Boosted` — exact enum **TBD** |
| AT-2.2 | `powerLevel` | Normal vs elevated — **TBD** quantization |
| AT-2.3 | `dustSignal` | Input mirror of dust/debris signal |

#### 3.2.2.2 Functions

| ID | Functional requirement |
|----|-------------------------|
| OBJ2-FR-1 | While `sessionActive` is true and no maneuver forces suspension, the Software Controller SHALL command **cleaning active** (vacuum/mop per product configuration). *(legacy **FR-001**)* |
| OBJ2-FR-2 | The system SHALL distinguish **cleaning active** from **cleaning suspended** (e.g., during obstacle maneuver). *(legacy **FR-002**)* |
| OBJ2-FR-3 | When forward obstacle handling requires suspension, the Software Controller SHALL command **cleaning suspended** for the duration of that maneuver. *(legacy **FR-005**)* |
| OBJ2-FR-4 | When `dustSignal` exceeds threshold **TBD**, the Software Controller SHALL command **increased cleaning power** for a **bounded** duration or until hysteresis clears the condition, per policy **TBD**. *(legacy **FR-011**, **FR-012**)* |
| OBJ2-FR-5 | After boosted cleaning, the Software Controller SHALL return to **normal** power unless `dustSignal` still demands boost. *(legacy **FR-013**)* |

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
| AT-3.3 | `rightObstacle` | Right sector |
| AT-3.4 | `lastUpdateTime` (per sector or fused) | For staleness — structure **TBD** |

#### 3.2.3.2 Functions

| ID | Functional requirement |
|----|-------------------------|
| OBJ3-FR-1 | The Software Controller SHALL obtain **front** obstacle state with latency sufficient for safe stop/avoidance (numeric budget **TBD**). Acquisition mechanism is **implementation-defined**. *(legacy **FR-017**)* |
| OBJ3-FR-2 | The Software Controller SHALL obtain **left** and **right** obstacle state with freshness sufficient for avoidance and surrounded logic (rate **TBD**, documented in design). *(legacy **FR-018**)* |
| OBJ3-FR-3 | For decisions needing **combined** front, left, and right state, the Software Controller SHALL apply a **consistent snapshot or fusion rule** (ordering, debounce, staleness **TBD**) so behavior is not undefined. *(legacy **FR-019**)* |
| OBJ3-FR-4 | If obstacle data for any required sector is **missing**, **invalid**, or **stale** beyond timeout **TBD**, the system SHALL enter **safe behavior** (e.g., stop motion, suspend autonomous cleaning commands) per **NFR-001** in §3.5. |

#### 3.2.3.3 Messages

| Direction | Message | Description |
|-----------|---------|---------------|
| Received | `ObstacleStateChanged` | Push notification **or** periodic read completes — **implementation-defined** |
| Sent | `FusedObstacleSnapshot` | To **NavigationAndEscapeCoordinator** |

---

### 3.2.4 Object: **NavigationAndEscapeCoordinator**

#### 3.2.4.1 Attributes

| ID | Attribute | Description |
|----|-----------|-------------|
| AT-4.1 | `motionState` | e.g., `ForwardCruise`, `Avoiding`, `Reversing`, `Turning` — **TBD** |
| AT-4.2 | `backupDistanceRemaining` | For bounded reverse — **TBD** units |

#### 3.2.4.2 Functions

| ID | Functional requirement |
|----|-------------------------|
| OBJ4-FR-1 | While cleaning is active (not suspended) and no higher-priority maneuver applies, the Software Controller SHALL command **straight forward** motion. *(legacy **FR-003**)* |
| OBJ4-FR-2 | After completing a turn or backup escape, the Software Controller SHALL prefer **forward** motion when safe. *(legacy **FR-004**)* |
| OBJ4-FR-3 | When forward is blocked/unsafe but **not** surrounded (i.e., the condition of **OBJ4-FR-4** does **not** hold per fusion policy), the Software Controller SHALL initiate avoidance: **suspend cleaning** (via message to **SurfaceCleaningController**, §3.2.2), then execute a **lateral turn**, **preferring right** if sensor-valid, else **left**; then when forward is safe, resume forward and **resume cleaning**. *(legacy **FR-005**, **FR-006**, **FR-007**)* |
| OBJ4-FR-4 | When `frontObstacle` ∧ `leftObstacle` ∧ `rightObstacle` is true per fusion policy, the Software Controller SHALL **not** command forward motion until resolved. *(legacy **FR-008**)* |
| OBJ4-FR-5 | When **OBJ4-FR-4** first holds, the Software Controller SHALL start a **surrounded-escape reverse segment** and SHALL command **continuous reverse** in that segment **until** a **lateral opening** exists (**left** and/or **right** indicates clearance to **begin a lateral turn** per fusion — thresholds **TBD**) **or** **maximum backup** per attempt **TBD** is consumed without such an opening. **During this segment**, the Software Controller **SHALL NOT** end reverse **solely** because **forward** reads clear if **neither** **left** nor **right** yet satisfies that lateral-opening predicate — **including** when that forward-only clearance causes the **front∧left∧right** conjunction of **OBJ4-FR-4** to become false **only** because **forward** cleared. When **maximum backup** is reached without a lateral opening, execute **fallback** **TBD**. *(legacy **FR-009**, **NFR-003**)* |
| OBJ4-FR-6 | When the **OBJ4-FR-5** surrounded-escape reverse **segment** ends because a **lateral opening** exists, the Software Controller SHALL execute a **lateral turn** using the same **prefer-right** rule as **OBJ4-FR-3**: if **both** sides are open for a turn, **prefer right** when sensor-valid else **left**; if only **one** side is open, turn **toward** that side. When forward is thereafter safe, the Software Controller SHALL resume **forward** motion and **cleaning**. If the segment ends **only** because **maximum backup** was reached (**no** lateral opening), behavior is defined solely by the **fallback** in **OBJ4-FR-5** (**TBD**). *(legacy **FR-010**)* |
| OBJ4-FR-7 | The Software Controller SHALL NOT emit contradictory motion commands in the same control cycle (e.g., forward and reverse together). *(legacy **NFR-002**)* |

#### 3.2.4.3 Messages

| Direction | Message | Description |
|-----------|---------|---------------|
| Received | `FusedObstacleSnapshot` | From **ObstaclePerceptionContext** (§3.2.3) |
| Sent | `MotionCommand` | Forward / reverse / turn |
| Sent | `SuspendCleaningForManeuver` / `ResumeCleaningAfterManeuver` | To **SurfaceCleaningController** (§3.2.2) |

---

## 3.3 Performance requirements

| ID | Requirement |
|----|-------------|
| PERF-1 | From **validated** obstacle state change to issuing the corresponding maneuver command, latency SHALL NOT exceed **TBD** milliseconds (per sector budgets may differ — **TBD**). *(legacy **NFR-004**)* |
| PERF-2 | The decision task SHALL run at minimum rate **TBD** Hz **or** shall react to equivalent pushed updates without violating PERF-1. *(legacy **NFR-005**)* |
| PERF-3 | Transient sensor noise SHALL be filtered so steady-state tests do not false-trigger avoidance or surrounded escape beyond defined debounce **TBD**. *(legacy **NFR-006**)* |

## 3.4 Design constraints

| ID | Constraint |
|----|------------|
| DC-1 | This SRS SHALL NOT be interpreted to require a specific programming language; organizational choice (e.g., C++) is **TBD**. |
| DC-2 | Obstacle acquisition (interrupt vs poll vs other) is **not** a design constraint imposed by this SRS. |
| DC-3 | Standards compliance (safety, EMC, etc.) **TBD** by product line. |

## 3.5 Software system attributes

| ID | Attribute type | Requirement |
|----|----------------|-------------|
| REL-1 | Reliability | After recoverable sensor dropout, the system SHALL follow a **defined recovery sequence** **TBD** without user intervention where feasible. *(legacy **NFR-007**)* |
| MAINT-1 | Maintainability | Obstacle thresholds, debounce windows, max backup, dust boost duration, and latency budgets SHALL be **configurable** (compile-time or runtime) per **NFR-011** legacy intent. |
| VER-1 | Verifiability | §3.2 behaviors SHALL be testable via simulation or HIL with **injected** obstacle and dust timelines. *(legacy **NFR-012**)* |
| RES-1 | Resource usage | CPU and memory budgets **TBD** for target platform. *(legacy **NFR-008**)* |
| SAFE-1 | Safe state | See **OBJ3-FR-4** and legacy **NFR-001**. |

## 3.6 Other requirements

| ID | Requirement |
|----|-------------|
| OTH-1 | Each **TBD** in this SRS SHALL be tracked in project configuration management with owner and target resolution date (IEEE 830 completeness practice). |
| OTH-2 | Requirements SHALL remain **backward-traceable** to preliminary goals and **forward-traceable** to design/test artifacts (matrices **TBD**). *(legacy **NFR-010**)* |
| OTH-3 | Detailed hardware control (PWM, drivers, mechanical design) remains **out of scope**; see legacy **FR-014**. |

---

# Appendix A — Change history

| Version | Date | Summary |
|---------|------|---------|
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
| FR-011 | OBJ2-FR-4 |
| FR-012 | OBJ2-FR-4 |
| FR-013 | OBJ2-FR-5 |
| FR-014 | §3.6 OTH-3 |
| FR-015 | OBJ1-FR-1 |
| FR-016 | OBJ1-FR-2 |
| FR-017 | OBJ3-FR-1 |
| FR-018 | OBJ3-FR-2 |
| FR-019 | OBJ3-FR-3 |
| NFR-001 | OBJ3-FR-4, §3.5 SAFE-1 |
| NFR-002 | OBJ4-FR-7 |
| NFR-003 | OBJ4-FR-5 |
| NFR-004 | PERF-1 |
| NFR-005 | PERF-2 |
| NFR-006 | PERF-3 |
| NFR-007 | REL-1 |
| NFR-008 | RES-1 |
| NFR-009 | *(covered by OBJ2-FR-4 bounded boost — energy policy **TBD**)* |
| NFR-010 | OTH-2 |
| NFR-011 | MAINT-1 |
| NFR-012 | VER-1 |

---

*End of SRS*
