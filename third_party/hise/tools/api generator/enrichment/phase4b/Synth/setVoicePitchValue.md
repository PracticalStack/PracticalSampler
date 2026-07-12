Synth::setVoicePitchValue(Integer voiceIndex, Double pitchValue) -> undefined

Thread safety: SAFE -- writes a float member on the target voice object via direct indexed array access, no allocations, no locks.
Applies a pitch ratio to a specific voice by voice index. 1.0 = original pitch, 0.5 = one octave
down, 2.0 = one octave up. Documented range is 0.5-2.0 but no clamping is applied.

Dispatch/mechanics:
  owner->setScriptPitchValue(voiceIndex, pitchValue)
  -> jmax(0, voiceIndex) clamps negative indices to 0
  -> if voiceIndex >= voices.size(), silently does nothing

Pair with:
  setVoiceGainValue -- per-voice gain control using the same voice index
  addPitchFade -- event-ID-based pitch control (alternative approach)

Anti-patterns:
  - Do NOT pass values outside 0.5-2.0 without testing -- no clamping is applied at the
    scripting API level; behavior outside this range is synth-implementation-dependent.
  - The pitchValue is cast from double to float internally -- very precise pitch values may
    lose precision in the conversion.

Source:
  ScriptingApi.cpp  Synth::setVoicePitchValue()
    -> owner->setScriptPitchValue(voiceIndex, (float)pitchValue)
