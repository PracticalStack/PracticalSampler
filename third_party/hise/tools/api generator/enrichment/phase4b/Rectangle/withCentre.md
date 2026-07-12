Rectangle::withCentre(Double newCentreX, Double newCentreY) -> ScriptObject

Thread safety: SAFE
Returns a new Rectangle with the same size, repositioned so its centre is at
the given coordinates.

Dispatch/mechanics:
  Constructs Point<double>(newCentreX, newCentreY)
  -> JUCE Rectangle::withCentre(Point)

Pair with:
  setCentre -- mutating alternative (modifies in place)
  withSizeKeepingCentre -- resize around the centre

Source:
  RectangleDynamicObject.cpp  ADD_FUNCTION(withCentre, ...)
    -> Point<double>(arg0, arg1) -> JUCE Rectangle::withCentre(point)
