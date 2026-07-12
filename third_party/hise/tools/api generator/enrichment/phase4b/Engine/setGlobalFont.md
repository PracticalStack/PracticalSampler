Engine::setGlobalFont(String fontName) -> undefined

Thread safety: UNSAFE -- font construction, LookAndFeel update
Sets the default font for all UI elements. Font must be loaded via loadFontAs()
first or be a system font. Empty string resets to default HISE font.
Anti-patterns:
  - Unrecognized font names silently fall back to system font with no warning
Pair with:
  loadFontAs -- load fonts before setting
Source:
  ScriptingApi.cpp  Engine::setGlobalFont()
    -> MainController::setGlobalFont(fontName)
