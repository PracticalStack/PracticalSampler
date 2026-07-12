ScriptSlider::setValueNormalized(Double normalizedValue) -> undefined

Thread safety: SAFE
Maps normalized 0..1 input into current slider range and then calls setValue.

Required setup:
  const var sl = Content.addKnob("MySlider", 0, 0);
  sl.setRange(0.0, 100.0, 1.0);

Dispatch/mechanics:
  normalized input is converted through current NormalisableRange and optional midpoint skew
  mapped value is forwarded into setValue for state update and notifications

Pair with:
  getValueNormalized -- inverse conversion
  setRange/setMidPoint -- controls mapping curve and midpoint behavior

Anti-patterns:
  - Do NOT use with invalid range configuration -- value update is skipped.
  - Do NOT assume legacy -1 midpoint disables skew in all ranges -- use setMidPoint("disabled") for explicit no-skew mapping.
  - Do NOT assume callback logic ran -- call changed when callback-dependent workflows must execute.

Source:
  ScriptingApiContent.cpp:2054  normalized-to-value conversion then setValue dispatch
