# Software Requirements Specification

# DOPE-ASS — Digital Optical Performance Engine Application Software Stack

## Version 0.1 — DRAFT

**Date:** 2026-03-02  
**Language Target:** C++17  
**Primary Platform:** ESP32-P4 @ 400MHz

---

# 1. Introduction

## 1.1 Purpose

**DOPE-ASS** (Digital Optical Performance Engine — Application Software Stack) is the firmware that actually runs the scope. It sits on top of the DOPE ballistic library and owns everything DOPE deliberately ignores: the camera, the display, the reticle, the user interface, the sensor drivers, profiles, ranging modes, and the system state machine.

In short: DOPE does the math, DOPE-ASS does everything else. It collects raw sensor data, normalizes it into `SensorFrame` structs, feeds those into DOPE each cycle, reads back the `FiringSolution`, and puts the hold corrections on the glass.

---

## 1.2 Architectural Position

```
┌───────────────────────────────┐
│        UI / Application       │  ← DOPE-ASS
│  - Profiles                   │
│  - LiDAR mapping              │
│  - Target selection           │
│  - Rendering                  │
├───────────────────────────────┤
│   DOPE (Ballistic Engine)     │  ← DOPE submodule (read-only)
│  - AHRS                       │
│  - Atmosphere                 │
│  - Drag integration           │
│  - Coriolis / Eötvös          │
│  - Spin drift                 │
│  - Cant correction            │
├───────────────────────────────┤
│       Sensor Drivers          │  ← DOPE-ASS
│  - IMU                        │
│  - Magnetometer               │
│  - Barometer                  │
│  - LRF                        │
│  - Encoder                    │
└───────────────────────────────┘
```

DOPE-ASS owns the top and bottom layers. DOPE owns only the middle — the ballistic math.

---

# 2. Hardware Platform

Reference prototype build:

| Component | Part |
|---|---|
| MCU | ESP32-P4 @ 400MHz |
| Camera | IMX477 |
| Lens | Arducam 8–50mm zoom |
| Laser Rangefinder | JRT D09C |
| IMU | ISM330DHCX |
| Magnetometer | RM3100 |
| Barometer | BMP581 |
| Display | 390×390 AMOLED |
| Input | I2C rotary encoder |
| GNSS | Optional external receiver |

---

# 3. DOPE Interface Contract

DOPE-ASS talks to DOPE through these APIs and nothing else. Don't bypass this boundary.

## 3.1 Sensor Ingestion

```cpp
void BCE_Update(const SensorFrame* frame);
```

Called every cycle. DOPE-ASS fills out the `SensorFrame` from its peripheral drivers and hands it off.

## 3.2 Default Overrides

```cpp
struct BCE_DefaultOverrides {
    bool use_altitude;
    float altitude_m;

    bool use_pressure;
    float pressure_pa;

    bool use_temperature;
    float temperature_c;

    bool use_humidity;
    float humidity_fraction;

    bool use_wind;
    float wind_speed_ms;
    float wind_heading_deg;

    bool use_latitude;
    float latitude_deg;
};

void BCE_SetDefaultOverrides(const BCE_DefaultOverrides* defaults);
```

Called at startup (and whenever a profile is loaded) to seed DOPE's default environmental model with the user's saved settings.

## 3.3 Calibration APIs

Physical alignment corrections — run by the user through Calibration Mode — get pushed into DOPE through these:

```cpp
void BCE_SetIMUBias(const float accel_bias[3], const float gyro_bias[3]);
void BCE_SetMagCalibration(const float hard_iron[3], const float soft_iron[9]);
void BCE_SetBoresightOffset(const BoresightOffset* offset);
void BCE_SetReticleMechanicalOffset(float vertical_moa, float horizontal_moa);
void BCE_CalibrateBaro(void);
void BCE_CalibrateGyro(void);
```

## 3.4 FiringSolution Output

DOPE produces a `FiringSolution` every cycle. DOPE-ASS reads it and renders the hold corrections on screen. The full structure:

