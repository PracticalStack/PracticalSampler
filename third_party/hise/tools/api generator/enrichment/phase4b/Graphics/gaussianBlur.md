Graphics::gaussianBlur(Number blurAmount) -> undefined

Thread safety: UNSAFE -- allocates a new PostActionBase object
Applies a gaussian blur to the current layer's pixels. Smooth bell-curve distribution.
blurAmount clamped to 0-100, cast to int. For cheaper but blockier blur, use boxBlur.
Requires an active layer via beginLayer().

Pair with:
  beginLayer/endLayer -- required wrapper for this post-processing effect
  boxBlur -- cheaper but blockier alternative

Anti-patterns:
  - Do NOT call without an active layer -- triggers script error
    "You need to create a layer for gaussian blur"

Source:
  ScriptingGraphics.cpp  GraphicsObject::gaussianBlur()
    -> PostGraphicsRenderer::gaussianBlur() on layer's offscreen image
