Graphics::endLayer() -> undefined

Thread safety: UNSAFE -- pops layer from stack (ReferenceCountedArray::removeLast)
Ends the current layer and composites it onto the parent canvas. During UI-thread
rendering: layer's internal actions render to offscreen image, post-processing
effects applied via PostGraphicsRenderer, result drawn onto parent surface.

Pair with:
  beginLayer -- every beginLayer must have a matching endLayer
  beginBlendLayer -- also closed by endLayer

Anti-patterns:
  - Calling without a prior beginLayer/beginBlendLayer is a silent no-op
  - Forgetting endLayer causes all subsequent draws to target the orphaned layer;
    flush() discards the layer without compositing at callback end

Source:
  ScriptingGraphics.cpp  GraphicsObject::endLayer()
    -> DrawActions::Handler::endLayer()
    -> layerStack.removeLast()
