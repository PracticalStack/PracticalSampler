Graphics::applyGradientMap(Colour darkColour, Colour brightColour) -> undefined

Thread safety: UNSAFE -- allocates a new PostActionBase object and converts colour vars
Maps brightness values of the current layer's pixels to a two-colour gradient.
Dark pixels remap toward darkColour, bright pixels toward brightColour (duotone effect).
Requires an active layer via beginLayer().

Pair with:
  beginLayer/endLayer -- required wrapper for this post-processing effect

Anti-patterns:
  - Do NOT call without an active layer -- triggers script error
    "You need to create a layer for applyGradientMap"

Source:
  ScriptingGraphics.cpp  GraphicsObject::applyGradientMap()
    -> constructs ColourGradient from two colours
    -> PostGraphicsRenderer::applyGradientMap() on layer's offscreen image
