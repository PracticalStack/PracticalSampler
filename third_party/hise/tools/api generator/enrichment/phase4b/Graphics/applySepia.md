Graphics::applySepia() -> undefined

Thread safety: UNSAFE -- allocates a new PostActionBase object
Applies a fixed sepia tone filter to the current layer's pixels, converting colours
to warm brownish tones. No parameters. Requires an active layer via beginLayer().

Pair with:
  beginLayer/endLayer -- required wrapper for this post-processing effect

Anti-patterns:
  - Do NOT call without an active layer -- triggers script error
    "You need to create a layer for applySepia"

Source:
  ScriptingGraphics.cpp  GraphicsObject::applySepia()
    -> PostGraphicsRenderer::applySepia() on layer's offscreen image
