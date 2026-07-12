# Flex AHDSR Envelope - C++ Exploration

**Source:** `hi_core/hi_modules/modulators/mods/FlexAhdsrEnvelope.h`, `hi_core/hi_modules/modulators/mods/FlexAhdsrEnvelope.cpp`
**DSP core:** `hi_dsp_library/dsp_nodes/EnvelopeNodes.h` (template `flex_ahdsr`, struct `flex_ahdsr_base`)
**Display/UI base:** `hi_tools/hi_standalone_components/RingBuffer.h` (`flex_ahdsr_base`, `FlexAhdsrGraph`)
**Base class:** `EnvelopeModulator` (Modulators.h)

## Signal Path

The FlexAhdsrEnvelope is a per-voice envelope modulator that delegates all DSP to a scriptnode-derived template object `flex_ahdsr<NUM_POLYPHONIC_VOICES, ...>`. The signal flow is:

1. **Voice Start** (`startVoice`): Collects modulation values from all 5 internal chains (VoiceStartOnly evaluation). Sets per-voice modulation multipliers for time and level on the state machine. Sends a gate-on via `handleHiseEvent`. Calls `processFrame` once to produce the first sample value.

2. **Block Processing** (`calculateBlock`): Two paths depending on whether SustainLevelChain has time-variant modulators:
   - **Without time-variant sustain mods (common path):** Fills the internal buffer with 1.0f, then calls `obj.process(pd)` which iterates per-sample through the state machine, multiplying each sample by the envelope value.
   - **With time-variant sustain mods:** Calculates sustain modulation per-block, then processes sample-by-sample, updating the sustain/decay level modulation values at each sample before calling `processFrame`.

3. **Per-sample state machine** (`PolyState::calculateNewValue`): The core DSP. Each sample:
   - Computes progress ratio `counter / thisTime`
   - For dynamic stages (Attack, Decay, Release): applies curve exponent to progress, interpolates between `prevLevel` and target level
   - For static stages (Hold, Sustain): outputs the target level directly
   - Increments counter; when counter exceeds thisTime, calls `bump()` to advance to next state

4. **Voice Stop** (`stopVoice`): Sends a gate-off via `handleHiseEvent`, which transitions the state machine to RELEASE if currently before release.

5. **Modulation output**: The envelope value modulates the voice amplitude (standard EnvelopeModulator behavior via `render()` in the base class which calls `calculateBlock` then applies to the voice buffer).

Input (noteOn/noteOff) -> State machine (IDLE -> ATTACK -> HOLD -> DECAY -> SUSTAIN -> RELEASE -> DONE) -> per-sample envelope value -> multiply into voice buffer -> output

## Gap Answers

### signal-path-stage-order: Stage calculation order and curve computation

The state machine processes per-sample at the control rate (downsampled by `HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR`). The `prepareToPlay` method sets `ps.sampleRate = sampleRate / HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR` and `ps.blockSize` likewise. The state machine in `calculateNewValue()`:

1. Computes `value = counter / thisTime` (linear progress 0-1)
2. For non-static stages, reads the smoothed curve value via `get<Curve>()`
3. If curve ~= 1.0: linear interpolation between prevLevel and thisLevel
4. If curve > 1.0: `value = pow(value, curve)` then linear interpolation (concave attack, convex decay/release)
5. If curve < 1.0: inverted power curve `value = 1 - pow(1 - value, 1/curve)` then linear interpolation (convex attack, concave decay/release)

This differs from AHDSR which uses a coefficient-based exponential decay approach. FlexAHDSR uses explicit power-curve shaping with a progress counter, making it more flexible but fundamentally different in character.

### mode-trigger-vs-note-vs-loop: Mode behavior

The Mode enum has three values (stored per-voice as `PolyState::m`):

- **Note (1, default):** Standard AHDSR behavior. The envelope reaches SUSTAIN and holds there indefinitely until note-off. In `calculateNewValue`, when `s == State::SUSTAIN` and `m == Mode::Note`, the sustain stage does NOT call `bump()` - it stays in sustain.

