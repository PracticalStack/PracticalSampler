ScriptSlider::getValueNormalized() -> Double

Thread safety: SAFE
Converts current value to normalized 0..1 space using min, max, optional midpoint skew, and step configuration.

Required setup:
  const var sl = Content.addKnob("MySlider", 0, 0);
  sl.setRange(20.0, 20000.0, 1.0);

Dispatch/mechanics:
  value is normalized through the current HiSlider NormalisableRange mapping
  invalid range state returns deterministic fallback 0.0

Pair with:
  setValueNormalized -- inverse mapping from 0..1 into value range
  setRange/setMidPoint -- defines conversion curve and skew

Anti-patterns:
  - Do NOT rely on legacy -1 midpoint as a global disable token -- use "disabled" to bypass skew regardless of range.
  - Do NOT rely on defaults after custom mode/range edits without checking -- invalid range state collapses to 0.0.

Source:
  ScriptingApiContent.cpp:2054  normalized conversion helpers guarded by USE_BACKEND diagnostics
