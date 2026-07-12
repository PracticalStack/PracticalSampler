Graphics::applyGamma(Double gamma) -> undefined

Thread safety: UNSAFE -- allocates a new PostActionBase object
Applies gamma correction to the current layer's pixels. Values < 1.0 brighten
(lift shadows), > 1.0 darken (crush shadows), 1.0 = no change.
Requires an active layer via beginLayer().

Pair with:
  beginLayer/endLayer -- required wrapper for this post-processing effect

Anti-patterns:
  - Do NOT call without an active layer -- triggers script error
    "You need to create a layer for applying gamma"

Source:
  ScriptingGraphics.cpp  GraphicsObject::applyGamma()
    -> PostGraphicsRenderer::applyGamma() on layer's offscreen image
