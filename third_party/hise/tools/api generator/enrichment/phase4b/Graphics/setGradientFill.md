Graphics::setGradientFill(Array gradientData) -> undefined

Thread safety: UNSAFE -- allocates a new draw action and constructs a JUCE ColourGradient
Sets the current fill to a gradient. Replaces setColour for subsequent drawing.
Three array formats:
  6 elements: [Colour1, x1, y1, Colour2, x2, y2] -- linear gradient
  7 elements: + isRadial (bool) at index 6
  7+ elements: + pairs of [StopColour, position(0-1)] after index 6 for multi-stop

Pair with:
  setColour -- solid colour alternative (also clears gradient)

Anti-patterns:
  - Arrays with fewer than 6 elements silently do nothing -- no gradient set, no error
  - Non-Array input triggers "Gradient Data is not sufficient"
  - Multi-stop: additional stops must come in pairs [colour, position]. Odd trailing
    elements cause out-of-bounds array access.

Source:
  ScriptingGraphics.cpp  GraphicsObject::setGradientFill()
    -> constructs ColourGradient from array data
    -> additional stops added via ColourGradient::addColour(position, colour)