- **Trigger (0):** The envelope plays through all stages without waiting for note-off. When reaching SUSTAIN, since `m != Mode::Note`, `bump()` is called immediately, advancing to RELEASE. The envelope runs as a one-shot: Attack -> Hold -> Decay -> (skip Sustain) -> Release -> Done. Note-off is essentially ignored for stage progression (though it can still trigger release if it arrives during an earlier stage).

- **Loop (2):** Like Trigger, the envelope does not hold at sustain. When reaching DONE state, if `m == Mode::Loop && gateActive`, the state resets to IDLE and `bump()` is called again, restarting from ATTACK. The envelope loops continuously until note-off. On note-off, `gateActive` becomes false, so the next time DONE is reached, the voice stops.

### curve-shape-implementation: Curve parameter mapping

The curve parameters (AttackCurve, DecayCurve, ReleaseCurve) range 0.0-1.0 in the UI. The `convert<Curve>` method transforms them: `pow(2.0, (value - 0.5) * 8.0)`. This maps:

- 0.0 -> pow(2, -4) = 0.0625 (extreme logarithmic/convex shape)
- 0.5 -> pow(2, 0) = 1.0 (linear)
- 1.0 -> pow(2, 4) = 16.0 (extreme exponential/concave shape)

So **0.5 = linear** (the default). The internal curve value is used as the exponent in `pow(value, curve)`. The `sfloat` wrapper provides smoothed transitions when curve values change. This is completely different from AHDSR which has no independent curve per stage - AHDSR uses a single `AttackCurve` coefficient-based approach.

### sustain-normalized-vs-decibel: Sustain and AttackLevel as linear amplitude

Both Sustain and AttackLevel use NormalizedPercentage (0.0-1.0). The `convert<Level>` method simply returns the value unchanged (`return value`). These are linear amplitude values applied directly in the interpolation. There is no dB conversion. This means Sustain at 0.5 is -6 dB (linear 50%), whereas in AHDSR, Sustain uses a Decibel range (-100 to 0 dB) with dB-to-gain conversion.

### monophonic-retrigger-interaction: Three-way interaction

These are handled in the `FlexAhdsrEnvelope` wrapper (not in the scriptnode core):

- **Monophonic** (`isMonophonic`, from EnvelopeModulator base): Forces `voiceIndex = 0` for all operations. All notes share a single voice state.
- **Retrigger** (`shouldRetrigger`, from EnvelopeModulator base): In `startVoice`, `restart = shouldRetrigger || getNumPressedKeys() == 1`. If retrigger is off and multiple keys are held, the envelope continues from its current position.
- **Mode** interacts independently - it controls the state machine's sustain/loop behavior regardless of mono/retrigger settings.

Specific interaction: `Monophonic=On, Retrigger=Off, Mode=Loop` - the envelope starts on the first note, loops continuously. New notes do NOT restart the envelope (retrigger off, numKeys > 1). The loop continues from wherever it is. On last key release, loop ends at next DONE state.

### display-buffer-source-usage: DisplayBufferSource interface

FlexAhdsrEnvelope extends `ProcessorWithSingleStaticExternalData` providing one DisplayBuffer. In the constructor, a `SimpleRingBuffer` is created with size (1, 9) - 1 channel, 9 samples (one per parameter value). The `flex_ahdsr_base::Properties` object is registered as the property handler.

The display is NOT a ring buffer of real-time envelope output. Instead:
- **Parameter values** are written via `refreshUI()` which copies all 10 parameter values into the buffer
- **Playback position** is sent via `rb->sendDisplayIndexMessage(modDisplay)` where `modDisplay = (int)currentState + fractionalProgress`. This encodes both which stage is active and how far through it.
- The `FlexAhdsrGraph` component uses `createPath()` from the Properties object to render the envelope shape from the parameter values (not from audio data). The ball position is computed from the display index message.

