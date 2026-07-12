Synth::setVoiceGainValue(Integer voiceIndex, Double gainValue) -> undefined

Thread safety: SAFE -- writes a float member on the target voice object via direct indexed array access, no allocations, no locks.
Applies a linear gain factor to a specific voice by voice index. The value is stored as the voice's
scriptGainValue and applied as an additional multiplier alongside the modulation chain output.

Dispatch/mechanics:
  owner->setScriptGainValue(voiceIndex, gainValue)
  -> jmax(0, voiceIndex) clamps negative indices to 0
  -> if voiceIndex >= voices.size(), silently does nothing

Pair with:
  setVoicePitchValue -- per-voice pitch control using the same voice index
  addVolumeFade -- event-ID-based volume control (alternative approach)

Anti-patterns:
  - Do NOT pass voice indices from a different synth's context -- out-of-range indices are
    silently ignored, making bugs invisible.
  - Negative voice indices are silently clamped to 0, not rejected -- passing -1 sets gain
    on voice 0 without error.

Source:
  ScriptingApi.cpp  Synth::setVoiceGainValue()
    -> owner->setScriptGainValue(voiceIndex, (float)gainValue)
