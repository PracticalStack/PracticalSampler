Engine::getValueForText(String text, String convertedMode) -> Double

Thread safety: WARNING -- string parsing
Parses formatted text back to numeric value. Inverse of getTextForValue().
"1.5 kHz"->1500.0, "500ms"->500.0, "1/4"->5.0, "50L"->-50.0, "75%"->0.75,
"-INF"->-100.0, "+2 st"->2.0.
Anti-patterns:
  - Invalid mode string silently falls back to plain getDoubleValue() parsing
Pair with:
  getTextForValue -- forward conversion
Source:
  ScriptingApi.cpp  Engine::getValueForText()
    -> ValueToTextConverter(mode).getValueForText(text)
