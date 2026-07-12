ScriptSlider::updateValueFromProcessorConnection() -> undefined

Thread safety: UNSAFE
Refreshes slider value from its configured processor/parameter connection.
If no connection exists, it does nothing.

Required setup:
  const var sl = Content.addKnob("MySlider", 0, 0);
  sl.connectToModulatedParameter("SimpleGain1", "Gain");

Dispatch/mechanics:
  reads current connected target value and applies special parameter index rules
  -2 maps modulation intensity, -3 maps bypass 1.0/0.0, -4 maps inverted bypass 0.0/1.0

Pair with:
  connectToModulatedParameter -- defines the source to refresh from

Anti-patterns:
  - Do NOT skip this after external processor state restore -- slider UI can stay stale.

Source:
  ScriptingApiContent.cpp:2054  processor connection pull and value refresh path
