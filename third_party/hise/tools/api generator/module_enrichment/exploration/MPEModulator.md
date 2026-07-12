# MPE Modulator - C++ Exploration

**Source:** `hi_core/hi_modules/modulators/mods/MPEModulators.h`, `hi_core/hi_modules/modulators/mods/MPEModulators.cpp`
**Base class:** `EnvelopeModulator` (also implements `LookupTableProcessor`, `MPEData::Listener`)

## Signal Path

MIDI event -> gesture type filter -> normalize to 0-1 -> table lookup -> smoother -> internalBuffer -> applyTimeModulation (intensity scaling)

The signal path starts in `handleHiseEvent()` which filters incoming MIDI events by gesture type. On note-on, the MIDI channel is saved for per-voice routing and the velocity is captured (used as Stroke value or Press initial scaling). For continuous gestures (Press, Slide, Glide), the corresponding MIDI message type is matched, normalized to 0-1, run through the table lookup, and set as the target value on matching voice states. The per-voice `MPEState::process()` method smooths toward the target value sample-by-sample. The base class `render()` calls `calculateBlock()` which writes smoothed values into `internalBuffer`, then calls `applyTimeModulation()` which applies the modulator intensity to the voice audio buffer.

## Gap Answers

### gesture-cc-mapping: How do the 5 gesture types map to MIDI messages?

In `handleHiseEvent()`:
- **Press** (1): Channel aftertouch / channel pressure (`m.isChannelPressure()`). Value from `m.getNoteNumber() / 127.0` (HISE encodes pressure in the note number field of the HiseEvent).
- **Slide** (2): CC #74 (`m.isControllerOfType(74)`). Value from `m.getControllerValue() / 127.0`. This is the MPE standard "timbre" dimension. The table X-axis converter switches to pitch-bend range display for this gesture.
- **Glide** (3): Pitch wheel (`m.isPitchWheel()`). Value from `(pitchWheelValue - 8192) / 2048 * 0.5 + 0.5`, mapping the full pitch bend range to 0-1 with center at 0.5. This is the MPE "pitch bend per note" dimension.
- **Stroke** (4): Note-on velocity. Captured during note-on only; the table is applied immediately at note-on and the result stored as `unsavedStrokeValue`. No continuous updates during the note.
- **Lift** (5): Note-off velocity (`m.isNoteOff()`). Value from `m.getVelocity() / 127.0`. Only fires once at note-off.

The 5 gestures map to the 3 standard MPE dimensions (pressure, slide/CC74, glide/pitch bend) plus 2 non-continuous event-based values (note-on velocity, note-off velocity). Stroke and Lift are not MPE-specific extensions but rather standard MIDI velocity data repurposed as modulation sources.

### signal-path-order: What is the processing order?

1. **MIDI event received** in `handleHiseEvent()`
2. **Gesture type filter**: only the matching MIDI message type passes through
3. **Normalize to 0-1**: raw MIDI value normalized to float range
4. **Monophonic max-value selection** (monophonic mode only): `mpeValues.storeAndGetMaxValue()` stores per-channel values and returns the maximum across all channels (for Press/Slide) or the channel with greatest distance from center (for Glide)
5. **Table lookup**: `table->getInterpolatedValue(midiValue)` maps the normalized value through the user curve
6. **Set as target**: the table output becomes the target value on matching voice states, scaled by `smoother.getA0()` (the smoother coefficient)
7. **Per-sample smoothing**: `MPEState::process()` applies one-pole smoothing toward the target, writing into `internalBuffer`
8. **Intensity scaling**: `applyTimeModulation()` (base class) applies the modulator intensity to the voice buffer

For **Stroke**, the table lookup happens at note-on and the result is used as the initial voice target value. For all other gestures, the table lookup happens on each incoming MIDI message.

### smoothed-intensity-role: What does SmoothedIntensity control?

SmoothedIntensity is stored into the modulator's `intensity` via `setIntensity(smoothedIntensity)` in `setInternalAttribute()`. It functions as the overall modulation depth/amplitude scaling applied by the base class `applyTimeModulation()`. It is NOT a mix between smoothed and unsmoothed signals. It is the equivalent of the standard modulator Intensity parameter, but with mode-dependent scaling:
- GainMode: stored directly (0-1)
- PitchMode: divided by 12 (so the UI shows semitones, internal is normalized)
- PanMode: divided by 100 (so the UI shows percentage, internal is normalized)

Setting SmoothedIntensity to 0 makes the modulation output zero (no modulation applied). The smoothing always runs regardless of this parameter.

