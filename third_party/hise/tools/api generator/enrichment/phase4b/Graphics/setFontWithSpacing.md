Graphics::setFontWithSpacing(String fontName, Double fontSize, Double spacing) -> undefined

Thread safety: UNSAFE -- allocates a new draw action, resolves font via MainController
Sets the current font with an extra kerning (character spacing) factor. Identical to
setFont except spacing is applied via Font::setExtraKerningFactor(). 0.0 = same as setFont.
Positive widens, negative tightens. Affects getStringWidth() measurements consistently.

Pair with:
  setFont -- version without kerning (resets spacing to 0.0)
  getStringWidth -- returns width consistent with the current spacing

Source:
  ScriptingGraphics.cpp  GraphicsObject::setFontWithSpacing()
    -> mc->getFontFromString(fontName, fontSize)
    -> Font::setExtraKerningFactor(spacing)
    -> stores currentFontName, currentFontHeight, currentKerningFactor=spacing
