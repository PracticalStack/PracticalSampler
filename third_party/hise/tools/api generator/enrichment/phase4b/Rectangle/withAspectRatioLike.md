Rectangle::withAspectRatioLike(var otherRect) -> ScriptObject

Thread safety: SAFE
Returns the largest Rectangle fitting within this one while matching the aspect ratio
of otherRect. The result is centered on the unconstrained axis.

Dispatch/mechanics:
  Computes ar = other.height / other.width
  If ar > 1.0 (portrait): fills height, centers horizontally
  Else (landscape/square): fills width, centers vertically
  Only the aspect ratio of otherRect matters -- position and size are ignored.

Anti-patterns:
  - Invalid argument silently returns a copy of the original rectangle unchanged.
  - [BUG] Passing a rectangle with zero width causes division by zero (non-finite results).

Source:
  RectangleDynamicObject.cpp  ADD_FUNCTION(withAspectRatioLike, ...)
    -> getRectangleArgs(a, other) -> custom aspect ratio calculation
    -> centers result within source rectangle
