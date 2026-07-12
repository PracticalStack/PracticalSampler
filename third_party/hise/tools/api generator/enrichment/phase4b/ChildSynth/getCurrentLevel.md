ChildSynth::getCurrentLevel(bool leftChannel) -> Double

Thread safety: SAFE
Returns the current peak display level for the specified channel. Pass true for
left, false for right. These are display values updated at UI refresh rate, not
real-time sample-accurate values.
Anti-patterns:
  - Do NOT pass raw peak values directly to UI -- fluctuates rapidly. Apply decay
    smoothing: level = Math.max(newPeak, level * 0.94)
Source:
  ScriptingApiObjects.cpp  getCurrentLevel()
    -> synth->getDisplayValues().outL or .outR
