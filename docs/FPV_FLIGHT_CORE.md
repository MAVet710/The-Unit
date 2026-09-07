# FPV Flight Core — Phases 1–2

## Goal

Build an FPV drone that behaves as a physics-driven quadcopter inside The Unit rather than a flying character controller. The implementation deliberately models the physical causes behind convincing FPV motion instead of cloning another game's proprietary behavior.

Phase 1 establishes rigid-body flight, four-rotor thrust, rate control, camera geometry, and the operator possession loop. Phase 2 adds lightweight rotor-wake/propwash behavior, LiPo discharge and voltage sag, per-rotor crash damage, RF/video-link behavior, failsafe handling, OSD-ready telemetry, and a clean input seam for future USB RC radios.

## Phase 1 baseline

- 0.70 kg five-inch-style airframe baseline.
- Four independent rotor force locations on an X quad.
- 7.5 N maximum thrust per motor (30 N total).
- Approximate static thrust-to-weight ratio: 4.37:1 at a fresh battery and healthy props.
- Motor command-to-thrust relationship: normalized command squared.
- 35 ms first-order motor spool response.
- Per-axis PID angular-rate controller.
- Acro/manual mode: sticks command angular rate.
- Stabilized mode: roll/pitch sticks command angle, converted to rate targets; yaw remains rate controlled.
- Default max rates: 900 deg/s roll, 900 deg/s pitch, 700 deg/s yaw.
- Rate expo.
- Rotor reaction torque for yaw.
- Directional quadratic aerodynamic drag based on local projected area.
- Lightweight ground-effect model.
- 25 degree FPV camera uptilt and 110 degree FOV.
- Physics substepping at up to 120 Hz effective step size (`0.008333 s`) with up to four substeps.
- Operator deploy/possess/return loop.

## Phase 2 systems

### LiPo battery and voltage sag

`UTUFPVBatteryComponent` owns battery state independently from flight control.

Default baseline:

- 6S pack.
- 1300 mAh capacity.
- 4.20 V/cell full and 3.30 V/cell empty.
- 0.028 ohm effective whole-pack resistance.
- Estimated electrical load from electronics plus normalized motor load.
- Loaded voltage drops instantly with current draw (sag) and gradually with state of charge.
- Available rotor thrust scales with loaded pack voltage.
- Low-voltage state is exposed to telemetry.

This is intentionally a lightweight electrical model. It is designed to create the correct gameplay causes — fresh-pack punch, throttle sag, declining headroom, and low-voltage warning — without simulating an ESC/motor circuit at microsecond resolution.

### Rotor wake / propwash

The propwash approximation becomes active when the aircraft descends rapidly along its own thrust axis and remains within its rotor wake.

- Starts around 2 m/s descent.
- Reaches full configured intensity around 7 m/s descent.
- Lateral velocity allows the quad to escape its own turbulent wake.
- Maximum default thrust loss is 22%.
- High-frequency roll/pitch disturbance torque is applied during the event.
- Current propwash intensity is exposed as telemetry from 0–1.

This is a tunable gameplay approximation, not CFD. It should be calibrated against actual dive/recovery behavior after the basic mixer and PID signs are proven.

### Prop/motor crash damage

Physics hits now use Chaos `NormalImpulse` to estimate impact delta-v. Impacts above the configured threshold damage the rotor nearest the impact point.

- Damage begins around an estimated 4 m/s collision delta-v.
- Full severity is reached around 12 m/s.
- Each severe hit can remove up to 70% health from the nearest rotor.
- Rotor health scales both thrust and reaction torque.
- Damaged rotors produce command-dependent vibration torque.
- `ServiceDrone()` repairs all four rotors and recharges the battery for bench/testing workflows.

Motor order remains:

1. front-left
2. front-right
3. rear-right
4. rear-left

### RF/control and FPV video link

`UTUFPVSignalComponent` owns link simulation independently from flight physics.

Default baseline:

- Digital video profile by default.
- 250 m clear video reference range.
- 500 m clear control reference range.
- Distance quality uses a smooth nonlinear falloff.
- Visibility obstruction reduces video quality more aggressively than control quality.
- Digital video latency rises as link quality falls.
- Analog profile keeps latency low while quality degrades more directly.
- Link checks run at 10 Hz rather than every render frame.
- If control quality falls under the failsafe threshold, motor commands are cut; after the configured delay the drone disarms.

The current link model is intentionally generic. Level-specific RF attenuation volumes, antenna orientation, concrete/metal material loss, relay systems, and multipath are later refinements.

### OSD-ready telemetry

`GetTelemetry()` exposes one Blueprint-friendly snapshot containing:

- battery voltage
- battery percentage
- current draw
- low-battery warning
- video quality
- control quality
- estimated video latency
- link distance
- speed
- altitude AGL
- propwash intensity
- all four rotor health values
- armed state
- flight mode
- signal-failsafe state

The data layer is implemented first so the eventual visual OSD can be swapped between analog, digital, bodycam/tactical, training, and debug presentations without coupling UI code to the physics solver.

### External RC input seam

`SetExternalRCEnabled()` and `SetExternalRCInput()` provide native RC semantics:

- throttle: 0–1, non-spring-centered
- roll: -1–1
- pitch: -1–1
- yaw: -1–1

When external RC input is enabled, the temporary keyboard/gamepad axis callbacks stop overwriting those values. A RawInput/HID adapter can therefore be added without changing the flight controller or mixer.

## Current controls

### Operator

- `G`: deploy the FPV drone or re-enter the existing drone.

