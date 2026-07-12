Graphics::fillRect(Array area) -> undefined

Thread safety: UNSAFE -- allocates a new draw action
Fills a rectangle with the current colour. Area is [x, y, width, height].
For outline only, use drawRect.

Pair with:
  drawRect -- outline version
  setColour -- must set colour before drawing

Anti-patterns:
  - Do NOT pass x, y, w, h as separate arguments -- must be a single [x, y, w, h] array.
    Backend diagnostic catches this common mistake.

Source:
  ScriptingGraphics.cpp  GraphicsObject::fillRect()
    -> new draw action with area
