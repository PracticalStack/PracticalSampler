Engine::getTextForValue(double value, String converterMode) -> String

Thread safety: WARNING -- string construction
Converts a numeric value to formatted text using built-in converter modes:
"Frequency" (Hz/kHz), "Time" (ms/s), "TempoSync" (1/4 etc), "Pan" (L/C/R),
"NormalizedPercentage" (%), "Decibel" (dB/-INF), "Semitones" (+N st).
Anti-patterns:
  - Invalid mode string silently falls back to plain integer conversion with no warning
Pair with:
  getValueForText -- inverse (parse formatted text back to number)
Source:
  ScriptingApi.cpp  Engine::getTextForValue()
    -> ValueToTextConverter(mode).getTextForValue(value)