### per-voice-mpe-routing: How does per-voice MPE routing work?

MPE channel-to-voice mapping works through the `midiChannel` field on each `MPEState`. In `startVoice()`, the channel from the most recent note-on (`unsavedChannel`) is stored in the voice's state: `s->midiChannel = unsavedChannel`. The `unsavedChannel` is captured in `handleHiseEvent()` when a note-on is received: `unsavedChannel = m.getChannel()`.

When continuous gesture messages arrive (Press, Slide, Glide), `handleHiseEvent()` iterates over `activeStates` and checks `s->midiChannel == c` (where `c` is the incoming message channel). Only the voice whose MIDI channel matches the incoming message channel receives the gesture update. This implements the MPE one-channel-per-voice model.

In monophonic mode, the channel check is skipped (`isMonophonic` makes `midiChannelMatches` always true), and all gesture data feeds into the single `monoState`.

### envelope-lifecycle: How does the note-on/note-off lifecycle work?

- **startVoice()**: Sets `isPressed = true`, initializes the smoother with `defaultValue` as the starting point. For Stroke, the target is set to the captured velocity-through-table value. For other gestures, the target starts at `defaultValue`.
- **During note**: Continuous gesture messages update the target value via `setTargetValue()`. The smoother interpolates toward the target each sample in `process()`.
- **stopVoice()**: Sets `isPressed = false`. If the current target is 0.0, sets `isRingingOff = true`. With `isPressed = false`, the voice no longer receives gesture updates (the `s->isPressed && midiChannelMatches` check in `handleHiseEvent()` prevents it).
- **isPlaying()**: Returns false only when `isRingingOff && currentRampValue == 0` (meaning the target was zero and the smoother has finished ramping down). In monophonic mode or when intensity < 1.0, always returns true.
- **reset()**: Clears the voice state entirely, removes from `activeStates`.

The modulation value does NOT automatically ramp to zero on note-off. It holds its last value unless a Lift gesture sets a new target, or unless the gesture naturally decays to zero. The `isRingingOff` mechanism only triggers when the target was already at zero at the moment of note-off.

### monophonic-mpe-interaction: How does Monophonic mode interact with MPE?

In monophonic mode, all voices share a single `monoState` instance. The channel matching is bypassed (all channels feed the monoState). The `MPEValues` struct stores per-channel values for each gesture type in 16-element arrays. `storeAndGetMaxValue()` returns:
- For Press: the maximum pressure across all 16 channels
- For Slide: the maximum CC74 across all 16 channels
- For Glide: the value from the channel with the greatest absolute distance from center (0.5)
- Stroke and Lift: not handled in `storeAndGetMaxValue()` (returns 1.0)

A `monophonicVoiceCounter` tracks active voices. On the first voice, `monoState` is fully initialized. On subsequent voices with retrigger enabled, `monoState.startVoice()` is called to reset the smoother. When all voices are released and counter reaches 0, `monoState.reset()` and `mpeValues.reset()` clear the state.

### table-usage: Is there a UseTable toggle?

There is no UseTable parameter. The table is always active. In `handleHiseEvent()`, every incoming gesture value (except during note-on for non-Stroke gestures) passes through `table->getInterpolatedValue(midiValue)`. For Stroke, the table is applied at note-on. The table is initialized via `referenceShared()` in the constructor and loaded/saved via `loadTable`/`saveTable` in serialization. A fresh table defaults to a linear identity curve (diagonal line), which passes values through unchanged.

## Processing Chain Detail

1. **MIDI event filter** (per-voice, negligible): Matches incoming HiseEvent type against selected gesture. Normalizes to 0-1 float.
2. **Monophonic aggregation** (shared, negligible): In mono mode only, stores per-channel values and selects maximum. Per-voice in poly mode (no aggregation).
3. **Table lookup** (shared resource, negligible): 512-point interpolated lookup table. Applied to normalized gesture value. Always active.
4. **Target value update** (per-voice, negligible): Sets the smoothing target on matching voice state(s), scaled by smoother coefficient a0.
5. **Per-sample smoothing** (per-voice, low): One-pole smoother runs per-sample in `MPEState::process()`. Controlled by SmoothingTime parameter. Runs at control rate (downsampled).
6. **Intensity scaling** (per-voice, negligible): Base class `applyTimeModulation()` multiplies the internal buffer values by intensity (SmoothedIntensity).

## Modulation Points

