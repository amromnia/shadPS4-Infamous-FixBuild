# Synthetic motion correction

The fork already implements mouse motion and G/H/T gestures, but physical or
virtual DS4 sensor events cleared `m_motion_emulated` on every update. A held key
only activates its output on a state change. Consequently, a subsequent sensor
event could cancel the gesture until another key transition. Mouse motion could
likewise be repeatedly reset by the next sensor update.

The correction keeps physical sensor buffers separate from the synthetic report.
Each controller poll reads the physical sample first, then applies active synthetic
motion. Physical updates no longer disable it or erase held gestures. After keys
are released, the roll returns to neutral before control returns to physical motion.
Turning mouse gyro off explicitly relinquishes its override.

The mouse path also treated displacement as angular velocity without dividing by
the polling interval. At the 33 ms mouse timer, 100 horizontal pixels produced only
about 0.033 radians rather than the intended one radian of rotation. The corrected
conversion uses 0.01 radians per pixel divided by the timer interval.

## Existing patches reviewed

- [Upstream PR 5248](https://github.com/shadps4-emu/shadPS4/pull/5248) applies
  synthetic gestures after physical samples and keeps a shake active for at least
  one second. This fork had an alternative implementation lacking those properties.
  The correction adopts its minimum-duration, 9 Hz, 2.5 g shake with wrist wobble.
- [TFSG28 commit 90d08a9](https://github.com/TFSG28/shadPS4-Infamous-FixBuild/commit/90d08a97fd3be724c472228a196fb30ef0308064)
  adds controller chords while retaining keyboard controls. Backported with attribution.
- [Issue 4546](https://github.com/shadps4-emu/shadPS4/issues/4546) requests DSU
  sensor support. It is not an available shake fix and is not required for synthetic motion.

## Controls and diagnostics

- Hold G/H to tilt left/right. Hold the tilt through the sideways-controller prompt.
- Press or hold T to shake. A short press now runs for at least one second.
- Controller alternatives: View/Back + D-pad Left/Right for tilt; View/Back + D-pad Up for shake.
- F6 toggles mouse gyro. Hold Right Shift while moving the mouse horizontally to roll
  the controller sideways; ordinary mouse movement controls yaw/pitch rather than roll.
- These defaults are appended to global.ini if absent; explicitly unmapped controller
  outputs are respected by the backported binding migration.

The log reports `Emulated tilt`, `Emulated shake`, and `Mouse gyro enabled` when
the corresponding actions reach the input implementation. If they are absent,
investigate focus, bindings, UI input capture, launcher user-data directory, and
controller slot assignment before changing the motion waveform again. If they are
present but the game does not react, capture a fresh log and reproduce with the
same save and controller configuration. No game-specific bypass is added.

Validation: source/control-flow review and whitespace checks. Existing Windows
build/tests provide compilation coverage, not in-game proof of gesture recognition.
The supplied startup log did not cover a working motion prompt; the identified
defects do not prove that every reported failure has the same cause.
