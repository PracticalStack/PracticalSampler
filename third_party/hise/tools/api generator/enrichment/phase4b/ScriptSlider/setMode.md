ScriptSlider::setMode(String mode) -> undefined

Thread safety: UNSAFE
Sets slider conversion and display mode (Frequency, Decibel, Time, TempoSync, Linear, Discrete, Pan, NormalizedPercentage).
If old mode defaults are untouched, range, step, suffix, and midpoint migrate to new mode defaults.

Required setup:
  const var sl = Content.addKnob("MySlider", 0, 0);

Dispatch/mechanics:
  mode string maps to HiSlider::Mode index and updates script property state
  default-range migration runs only when previous settings still match old-mode defaults
  wrapper applies mode-specific ranges and suffix behavior from HiSlider providers

Pair with:
  setRange/setMidPoint -- override or refine auto-migrated defaults
  setValueNormalized/getValueNormalized -- mode affects conversion interpretation

Anti-patterns:
  - Do NOT pass invalid mode strings -- runtime falls back internally and can leave property text/state mismatched.

Source:
  ScriptingApiContent.cpp:2054  setMode() string mapping and default-range migration
  MacroControlledComponents.cpp:1  HiSlider::getRangeForMode() and getSuffixForMode()
