Graphics::applyVignette(Double amount, Double radius, Double falloff) -> undefined

Thread safety: UNSAFE -- allocates a new PostActionBase object
Applies a vignette effect (darkened corners) to the current layer's pixels.
amount = intensity, radius = size of unaffected center, falloff = transition curve.
Requires an active layer via beginLayer().

Pair with:
  beginLayer/endLayer -- required wrapper for this post-processing effect

Anti-patterns:
  - Do NOT call without an active layer -- triggers script error (the error message
    incorrectly says "applySepia" instead of "applyVignette" due to a copy-paste bug)

Source:
  ScriptingGraphics.cpp  GraphicsObject::applyVignette()
    -> PostGraphicsRenderer::applyVignette() on layer's offscreen image
