# envelope.silent_killer - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/EnvelopeNodes.h:1752`
**Base class:** `voice_manager_base`, `polyphonic_base`
**Classification:** audio_processor

## Signal Path

silent_killer sits in the audio signal path but does NOT modify the audio. In `process()`, it checks if the current voice's audio block is silent via `d.isSilent()`. If silence is detected and the voice has received a note-off, it sends a voice reset message through the PolyHandler. The `processFrame()` method is empty (no per-sample processing). Audio passes through unmodified.

## Gap Answers

### silence-detection-algorithm: Silence detection method

silent_killer uses `ProcessData::isSilent()` which is a SIMD-accelerated per-block check that returns true when all samples are below ~-90dB. The check is per-block, not per-sample. Additionally, the node tracks a per-voice boolean `state` that is set to `true` on note-on and `false` on note-off. The silence check only triggers when: (1) `active` is true, (2) `state.get()` is false (note-off received), AND (3) `d.isSilent()` returns true. This prevents false kills during note-on when audio might be briefly silent.

Note: The `threshold` member is computed from the Threshold parameter via `Decibels::decibelsToGain()` but is NOT actually used in the silence check -- `isSilent()` uses its own hardcoded threshold.

### voice-reset-mechanism: How voices are reset

When silence is detected, `p->sendVoiceResetMessage(false)` is called on the PolyHandler. The `false` argument means it is a non-panic reset (kills only the current voice, not all voices). This is an immediate kill without fade-out -- the voice is already silent so no click occurs.

### active-parameter-behaviour: Active parameter when disabled

When Active is 0 (off), the `active` member is false. The condition in `process()` is `if (active && !s && d.isSilent())` -- when `active` is false, the entire check is skipped. The audio passes through completely unchanged. The node becomes a transparent no-op.

### audio-passthrough: Audio passthrough behavior

silent_killer passes audio through completely unmodified. The `process()` method only reads from the audio data (via `isSilent()`), never writes. There is no fade-out, muting, or any audio modification. The `processFrame()` method is entirely empty.

### description-grammar: Grammar issue in description

Confirmed: the description in the C++ source reads "Send a voice reset message as soon when silence is detected" -- should be "as soon as" or "when".

## Parameters

- **Active:** On/Off toggle (default On). When Off, silence detection is disabled entirely.
- **Threshold:** Range -120 to -60 dB (default -100dB). Converted to linear gain internally but NOT used by the silence detection -- `isSilent()` has its own threshold.

## Conditional Behaviour

The node only acts when all three conditions are met: Active is On, the voice has received note-off (state=false), and the audio block is silent. The per-voice boolean state prevents killing voices that are still playing.

## Polyphonic Behaviour

`PolyData<bool, NumVoices> state` stores a boolean per voice tracking note-on/off. The `handleHiseEvent()` method sets the current voice's state to true on note-on and false on note-off. Note: `polyphonic_base` is constructed with `addProcessEventFlag=false`, so `IsProcessingHiseEvent` is NOT registered. However, `handleHiseEvent()` is still defined and will be called if the node is wrapped in a context that forwards events.

## CPU Assessment

baseline: negligible
polyphonic: true
scalingFactors: []

## Notes

The Threshold parameter is vestigial in its current implementation -- it is stored but never actually used in the silence check. The `isSilent()` method on ProcessData has its own hardcoded -90dB threshold.
