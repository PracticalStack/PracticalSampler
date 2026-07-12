Graphics::drawAlignedTextShadow(String text, Array area, String alignment, JSON shadowData) -> undefined

Thread safety: UNSAFE -- allocates a new draw action, constructs a melatonin shadow object
Renders a blurred shadow behind text at the specified alignment. Only draws the shadow,
NOT the text itself. Call drawAlignedText separately with the same params to draw text on top.
shadowData properties: Colour (0xAARRGGBB), Offset ([x,y]), Radius (int), Spread (int), Inner (bool).

Pair with:
  drawAlignedText -- must call separately to draw the visible text on top of shadow
  setFont/setFontWithSpacing -- must set font before drawing

Anti-patterns:
  - Do NOT expect this method to draw the text -- it only draws the shadow.
    Forgetting to call drawAlignedText afterward leaves invisible text.
  - shadowData must be a JSON object -- passing a number or string triggers
    "shadowData needs to be a JSON object with the shadow parameters"
  - Uses British spelling "centred" for alignment, same as drawAlignedText

Source:
  ScriptingGraphics.cpp  GraphicsObject::drawAlignedTextShadow()
    -> ApiHelpers::getShadowParameters() parses JSON
    -> melatonin::DropShadow or melatonin::InnerShadow based on Inner property
