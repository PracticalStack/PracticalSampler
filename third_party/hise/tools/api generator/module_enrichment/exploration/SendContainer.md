# SendContainer - C++ Exploration (Step 3)

## Source Files

- `hi_core/hi_modules/effects/fx/RouteFX.h` (505 lines) - SendContainer lines 104-234, SendEffect lines 236-500
- `hi_core/hi_modules/effects/fx/RouteFX.cpp` (158 lines) - SendEffect::createMetadata

## Gap Answers

### signal-flow

**Question:** How does SendContainer receive audio from SendEffect instances?

**Answer:** The signal flow is a two-phase accumulate-then-process model:

1. **Accumulation phase (during SendEffect rendering):** Each SendEffect calls `container->addSendSignal(b, startSample, numSamples, startGain, endGain, channelOffset)` in its `applyEffect()` method (RouteFX.h:414-444). This uses `addFrom` / `addFromWithRamp` to additively mix the send signal into the container's `internalBuffer` at the specified channel offset and gain.

2. **Processing phase (during SendContainer rendering):** `renderNextBlockWithModulators()` (RouteFX.h:195-229) runs the effect chain on the accumulated `internalBuffer`, then uses the routing matrix to copy each internal channel to the appropriate output channel via `addFrom`. Finally, `internalBuffer` is cleared for the next cycle.

The SendContainer does NOT use the standard ModulatorSynth voice rendering at all - `renderNextBlockWithModulators()` is completely overridden with no call to the base class version. It processes MIDI events (for the modulation chains) but does not render voices.

### fx-chain-type

**Question:** What effects can go in the FX chain?

**Answer:** The FX chain is constrained by `NoMidiInputConstrainer` (RouteFX.h:122-124). The metadata also uses `.withFXConstrainer<NoMidiInputConstrainer>()` (line 113). Additionally, `effectChain->setForceMonophonicProcessingOfPolyphonicEffects(true)` is called in the constructor (line 125), meaning any polyphonic effects placed in the chain are forced to process monophonically.

The constrainer type from moduleList.json is "MasterEffect", meaning only MasterEffect-subtype effects can be added.

### multichannel-routing

**Question:** How does channel offset interact with routing matrix?

**Answer:** The SendEffect's `channelOffset` parameter determines which pair of channels in the container's `internalBuffer` receives the send signal. In `addSendSignal()` (RouteFX.h:164-184), the offset is clamped to `[0, internalBuffer.getNumChannels() - 2]` and the stereo signal is written to channels `[channelOffset, channelOffset+1]`.

Then in `renderNextBlockWithModulators()`, after the FX chain processes the internal buffer, the routing matrix maps each internal channel to an output channel: `getMatrix().getConnectionForSourceChannel(i)` determines the destination for each source channel (RouteFX.h:216-222).

The routing matrix is resizable (`getMatrix().setAllowResizing(true)` at line 120), so the container can have more than 2 internal channels for multichannel routing setups.

### voice-context-actual

**Question:** Is SendContainer effectively monophonic?

**Answer:** Yes. The constructor calls `ModulatorSynth(mc, id, 1)` - it is initialised with exactly 1 voice (RouteFX.h:117). The `renderNextBlockWithModulators()` override completely bypasses the standard voice rendering pipeline. There are no voice objects - the module operates on a single monophonic internal buffer. The VoiceLimit and KillFadeTime parameters are inherited but functionally irrelevant.

### gain-mod-relevance

**Question:** Is the Gain Modulation chain actually applied?

**Answer:** The `renderNextBlockWithModulators()` override does NOT call the base class version, and it does not explicitly apply the Gain or Gain Modulation chain to the internal buffer. It only calls `processHiseEventBuffer()` (for event routing), then runs the effect chain, then routes via matrix. The Gain and Balance parameters are inherited from ModulatorSynth but are NOT applied in the container's render path. The Gain Modulation chain is similarly not applied.

However, the Gain parameter may still affect the peak display or other base-class bookkeeping. For practical purposes, the container's output level is controlled entirely by the SendEffect gain parameters and the effects in the FX chain.

### multiple-senders

**Question:** Can multiple SendEffects route to the same SendContainer?

**Answer:** Yes. `addSendSignal()` uses `addFrom` and `addFromWithRamp` (RouteFX.h:172-183), which are additive operations. Multiple SendEffect instances can target the same container (via matching SendIndex), and their signals will be summed into the internal buffer. This makes SendContainer function as a summing bus.

## Additional Findings

- SendContainer uses an `EmptyProcessorEditorBody` (line 158) - it has no custom UI panel, just the module header and FX chain.
- The SendEffect connects to a SendContainer by index - it scans all SendContainer instances in the main synth chain and connects by position (RouteFX.h:465-486).
- SendEffect gain is in decibels (-100 to 0 dB) with optional smoothing (80ms ramp at block rate). The gain is also modulated by a "Send Modulation" chain.
- SendEffect supports soft bypass with ramped gain transitions (wasBypassed/shouldBeBypassed logic at lines 435-441).
- The internal buffer is cleared at the end of each render cycle (line 228), so it always starts fresh.

## Issues

The base-class Gain, Balance, VoiceLimit, KillFadeTime parameters are exposed in the moduleList.json but Gain and Balance are not functionally applied by the container's render path. VoiceLimit and KillFadeTime are irrelevant (1 voice, no voice model). The Pitch Modulation chain is also inherited but meaningless. These should be noted as vestigial in the reference page rather than documented as functional parameters.
