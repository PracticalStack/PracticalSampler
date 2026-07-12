Graphics::getStringWidth(String text) -> Double

Thread safety: WARNING -- string involvement, atomic ref-count operations
Returns the pixel width of text when rendered with the current font. Uses the font
name, size, and kerning stored locally (set via setFont/setFontWithSpacing). This is
the only Graphics method that returns a value instead of being a void draw command.

Dispatch/mechanics:
  MainController::getStringWidthFromEmbeddedFont(text, fontName, fontSize, kerning)
  -> measures text without needing the deferred JUCE Graphics context
  -> uses locally cached currentFontName, currentFontHeight, currentKerningFactor

Pair with:
  setFont/setFontWithSpacing -- must set font first for accurate measurement
  drawAlignedText -- use measured width to position text

Source:
  ScriptingGraphics.cpp  GraphicsObject::getStringWidth()
    -> mc->getStringWidthFromEmbeddedFont(text, currentFontName, currentFontHeight, currentKerningFactor)
