Engine::getStringWidth(String text, String fontName, float fontSize, float fontSpacing) -> Double

Thread safety: UNSAFE -- iterates custom typeface list (string comparisons, font metrics)
Returns pixel width of text rendered with the given font properties. Falls back to
default font if fontName does not match any loaded custom typeface.
Source:
  ScriptingApi.cpp  Engine::getStringWidth()
    -> looks up fontName in custom typefaces -> Font::getStringWidthFloat()