```cpp
struct FiringSolution {
    uint32_t solution_mode;
    uint32_t fault_flags;
    uint32_t defaults_active;

    float hold_elevation_moa;
    float hold_windage_moa;

    float range_m;
    float horizontal_range_m;
    float tof_ms;
    float velocity_at_target_ms;
    float energy_at_target_j;

    float coriolis_windage_moa;
    float coriolis_elevation_moa;
    float spin_drift_moa;
    float wind_only_windage_moa;
    float earth_spin_windage_moa;
    float offsets_windage_moa;
    float cant_windage_moa;

    float cant_angle_deg;
    float heading_deg_true;

    float air_density_kgm3;
};
```

`defaults_active` carries a flag indicating which latitude source is active. DOPE-ASS uses this to show the user an appropriate confidence indicator (see §9.3).

## 3.5 Operational Range Limit

```cpp
constexpr uint32_t BCE_MAX_RANGE_M = 2500;
```

DOPE's internal buffers cover up to 2500 m. DOPE-ASS can impose a lower ceiling for the current session if needed.

---

# 4. Application Responsibilities

The following are explicitly **not** handled by DOPE. DOPE-ASS owns all of these.

### 4.1 Camera & Optics

- IMX477 driver and frame capture
- Arducam 8–50mm zoom lens control; focal length reported to DOPE via zoom encoder input
- **First focal plane (FFP) reticle** — reticle scales proportionally with zoom so MOA/MIL subtensions remain correct at all magnifications
- **Reticle style:** etched-glass tactical overlay rendered on top of camera feed
- Optical pipeline management at target display refresh rate of **60 fps**

### 4.2 Display & Rendering

- 390×390 AMOLED driver
- Three view modes:
  - **Camera View** — live camera feed with HUD overlay (default operating mode)
  - **Data View** — full solution data readout (all `FiringSolution` fields)
  - **Settings** — menu-driven configuration
- **Always-on HUD elements (Camera View):** range, hold elevation, hold windage, battery level, mode indicator
- Reticle overlay rendered using `hold_elevation_moa` / `hold_windage_moa`
- Fault and diagnostic display using `fault_flags` and `defaults_active`
- Latitude confidence indicator derived from latitude source flag in `defaults_active`
- Brightness control

### 4.3 Target Selection

- **v1: Purely manual.** User aims; crosshair position is the target. No CV or auto-detection.
- Passes a single range and orientation into DOPE
- CV/auto-detection is planned but deferred to a future version; DOPE's interface is unchanged regardless

### 4.4 LiDAR / Point Cloud Processing

- **Out of scope for v1.** No LiDAR hardware in the prototype.
- When added in a future version, DOPE-ASS will process point clouds and extract a single range value to pass to DOPE. DOPE's interface is unchanged.

### 4.5 User Profiles

Profiles are **hierarchical**, not monolithic. Four profile types:

| Profile Type | Contents |
|---|---|
| **Gun** | Sight height, twist rate, caliber, barrel data |
| **Cartridge** | BC, drag model, MV, bullet mass, bullet length |
| **Atmosphere** | Altitude, pressure, temperature, humidity, wind, latitude defaults |
| **Preferences** | Display units (imperial/metric), HUD layout, hold display format (MOA/MIL) |

- Profiles are loaded at startup and pushed to DOPE via `BCE_SetDefaultOverrides`
- Profile selection via encoder-driven menu
- Storage medium: **TBD**
- Maximum profile count: **TBD**

### 4.6 GPS / GNSS Integration

- Attaching an optional GNSS receiver
- Parsing NMEA or satellite data (DOPE does not ingest raw GNSS data)
- Resolving a single latitude value and passing it to DOPE via `BCE_Update` or `BCE_SetDefaultOverrides`
- A manually entered latitude should be used to verify/calibrate GPS accuracy at a known benchmark point

### 4.7 Manual Latitude Entry UI

DOPE supports three latitude sources in priority order (managed by DOPE-ASS):

1. **GPS / GNSS** — DOPE-ASS feeds resolved latitude from attached receiver (highest priority)
2. **Manual entry** — DOPE-ASS provides UI for user to enter latitude explicitly; also serves as calibration reference for the magnetometer estimator
3. **Magnetometer-derived** — DOPE estimates autonomously from dip angle; DOPE-ASS has no action required
4. **Unset** — DOPE-ASS displays diagnostic indicator; Coriolis silently disabled in DOPE