The "draggable curves" are handled by the `FlexAhdsrGraph` UI component, which translates mouse drags into parameter changes via `handleUIDrag` -> `handleAdditionalDrag` -> `setAttribute`. The graph itself is purely a parameter visualization and editing surface.

### mod-chain-no-parameter-index: Modulation chain connections

Verified in `startVoice()`:
- `AttackTimeChain` (index 0) -> `setModulationValue<ATTACK, TIME>` (Attack time multiplier)
- `AttackLevelChain` (index 1) -> `setModulationValue<ATTACK, LEVEL>` AND `setModulationValue<HOLD, LEVEL>` (both attack and hold level)
- `DecayTimeChain` (index 2) -> `setModulationValue<DECAY, TIME>` (Decay time multiplier)
- `SustainLevelChain` (index 3) -> `setModulationValue<DECAY, LEVEL>` AND `setModulationValue<SUSTAIN, LEVEL>` (only when chain has no time-variant mods; otherwise handled per-sample in calculateBlock)
- `ReleaseTimeChain` (index 4) -> `setModulationValue<RELEASE, TIME>` (Release time multiplier)

All chains use `ModulationType::VoiceStartOnly` except SustainLevelChain which uses `ModulationType::Normal` (supporting both voice-start and time-variant modulation). The modulation values are multiplied into the per-voice state as `time * timeModValue` and `level * levelModValue`.

Note: SustainLevelChain does NOT have a `VoiceStartModulatorFactoryType::Constrainer` in the metadata (line 116 of FlexAhdsrEnvelope.cpp) - it accepts any modulator type, not just voice-start modulators. This is intentional since it supports time-variant modulation.

### time-range-30000: Extended time range

Time parameters use range 0-30000ms with a `withCentreSkew(2000.0)` skew. The conversion to samples is `value * 0.001 * sr` where `sr` is the control-rate sample rate (audio SR / HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR). There is no special resolution handling for longer times. The counter is a float, so at very long times (30s at 44100/8 = 5512.5 Hz control rate), the counter reaches ~165375 samples. Float precision at this magnitude is sufficient (24-bit mantissa handles values up to ~16M exactly).

There is no EcoMode parameter. The downsampling is always applied via the global `HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR` (typically 8x). This is the same mechanism used by AHDSR since EcoMode became vestigial.

## Processing Chain Detail

1. **Voice initialization** (per note-on): Evaluate 5 modulation chains, set per-voice mod values, gate on, compute first sample. Weight: negligible.

2. **Per-sample state machine** (per block): For each sample in the downsampled block:
   - Read smoothed level and curve values (sfloat advance)
   - Compute power-curve interpolation
   - State transition check
   Weight: low (simple arithmetic per sample, no transcendentals except `pow` for non-linear curves).

3. **Display update** (per block, last started voice only): Encode state + progress as float, send to ring buffer. Weight: negligible.

## Modulation Points

| Chain | Target | Application Point | Type |
|-------|--------|--------------------|------|
| AttackTimeModulation | Attack time | Voice start (multiplier) | VoiceStartOnly |
| AttackLevelModulation | Attack + Hold level | Voice start (multiplier) | VoiceStartOnly |
| DecayTimeModulation | Decay time | Voice start (multiplier) | VoiceStartOnly |
| SustainLevelModulation | Sustain + Decay target level | Voice start or per-sample | Normal (allows time-variant) |
| ReleaseTimeModulation | Release time | Voice start (multiplier) | VoiceStartOnly |

## Conditional Behavior

### Mode parameter
- **Mode::Note (1)**: Sustain stage holds indefinitely. Standard AHDSR. Default.
- **Mode::Trigger (0)**: Sustain is skipped (immediate bump to release). One-shot envelope.
- **Mode::Loop (2)**: Sustain is skipped. At DONE, if gate still active, loops back to ATTACK. Stops when gate off.

### Monophonic + Retrigger
- **Monophonic off**: Each note gets independent voice state.
- **Monophonic on, Retrigger on**: New notes restart the envelope.
- **Monophonic on, Retrigger off**: New notes continue from current envelope position (legato).

