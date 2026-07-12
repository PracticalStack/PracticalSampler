Graphics::beginLayer(Integer drawOnParent) -> undefined

Thread safety: UNSAFE -- allocates a new ActionLayer object and pushes onto layer stack
Starts an offscreen layer. Subsequent draw calls record into this layer's action list.
Post-processing effects (blur, HSL, mask, etc.) can be applied before endLayer()
composites the result. drawOnParent=false starts blank; true captures parent content.

Dispatch/mechanics:
  DrawActions::Handler::beginLayer() pushes ActionLayer onto layerStack
  -> subsequent addDrawAction() calls target the layer's internalActions
  -> post-processing calls target the layer's postActions
  -> endLayer() renders internalActions to offscreen image, applies postActions
     via PostGraphicsRenderer, composites result onto parent

Pair with:
  endLayer -- every beginLayer must have a matching endLayer

Anti-patterns:
  - Forgetting endLayer() leaves the layer on the stack -- subsequent draw calls
    target the orphaned layer, and flush() discards it without compositing

Source:
  ScriptingGraphics.cpp  GraphicsObject::beginLayer()
    -> DrawActions::Handler::beginLayer()
  MiscComponents.h  ActionLayer (internalActions + postActions + PostGraphicsRenderer::DataStack)