### Drone keyboard

- `W`: full throttle.
- `S`: zero throttle.
- neither `W` nor `S`: centered/mid throttle for prototype keyboard testing.
- `A` / `D`: yaw left/right.
- Arrow Left / Right: roll.
- Arrow Up / Down: pitch forward/back.
- `Space`: arm/disarm.
- `M`: toggle Stabilized / Acro.
- `R`: reset the drone to its spawn transform and disarm. Reset does not repair/recharge.
- `Tab`: disarm and return control to the operator.

### Gamepad

- Left stick Y: throttle.
- Left stick X: yaw.
- Right stick X: roll.
- Right stick Y: pitch.

A spring-centered gamepad throttle remains an accessibility/testing mapping. Native USB RC-radio input should use the external-RC seam.

## Acceptance tests — Phase 1

Run these before tuning Phase 2 effects.

1. **Spawn/return:** Press `G`; the drone should spawn roughly 0.8 m ahead and 0.35 m above the operator. `Tab` should return to the operator without destroying the drone. Pressing `G` again should reuse the same drone.
2. **Disarmed physics:** The drone behaves as an ordinary rigid body with gravity and collision while disarmed.
3. **Arm/hover:** Centered prototype throttle is in the usable hover region rather than causing extreme vertical acceleration.
4. **Roll direction:** Right-roll command raises the correct side of the quad so the aircraft actually rolls right.
5. **Pitch direction:** Forward pitch raises rear thrust relative to front thrust so the nose pitches down/forward.
6. **Yaw direction:** Yaw comes from opposing rotor reaction torque/mixer changes rather than direct actor rotation.
7. **Stabilized recovery:** Releasing roll/pitch returns toward level.
8. **Acro persistence:** Releasing roll/pitch does not auto-level.
9. **Motor lag:** Rapid throttle changes have visible spool response rather than instantaneous thrust.
10. **Ground effect:** Thrust increases slightly near a floor and fades by roughly 0.55 m.
11. **Directional drag:** Fast motion bleeds energy rather than behaving like zero-drag spacecraft motion.
12. **Collision:** Rotor physics never bypasses Chaos collision through walls, floors, door frames, or kill-house obstacles.

## Acceptance tests — Phase 2

Do not tune all of these simultaneously.

1. **Fresh-pack voltage:** A serviced 6S pack reports approximately 25.2 V unloaded.
2. **Throttle sag:** High throttle immediately lowers loaded voltage and available thrust slightly; releasing throttle recovers some voltage.
3. **Discharge:** Sustained motor load reduces battery percentage over time.
4. **Low battery:** Low pack voltage reduces maximum thrust headroom and exposes the low-battery telemetry flag.
5. **Clean descent:** Descending slowly or moving laterally should produce little/no propwash intensity.
6. **Wake entry:** A rapid near-vertical descent should increase propwash intensity, reduce effective thrust, and introduce small rotational disturbances.
7. **Wake escape:** Adding substantial lateral speed should reduce the propwash effect.
8. **Minor collision:** Low-speed wall/floor contact should not damage a rotor.
9. **Hard arm/prop strike:** A hard collision near one arm should damage the nearest rotor, lower its available thrust, and create vibration under power.
10. **Persistent damage:** `R` resets position but does not magically repair the damaged prop.
11. **Service:** Calling `ServiceDrone()` restores rotor health and battery state.
12. **Clear link:** Near the operator with line of sight, video/control quality should remain near 1.0.
13. **Obstruction:** Moving behind solid kill-house geometry should reduce video quality more noticeably than control quality.
14. **Range:** Increasing distance should smoothly lower both link-quality values.
15. **Failsafe:** Driving control quality below threshold should cut motor commands and disarm after the configured failsafe delay.
16. **External input:** When external RC mode is enabled, keyboard/gamepad axis events should not overwrite externally supplied channels.

## Tuning order

1. Verify thrust units and hover point.
2. Verify motor/mixer signs.
3. Tune P only for rate tracking.
4. Add D until overshoot/oscillation is acceptably damped.
5. Add minimal I for sustained error/disturbance rejection.
6. Tune Stabilized level-rate gain.
7. Tune rate expo and maximum rates.
8. Tune drag against speed/coast observations.
9. Tune ground effect.
10. Validate current draw and battery sag against a representative five-inch FPV setup.
11. Tune propwash only after normal rate control is stable.
12. Tune crash thresholds using repeatable wall/floor impact speeds.
13. Tune signal distances/obstruction by level and intended tactical role.

## Next layer

- Visible OSD widget driven only by `FFPVTelemetry`.
- Low-battery and link-quality audio/visual warnings.
- Actual analog/digital video breakup treatment and optional render latency.
- USB RC transmitter adapter via RawInput/Enhanced Input, with calibration, channel mapping, deadband, expo, and arming safety.
- Betaflight-style named rate profiles and editable presets.
- Wind and indoor turbulence volumes.
- Blackbox-style telemetry recording for rate/trajectory comparison against reference footage or real logs.
- Server-authoritative multiplayer drone ownership and replication/prediction.

## Validation philosophy

The target is not to imitate another game's proprietary implementation. The target is to reproduce the physical causes that make good FPV simulators convincing: independent rotor thrust, inertial rigid-body motion, rate control, motor response, aerodynamic loss, camera geometry, power-system limitations, disturbed airflow, damage asymmetry, and communication constraints. The flight model should be judged against repeatable telemetry and flight tests rather than subjective camera feel alone.
