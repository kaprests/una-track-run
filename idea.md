# Track Mode App (indoor pace without GPS)

## Problem
Indoor tracks have no GPS signal, so the UNA Watch can't show live pace or distance. The goal is accurate live pace on an indoor track (typically 200 m) using only the watch.

## Concept
- Live pace = **cadence × stride length**.
- Cadence comes from the watch accelerometer.
- Stride length comes from a personal model, re-anchored on every lap of known length.
- The user sets the lap length (e.g. 200 m) and optionally the lane.

## Calibration

### Initial prior from historical FIT files
- Use outdoor runs that have both GPS speed and cadence.
- Keep only steady, flat sections with good GPS (filter on speed variance, elevation change and GPS quality).
- Fit stride length as a function of cadence, e.g. `stride = a + b·cadence` or a low-order polynomial. Stride length grows with speed, so a constant is not enough.
- Do this on PC/phone (e.g. the `performance_stats_app` data_analysis tooling) and push the coefficients to the app as settings.

### Online update per lap
- Treat the prior as a starting estimate and correct it after each lap using the known lap distance.
- Use a Kalman-style filter or weighted moving average on a correction factor (or on the model coefficients).
- The first laps absorb the day's conditions: shoes, fatigue, the tight indoor bends.
- Store the converged correction per track as the prior for the next session.

## Lap detection
1. **Manual:** button press each lap (MVP).
2. **Automatic:** gyro/magnetometer turn signature. A 200 m track has two 180° bends per lap, so counting full 360° of rotation gives laps. Optionally combine with the expected lap time from the pace model to reject false detections.

## Pitfalls
- **Lane offset:** each lane outward adds ~2π × lane width (≈ 6–7 m per lap on 200 m, ≈ 3%). Crowded tracks mean frequent lane 2 passing. Options: pick a lane, or estimate an average offset.
- **Bends:** stride is shorter on tight indoor bends than in the outdoor data. Lap anchoring compensates on average.
- **GPS noise in history:** handled by strict segment filtering and long steady segments.
- **Fatigue drift:** stride shortens late in sessions. The per-lap update tracks it.
- **Pace changes inside a lap (intervals):** the cadence model reacts instantly, and the lap correction only adjusts the scale.

## Open questions
- Can an app read historical FIT files on the watch, or must the calibration be computed off-watch?
- Which raw sensors (accelerometer, gyro, magnetometer) and cadence data does the SDK expose to apps, and at what rates?
- Which FIT fields to write (speed, distance, cadence, laps), and how to mark the data as model-derived rather than GPS.
- How to transfer the calibration coefficients to the watch (app settings?).

## Possible extensions
- **Facility timing gates** as exact lap/split anchors instead of manual or gyro laps (see `track_facility_system.md`).
- **External pace sensors** (Stryd or a DIY foot pod) if UNA opens up generic BLE/GATT support (see feature request on the UNA roadmap).
- Treadmill mode using the same model, calibrated against the treadmill's displayed speed.
