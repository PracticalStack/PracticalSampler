Graphics::addDropShadowFromAlpha(Colour colour, Number radius) -> undefined

Thread safety: UNSAFE -- creates a JUCE DropShadow object and a new heap-allocated draw action
Adds a drop shadow based on the alpha channel of previously drawn content. Unlike
drawDropShadow (rectangular), this reads the actual pixel alpha to generate a
shape-following shadow via the cached image mechanism.

Dispatch/mechanics:
  Uses wantsCachedImage() to receive parent component's rendered image snapshot
  -> DropShadow applied to alpha channel of cached image
  -> shadow rendered behind existing content

Pair with:
  drawDropShadowFromPath -- for path-based shadows with offset control
  drawDropShadow -- for simple rectangular shadows

Anti-patterns:
  - Do NOT call before drawing the shapes whose shadow you want -- the cached image
    is empty at that point, producing no visible shadow

Source:
  ScriptingGraphics.cpp  GraphicsObject::addDropShadowFromAlpha()
    -> new draw action with wantsCachedImage() override returning true
    -> setCachedImage() receives parent snapshot, applies DropShadow to alpha channel
