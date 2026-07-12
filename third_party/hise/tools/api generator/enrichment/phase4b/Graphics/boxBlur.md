Graphics::boxBlur(Number blurAmount) -> undefined

Thread safety: UNSAFE -- allocates a new PostActionBase object
Applies a box blur (uniform average filter) to the current layer's pixels.
Cheaper than gaussianBlur but produces visible block artifacts at large radii.
blurAmount clamped to 0-100, cast to int. Requires an active layer via beginLayer().

Pair with:
  beginLayer/endLayer -- required wrapper for this post-processing effect
  gaussianBlur -- smoother but more expensive alternative

Anti-patterns:
  - Do NOT call without an active layer -- triggers script error
    "You need to create a layer for box blur"

Source:
  ScriptingGraphics.cpp  GraphicsObject::boxBlur()
    -> PostGraphicsRenderer::boxBlur() on layer's offscreen image
