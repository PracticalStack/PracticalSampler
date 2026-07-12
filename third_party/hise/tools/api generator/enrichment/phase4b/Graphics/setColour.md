Graphics::setColour(Colour colour) -> undefined

Thread safety: UNSAFE -- allocates a new draw action and converts the colour var
Sets the current drawing colour for all subsequent shape and text operations.
Persists until the next setColour or setGradientFill call. Colour format is
0xAARRGGBB. Omitting alpha (e.g., 0xFF0000) produces 0x00FF0000 (alpha=0, invisible).

Pair with:
  setGradientFill -- gradient alternative to solid colour
  setOpacity -- modifies transparency independently of colour

Anti-patterns:
  - Do NOT omit the alpha channel -- 0xFF0000 is NOT red, it is 0x00FF0000
    (fully transparent). Use 0xFFFF0000 for opaque red.

Source:
  ScriptingGraphics.cpp  GraphicsObject::setColour()
    -> getCleanedObjectColour() converts var to Colour (int64, hex string, large int string)
