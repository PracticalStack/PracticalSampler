Graphics::drawMultiLineText(String text, Array xy, Number maxWidth, String alignment, Double leading) -> undefined

Thread safety: UNSAFE -- allocates a new draw action containing a String copy
Draws text that wraps at maxWidth. xy is a [x, y] point (NOT [x, y, w, h]).
y is the baseline position of the first line. alignment controls horizontal
placement of each wrapped line. leading adds extra vertical spacing between lines.

Pair with:
  setFont/setFontWithSpacing -- must set font before drawing text
  setColour -- must set colour before drawing text

Anti-patterns:
  - xy is a 2-element [x, y] point, NOT a 4-element area. Passing [x, y, w, h]
    uses only the first two elements and ignores the rest.
  - y is the text baseline, not the top -- first line appears slightly above y
  - Both xy values and maxWidth are cast to int -- no sub-pixel positioning

Source:
  ScriptingGraphics.cpp  GraphicsObject::drawMultiLineText()
    -> int startX = (int)xy[0]; int baseLineY = (int)xy[1];
    -> new draw action with word-wrapping at maxWidth