### Sustain time-variant modulation
- **No time-variant mods on SustainChain**: Block processing uses `obj.process()` (efficient batch).
- **Time-variant mods on SustainChain**: Sample-by-sample processing with per-sample sustain level update.

### Curve value
- **curve ~= 1.0**: Linear interpolation (fast path, no pow).
- **curve > 1.0**: Exponential curve via `pow(progress, curve)`.
- **curve < 1.0**: Inverted power curve via `1 - pow(1-progress, 1/curve)`.

### AttackLevel vs Sustain level interaction
In `PolyState::set<Level>`: If AttackLevel is set, it is clamped to `max(sustain, attackLevel)`. If Sustain is set, AttackLevel is reclamped to `max(originalAttackLevel, sustain)`. This ensures the attack peak is never below the sustain level.

## Interface Usage

### DisplayBufferSource (ProcessorWithSingleStaticExternalData)
- Provides one DisplayBuffer of size (1, 9)
- Parameter values written via `refreshUI()` for path rendering
- Playback state sent via `sendDisplayIndexMessage()` for ball animation
- `FlexAhdsrGraph` component reads this buffer for visualization
- Properties object handles path creation from parameter values (not real-time audio)

## CPU Assessment

- **Baseline tier: low**
- Per-sample processing at control rate (downsampled 8x from audio rate)
- Main cost: `pow()` call per sample for non-linear curves, smoothed value reads
- Linear curves (default 0.5) use a fast path avoiding `pow()`
- No parameters that scale cost significantly
- SustainLevelChain with time-variant mods adds per-sample modulation overhead (medium in that case)
- No SIMD, no FFT, no oversampling

## UI Components

### FloatingTile: `FlexAHDSRGraph`
- Registered via `Panel` inner class with `SET_PANEL_NAME("FlexAHDSRGraph")`
- Properties: `UseOneDimensionDrag` (default: true), `CurvePointTolerance` (default: 20)
- Creates `flex_ahdsr_base::FlexAhdsrGraph` component connected to the DisplayBuffer

### LAF Callbacks (6 total)
- `drawFlexAhdsrBackground` - background fill
- `drawFlexAhdsrFullPath` - complete envelope path
- `drawFlexAhdsrSegment` - individual stage segment (with hover/active states)
- `drawFlexAhdsrCurvePoint` - curve drag point (with hover/down states)
- `drawFlexAhdsrDragPoint` - stage drag point
- `drawFlexAhdsrPosition` - playback ball position
- `drawFlexAhdsrText` - parameter label text

### Editor (backend only)
- `FlexAhdsrEnvelopeEditor` creates a `FlexAhdsrGraph` display at the top
- Below: two rows of `HiSlider` knobs for all 10 parameters
- Uses `GlobalHiseLookAndFeel`

## Notes

- The `PolyState` has per-voice `Values` structs with smoothed `sfloat` for level and curve. The smoothing time is 20ms (set in `prepare`). This means parameter changes to level and curve are smoothed over 20ms to avoid clicks.
- The `bump()` method handles zero-time stages by skipping them: if a stage has time=0, it reads the level, sets prevLevel, and advances to the next stage in a while loop. This correctly handles Hold=0 (skip hold entirely).
- Hold stage has no curve parameter (no case for Hold in `getAttributeIndex` for Curve type). Hold is a static stage that outputs the target level directly. The forum insight about "no drag handle for Hold" is explained by Hold only having Time and Level (shared with Attack level), no curve.
- The `obj.active` property in LAF callbacks reflects whether the segment is being played by the last started voice (`sendBallUpdate` is set only for `polyManager.getLastStartedVoice() == voiceIndex`).
- Default Mode in metadata is 1.0 (Note mode), confirmed in `createMetadata()` line 77.
- AttackLevel modulation also sets Hold level modulation (line 256 of .cpp), which is correct since Hold maintains the attack peak level.
