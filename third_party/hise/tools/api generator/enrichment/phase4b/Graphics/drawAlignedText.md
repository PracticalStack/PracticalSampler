Graphics::drawAlignedText(String text, Array area, String alignment) -> undefined

Thread safety: UNSAFE -- allocates a new draw action containing a String copy
Draws a single line of text within the specified area using the specified alignment.
Uses the current font (setFont/setFontWithSpacing) and current colour (setColour/setGradientFill).
Preferred replacement for the deprecated drawText method.

Pair with:
  setFont/setFontWithSpacing -- must set font before drawing text
  setColour -- must set colour before drawing text
  drawAlignedTextShadow -- render shadow first, then text on top

Anti-patterns:
  - Do NOT use "center" or "centered" -- must use British spelling "centred".
    Invalid alignment strings trigger a script error.
  - Supported alignments: "left", "right", "top", "bottom", "centred",
    "centredTop", "centredBottom", "topLeft", "topRight", "bottomLeft", "bottomRight"

Source:
  ScriptingGraphics.cpp  GraphicsObject::drawAlignedText()
    -> new draw action with String copy and Justification from alignment string
    -> ApiHelpers::getJustification() resolves alignment string
