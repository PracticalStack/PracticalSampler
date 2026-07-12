Graphics::desaturate() -> undefined

Thread safety: UNSAFE -- allocates a new PostActionBase object
Removes all colour saturation from the current layer's pixels, converting to
grayscale. No parameters. Requires an active layer via beginLayer().

Pair with:
  beginLayer/endLayer -- required wrapper for this post-processing effect

Anti-patterns:
  - Do NOT call without an active layer -- triggers script error
    "You need to create a layer for desaturating"

Source:
  ScriptingGraphics.cpp  GraphicsObject::desaturate()
    -> PostGraphicsRenderer::desaturate() on layer's offscreen image
