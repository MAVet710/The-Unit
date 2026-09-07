# FPV Phase 3 — Presentation and USB RC Input

Phase 3 sits on top of the physics/simulation layers from `FPV_FLIGHT_CORE.md`. It does not replace or fork the flight solver.

## Player-facing pawn

`ATU_FPVDronePlayer` subclasses `ATU_FPVDrone` and owns systems that are presentation/input concerns rather than flight physics:

- native FPV OSD lifecycle
- analog/digital video degradation presentation
- USB RC radio adapter
- throttle-safe radio arming guard

The operator now deploys `ATU_FPVDronePlayer` by default. The base `ATU_FPVDrone` remains available as the reusable physics-only pawn.

## Native OSD

`UTUFPVOSDWidget` is a native UMG/Slate widget created only while a local player possesses the drone. It reads the existing `FFPVTelemetry` snapshot and never reaches into motor/PID internals.

Displayed data:

- armed/disarmed state
- Stabilized or Acro mode
- battery voltage and percentage
- estimated current draw
- video quality
- control-link quality
- estimated video latency
- distance from signal origin
- airspeed magnitude
- altitude AGL
- propwash intensity
- four independent rotor-health values
- active input source
- analog/digital video profile

Warnings include:

- control-link failsafe
- low battery
- damaged prop/motor
- degraded/lost video
- USB-radio arm blocked because throttle is not low

The OSD refreshes at 20 Hz rather than every render frame.

## Video-link presentation

The existing signal model remains the source of truth. Phase 3 turns its quality value into presentation effects.

### Digital

- mild image effects while the link is healthy
- degradation begins primarily in the lower-quality region
- film grain and chromatic aberration increase as quality falls
- small electronic camera jitter increases near link loss
- severe quality loss produces intermittent black-frame/freeze-style blackout flicker through the OSD overlay

### Analog

- degradation scales more continuously with signal loss
- film grain rises much earlier than digital
- chromatic aberration/jitter increase progressively
- no digital-style blackout threshold is applied by the OSD

The current treatment intentionally uses built-in Unreal camera/post-process controls so it does not require a committed third-party material or texture. A custom RF/noise material can replace this later without changing the signal model.

## RawInput USB RC support

The Unreal `RawInput` plugin is enabled in `TheUnit.uproject`, and the runtime module now links `RawInput`, `UMG`, `Slate`, and `SlateCore`.

`UTUFPVRadioInputComponent` reads Epic's Generic USB Controller axes and converts them to the flight solver's native RC semantics:

- throttle: 0..1
- roll: -1..1
- pitch: -1..1
- yaw: -1..1

Default generic axis assignment:

1. roll
2. pitch
3. throttle
4. yaw

Every channel exposes:

- raw minimum
- raw center
- raw maximum
- invert flag

Roll/pitch/yaw also use a configurable center deadband.

These defaults are only a starting point. A physical transmitter's Vendor ID, Product ID, axis ordering, inversion, and ranges must be confirmed in Unreal RawInput settings before flight.

## Radio safety behavior

USB radio mode is **opt-in** and disabled by default.

- Press `B` while controlling the drone to toggle USB RC mode.
- Enabling radio mode while armed disarms the drone.
- If no registered RawInput device/controller data is available, the external channels are forced to safe zero and an armed drone is disarmed.
- Arming with USB RC active is rejected when normalized throttle is above `ArmThrottleSafetyLimit` (default 5%).
- Lowering throttle clears the arm-blocked warning.
- Disabling USB RC restores keyboard/gamepad ownership of the FPV input axes.

## Phase 3 acceptance tests

Run these only after Phase 1 and Phase 2 acceptance checks are stable.

1. **OSD lifecycle:** deploy the drone and confirm the OSD appears only for the possessing local player; exit the drone and confirm it is removed.
2. **Telemetry:** compare OSD battery, speed, altitude, prop health, mode, and signal values against debug/Blueprint telemetry from the pawn.
3. **Warning priority:** force low battery, rotor damage, weak video, and failsafe states and confirm each appears without suppressing the others.
4. **Digital degradation:** reduce digital video quality gradually; the image should remain relatively clean at high quality, become visibly imperfect at low quality, and intermittently black out near link loss.
5. **Analog degradation:** switch to Analog and confirm degradation appears progressively instead of behaving like the digital cliff.
6. **Camera recovery:** restore full video quality and confirm camera relative rotation returns to the configured FPV tilt with no residual jitter.
7. **Radio off by default:** launch with a USB radio connected and verify keyboard/gamepad remain active until `B` is pressed.
8. **No-device safety:** enable USB radio mode with no registered RawInput device and verify the drone cannot receive arbitrary full-stick commands.
9. **Throttle safety:** with a configured radio, hold throttle above 5%, attempt to arm, and confirm the drone immediately disarms/blocks the arm state and displays the warning.
10. **Throttle-low arm:** lower throttle below 5% and confirm normal arming becomes possible.
11. **Axis directions:** verify physical right roll, forward pitch, right yaw, and throttle-up produce matching normalized inputs. Fix RawInput inversion/calibration rather than changing mixer signs.
12. **Full travel:** verify each centered axis reaches approximately -1, 0, +1 and throttle reaches approximately 0 and 1 without clipping early.
13. **Deadband:** release centered sticks and confirm small transmitter jitter does not create visible angular commands.
14. **Fallback:** disable USB RC with `B` and confirm keyboard/gamepad flight resumes immediately.

## Current controls added in Phase 3

- `B`: toggle USB RC RawInput mode while the FPV drone is possessed.

Existing FPV controls remain unchanged.

## Automation coverage

`TheUnit.FPV.RadioSafetyDefaults` verifies that:

- USB RC is opt-in
- default channel ordering is deterministic
- a missing controller never becomes a valid radio source
- missing/disabled radio sources return zero throttle

The existing battery simulation test remains in the same FPV automation suite.

## Next engineering targets

1. Compile/UHT the branch under UE 5.7 and run all `TheUnit.FPV.*` automation tests.
2. Run the Phase 1–3 manual flight acceptance sequence before tuning visual effects.
3. Capture blackbox-style rate/trajectory telemetry for repeatable tuning.
4. Add named Betaflight-style rate profiles and per-device RC calibration persistence.
5. Add level-authored RF attenuation/turbulence volumes.
6. Add server-authoritative ownership/replication and client prediction before multiplayer drone deployment is considered complete.