No modulation chains. The module has zero internal chains (`getNumInternalChains() returns 0`). SmoothedIntensity acts as a static intensity multiplier, not a modulatable parameter.

## Conditional Behavior

- **GestureCC** (1-5): Completely changes which MIDI message type is listened to. Stroke processes only at note-on; Lift only at note-off; Press/Slide/Glide are continuous. When changed, resets all voice states and recalculates the default value. Slide mode switches the table X-axis display to pitch-bend range.
- **Monophonic** (on/off): Switches between per-voice `MPEState` instances (poly) and shared `monoState` (mono). In mono mode, channel matching is disabled and max-value aggregation across channels is used. Clears all states on toggle.
- **Retrigger** (on/off): Only relevant in monophonic mode. When enabled and a new note arrives while one is already playing, resets the smoother to the start value. When disabled, the envelope continues from its current position.
- **MPE global enable** (external): `mpeModeChanged()` and `mpeModulatorAssigned()` control the bypass state. The modulator is bypassed when EITHER the global MPE mode is disabled OR the modulator is not registered in the MPEData connections list.

## Interface Usage

### LookupTableProcessor (TableProcessor)

The table is always active (no UseTable toggle). It is used in two places:
1. In `handleHiseEvent()` for continuous gestures: `table->getInterpolatedValue(midiValue)` maps the normalized 0-1 gesture value through the curve before setting it as the smoothing target.
2. In `handleHiseEvent()` for Stroke at note-on: `table->getInterpolatedValue(midiValue)` maps velocity through the curve and stores the result as `unsavedStrokeValue`.

The table X-axis display converter changes based on gesture type: Slide uses `getDomainAsPitchBendRange`, all others use `getDomainAsMidiRange`.

### MPEData::Listener

The modulator registers as a listener on the global `MPEData` object. Three callbacks:
- `mpeModeChanged(bool)`: Updates `isActive` and sets bypass state based on `isActive && isConnected`.
- `mpeModulatorAssigned(MPEModulator*, bool)`: When this modulator is the one being assigned/unassigned, updates bypass state.
- `mpeDataReloaded()`: Empty implementation.

On listener registration (`addListener`), `mpeModeChanged` is immediately fired with the current state, which sets the initial bypass. Since MPE defaults to disabled, the modulator starts bypassed.

## CPU Assessment

- **Overall baseline: low**
- Per-sample cost is minimal: one-pole smoothing per voice per sample
- Table lookup is per-MIDI-event, not per-sample
- No expensive operations (no FFT, no convolution, no oversampling)
- Cost scales linearly with active voice count (one smoother per voice)
- Runs at control rate, not audio rate

## UI Components

The editor (`MPEModulatorEditor`) uses a custom panel with:
- `TableEditor` for the lookup table
- `HiComboBox` for gesture type selection
- `HiSlider` for smoothing time and default value
- `MPEKeyboard` widget embedded in the editor

No FloatingTile content types discovered in `createEditor()`.

## Notes

- The Glide gesture normalization uses a range of 2048 (not 8192) for the divisor: `(pitchWheelValue - 8192) / 2048 * 0.5 + 0.5`. This maps the standard 14-bit pitch bend range (0-16383, center 8192) asymmetrically. Values below center map to 0.0-0.5, values above center map to 0.5-~2.5 before clamping. The `jlimit(0, 1, midiValue)` clamp means effective upward pitch bend range is only about 1/4 of the full range before hitting the ceiling. This may be intentional to provide finer resolution in the usable range, or it could be a calculation error (8192 would give symmetric 0-1 mapping).
- The `SmoothedIntensity` parameter name is misleading. It does not control smoothing intensity; it IS the modulator intensity (gain/depth). The name suggests a relationship to the smoothing process that does not exist. It calls `setIntensity()` directly.
- `Stroke` and `Lift` are event-based (fire once), while `Press`, `Slide`, and `Glide` are continuous. This fundamental behavioral difference is not visible from the parameter names alone.
- The MPE global enable mechanism means the modulator is effectively a no-op until both conditions are met: (1) global MPE mode is enabled, and (2) the modulator is registered in the MPEData connections list. Registration happens through the MPE panel UI.
- `Engine.isMpeEnabled()` is the only scripting API related to MPE state. There is no API to read individual MPE modulator values or to programmatically enable/disable individual MPE modulators.
- DefaultValue has mode-dependent scaling in `setInternalAttribute`: PitchMode divides by 24 and offsets by 0.5; PanMode divides by 200 and offsets by 0.5. The UI shows semitones/percentage but internal storage is normalized 0-1.
