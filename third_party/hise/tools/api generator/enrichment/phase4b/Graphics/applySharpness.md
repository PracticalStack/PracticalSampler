Graphics::applySharpness(Number delta) -> undefined

Thread safety: UNSAFE -- allocates a new PostActionBase object
Applies sharpening (positive values) or softening (negative values) to the current
layer's pixels. 0 = no change. Value is cast to int internally.
Requires an active layer via beginLayer().

Pair with:
  beginLayer/endLayer -- required wrapper for this post-processing effect

Anti-patterns:
  - Do NOT call without an active layer -- triggers script error
    "You need to create a layer for applySharpness"

Source:
  ScriptingGraphics.cpp  GraphicsObject::applySharpness()
    -> PostGraphicsRenderer::applySharpness() on layer's offscreen image