### 4.8 Wireless Communication

**Out of scope for v1.** No BLE, no Wi-Fi, no OTA. Not planned for this version.

### 4.9 Data Logging

Required for v1. Log entries are triggered by:

- Each shot / range event
- Session start and end
- AHRS / calibration state changes

Storage medium and log format: **TBD**

### 4.10 System State Machine

Known application-level states (beyond DOPE's internal IDLE / SOLUTION_READY / FAULT):

- **Camera View** — default operating mode with live feed and HUD
- **Data View** — full solution data readout
- **Settings Menu** — encoder-driven configuration
- **Calibration Mode** — IMU bias, mag cal, boresight, baro calibration workflows

No first-boot wizard. Application launches directly; user configures via menus.

Sleep trigger conditions and wake behavior: **TBD**

---

# 5. Latitude Sensor Presets

The settings UI includes hardware preset selectors for latitude uncertainty — the same pattern used for the LRF and thermometer presets. Picking a preset locks the manual σ field automatically.

- **GPS / GNSS Module** — framework is in place; specific receiver presets will be added as hardware gets qualified
- **Magnetometer (lat est.)** — PNI RM3100 at ±1.5° flat

---

# 6. Units Policy

DOPE speaks SI. DOPE-ASS can show the user whatever unit system they prefer, but it's responsible for converting everything before it goes into `BCE_Update` or `BCE_SetDefaultOverrides`. A few inputs (caliber, twist rate, bullet mass) are exceptions where DOPE handles the conversion internally — those are noted in the table.

| Quantity | Unit |
|---|---|
| Distance / range | meters |
| Velocity | m/s |
| Pressure | Pascals |
| Temperature | °C (DOPE converts to Kelvin internally) |
| Mass | kg |
| Angles (internal) | radians |
| Angles (user-facing) | degrees (convert before passing in) |
| Sight height | mm |
| Caliber | inches (DOPE converts internally) |
| Twist rate | signed inches/turn (DOPE converts internally) |
| Bullet mass | grains (DOPE converts internally) |



---

# 7. Operating Modes

DOPE reports its state via `solution_mode` in `FiringSolution`. Here's what each state means and what DOPE-ASS should do with it:

| Mode | Value | DOPE-ASS Behavior |
|---|---|---|
| `IDLE` | — | Insufficient data; display waiting state, no hold corrections rendered |
| `SOLUTION_READY` | — | Valid solution available; render hold corrections on reticle |
| `FAULT` | — | Required input missing or invalid; display fault indicator, suppress hold corrections |

Hard FAULT is raised by DOPE only for:

- No valid range
- No bullet profile
- Missing muzzle velocity
- Missing BC
- Zero unsolvable
- AHRS unstable
- Severe sensor invalid

Everything else — bad atmosphere data, missing latitude, stale baro — gets handled internally by DOPE using defaults, with bits set in `defaults_active`. DOPE-ASS should surface these as warnings, not hard faults.

---

# 8. Manual Inputs

These are the user-configurable ballistic parameters DOPE-ASS exposes in its settings UI. They persist until the user changes them and flow into DOPE every cycle.

| Parameter | Unit | Notes |
|---|---|---|
| BC | — | Ballistic coefficient |
| Drag model | — | G1–G8 |
| Muzzle velocity | m/s | |
| Bullet mass | grains | |
| Bullet length | mm | |
| Caliber | inches | |
| Twist rate | signed inches/turn | Sign determines drift direction |
| Zero range | m | |
| Sight height | mm | |
| Wind speed | m/s | |
| Wind heading | deg true | |
| Latitude | deg | See §4.7 for source priority |
| Altitude override | m | Optional; ISA default is 0 m |
| Humidity | fraction 0–1 | Optional; ISA default is 0.5 |

DOPE keeps the zero angle in sync automatically — whenever BC, MV, drag model, zero range, sight height, or atmospheric correction changes, it recomputes. DOPE-ASS doesn't need to do anything to trigger this.

---

# 9. Fault & Diagnostic Display

DOPE's output flags are the source of truth for system health. DOPE-ASS translates them into something the user can understand.

## 9.1 Fault Flags (`fault_flags`)

When DOPE is in FAULT, show it clearly and stop rendering hold corrections. If the flag indicates which input is missing, tell the user — don't just show a generic error.

## 9.2 Diagnostic Flags (`defaults_active`)

These are informational — DOPE is running but using defaults somewhere. DOPE-ASS should surface the relevant ones:

- **Latitude unset** — Coriolis is off; show a low-confidence indicator next to any Coriolis/Eötvös values
- **Latitude source** — tell the user which source is active (GPS, manual, or mag-derived) and at what confidence
- **Barometer defaulting** — let the user know atmospheric data is ISA, not live
- **Other active defaults** — a general "some values are estimated" advisory

## 9.3 Latitude Confidence Indicator

| Source | Confidence Display |
|---|---|
| GPS / GNSS | High — verified position |
| Manual entry | Medium — user-provided |
| Magnetometer-derived | Low — ±1–5° typical, ±10° worst case |
| Unset | None — Coriolis disabled |

---

# 10. Input & Navigation

The only physical inputs in v1 are:

| Input | Action |
|---|---|
| Encoder rotate | Scroll through options / adjust values |
| Encoder press | Confirm / select |
| Encoder long-press | Open menu / go back |
| Power button | Power on/off |
| Range trigger button | Fire a manual LRF range shot |

That's it. No touchscreen, no buttons beyond these two, no other inputs.

---

# 11. Ranging Modes

The JRT D09C can run anywhere from 1–16 Hz. DOPE-ASS exposes two modes:

| Mode | Behavior |
|---|---|
| **Manual** | Press the range trigger; the measurement is accepted immediately, no confirmation needed |
| **Continuous** | LRF fires at a user-configured rate; range updates automatically on each valid measurement |

The rate is user-configurable in settings.

---

# 12. Power & Battery

The battery will be a LiPo. Everything else — capacity, connector, charging circuit, low-battery thresholds, and shutdown behavior — is TBD pending hardware decisions.

---

# 13. Zeroing & Calibration UX

## 13.1 Zeroing Procedure

Zeroing is interactive, not just a number field. The user fires at a target, adjusts hold values in-app until the point of impact matches the reticle, then confirms. Under the hood this just updates the ballistic inputs (zero range, sight height, etc.) and DOPE recomputes the zero angle automatically.

## 13.2 Boresight Calibration

Pushes the result into DOPE via `BCE_SetBoresightOffset()`. The step-by-step UX workflow is TBD.

## 13.3 Other Calibration Workflows

All calibration workflows live in Calibration Mode. The API calls that back them:

- IMU bias: `BCE_SetIMUBias()`
- Magnetometer hard/soft iron: `BCE_SetMagCalibration()`
- Barometer zero: `BCE_CalibrateBaro()`
- Gyro zero: `BCE_CalibrateGyro()`

Step-by-step UX for each: TBD.

---

# 14. Performance

DOPE's compute targets on ESP32-P4 @ 400MHz (from DOPE SRS §14):

- 1000 m solution: < 8 ms
- 2500 m solution: < 15 ms
- AHRS update: < 1 ms

The full end-to-end budget from sensor input to updated hold value on screen is TBD — we need hardware in hand to benchmark it. Keeping that latency as low as possible is a core design goal.

---

# 15. Open Items

| Item | Status |
|---|---|
| Profile storage medium | TBD |
| Maximum profile count | TBD |
| Battery capacity | TBD |
| Charging connector | TBD |
| Low battery thresholds / shutdown behavior | TBD |
| Sleep trigger conditions and wake behavior | TBD |
| Full application state machine | TBD |
| Data log format and storage medium | TBD |
| Boresight calibration UX workflow | TBD |
| Other calibration UX workflows (IMU, mag, baro, gyro) | TBD |
| End-to-end display latency budget | TBD (hardware benchmark needed) |

---

End of Document  
DOPE-ASS SRS v0.1
