Graphics::applyMask(ScriptObject path, Array area, Integer invert) -> undefined

Thread safety: UNSAFE -- allocates a new PostActionBase object, copies the Path, scales it to area
Applies a path-based alpha mask to the current layer. Pixels inside the path are kept,
outside made transparent (or vice versa when invert is true). Path is scaled to fit
the area via scaleToFit (non-uniform). Requires an active layer via beginLayer().

Pair with:
  beginLayer/endLayer -- required wrapper for this post-processing effect
  Content.createPath -- to create the mask path

Anti-patterns:
  - Do NOT call without an active layer -- triggers script error
    "You need to create a layer for applying a mask"
  - Path aspect ratio distortion if path and area ratios differ (non-uniform scaling)

Source:
  ScriptingGraphics.cpp  GraphicsObject::applyMask()
    -> path.scaleToFit(area, preserveProportions=false)
    -> PostGraphicsRenderer::applyMask() renders path to alpha image and composites
