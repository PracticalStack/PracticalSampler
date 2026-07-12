# envelope.voice_manager - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/EnvelopeNodes.h:2011`
**Base class:** `voice_manager_base`
**Classification:** utility

## Signal Path

voice_manager is outside the signal path. It has no audio processing (SN_EMPTY_PROCESS, SN_EMPTY_PROCESS_FRAME). Its sole function is to receive a parameter value and use it to kill voices. When the "Kill Voice" parameter drops below 0.5, it sends a voice reset message via the PolyHandler.

## Gap Answers

### kill-voice-mechanism: How voices are killed

When `setParameter<0>(v)` is called with `v < 0.5` and `voiceIndex != -1` (a voice is currently being rendered), `p->sendVoiceResetMessage(false)` is called. The `false` parameter means a single-voice reset (not all voices). There is also a second parameter slot (P == 1) where `v < 0.5` triggers `p->sendVoiceResetMessage(true)` which is a panic/all-voices reset. However, only parameter 0 ("Kill Voice") is registered in `createParameters()`.

### value-threshold-logic: Threshold at 0.5

The threshold is exactly `v < 0.5` (strict less-than). So 0.5 itself does NOT trigger a kill; only values below 0.5 do. The default is 1.0 (no kill). Since the parameter has step=1.0, the UI offers only 0 and 1, where 0 kills and 1 does not kill. But via modulation, any value < 0.5 triggers.

Additionally, there is a voice index check: the kill only fires when `p->getVoiceIndex() != -1`, meaning a specific voice must be actively rendering. This means the kill operates on the currently-rendered voice only.

### outside-signal-path-role: Role of voice_manager

voice_manager is a pure modulation target. It registers `OutsideSignalPath` in its constructor. All audio and frame processing methods are empty. Its role is to receive a modulation value (typically from an envelope node's Gate output or any control signal) and use it to kill the currently-rendering voice when the value drops. It acts as a bridge between the modulation system and voice lifecycle management.

### monophonic-voice-kill: How a monophonic node kills polyphonic voices

voice_manager is NOT polyphonic (`isPolyphonic() = false`) but inherits from `voice_manager_base` which stores the `PolyHandler* p` pointer from `prepare()`. The PolyHandler is shared across all voices. When `setParameter<0>()` is called, `p->getVoiceIndex()` returns the voice currently being rendered on the audio thread (set by the outer polyphonic wrapper). So despite being monophonic, the parameter callback runs within the voice rendering context, and the PolyHandler knows which voice is active. The kill message targets that specific voice.

## Parameters

- **Kill Voice:** Range 0-1, step 1, default 1.0. When value < 0.5, sends a voice reset for the currently-rendering voice. The parameter has an unreferenced second slot (P==1) that triggers all-voices reset.

## CPU Assessment

baseline: negligible
polyphonic: false
scalingFactors: []

## Notes

The `voice_manager_base` class includes a custom editor component that displays the number of active voices and provides a panic button. The editor accesses `VoiceResetter` from the PolyHandler for voice count display and manual all-voices reset. The unreferenced parameter slot 1 (all-voices kill) may be intended for future use or internal debugging.
