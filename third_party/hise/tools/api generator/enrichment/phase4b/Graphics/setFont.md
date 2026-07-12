Graphics::setFont(String fontName, Double fontSize) -> undefined

Thread safety: UNSAFE -- allocates a new draw action, resolves font via MainController
Sets the current font for text drawing and getStringWidth(). fontName resolves via
MainController::getFontFromString() (embedded, system, or global HISE font). fontSize
is in pixels, SANITIZED. Resets kerning to 0.0 -- use setFontWithSpacing for custom kerning.

Pair with:
  setFontWithSpacing -- version with kerning control
  drawAlignedText/drawMultiLineText -- text methods that use the current font

Source:
  ScriptingGraphics.cpp  GraphicsObject::setFont()
    -> mc->getFontFromString(fontName, fontSize)
    -> stores currentFontName, currentFontHeight, currentKerningFactor=0.0
