Graphics::flip(Integer horizontally, Array totalArea) -> undefined

Thread safety: UNSAFE -- allocates a new draw action containing an AffineTransform
Applies a mirror transform. horizontally=true flips left-to-right around the vertical
center of totalArea; false flips top-to-bottom. Affects all subsequent drawing in the
same paint callback. totalArea defines the reflection axis (converted to int rectangle).

Anti-patterns:
  - Flip is cumulative: calling flip(true, ...) twice restores original orientation
  - totalArea converted to integer -- sub-pixel mirror axis precision is lost

Source:
  ScriptingGraphics.cpp  GraphicsObject::flip()
    -> AffineTransform: scale(-1, 1) + translate(width, 0) for horizontal
    -> AffineTransform: scale(1, -1) + translate(0, height) for vertical
