Graphics::applyHSL(Double hue, Double saturation, Double lightness) -> undefined

Thread safety: UNSAFE -- allocates a new PostActionBase object
Applies additive HSL colour grading to the current layer's pixels.
Hue: 0.5 = 180-degree shift, 1.0 = full rotation. Saturation/lightness: positive
increases, negative decreases. (0.0, 0.0, 0.0) = no change.
Requires an active layer via beginLayer().

Pair with:
  beginLayer/endLayer -- required wrapper for this post-processing effect

Anti-patterns:
  - Do NOT call without an active layer -- triggers script error
    "You need to create a layer for applying HSL"
  - Parameters are additive offsets, not absolute values

Source:
  ScriptingGraphics.cpp  GraphicsObject::applyHSL()
    -> PostGraphicsRenderer::applyHSL() on layer's offscreen image
