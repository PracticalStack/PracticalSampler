Graphics::beginBlendLayer(String blendMode, Double alpha) -> undefined

Thread safety: UNSAFE -- allocates a new BlendingLayer object and pushes onto layer stack
Begins a layer that composites using a Photoshop-style blend mode. All subsequent draw
calls render to an offscreen image; endLayer() composites using the specified blend mode
and alpha. Supports 25 gin library blend modes (case-sensitive strings).

Dispatch/mechanics:
  DrawActions::Handler::beginBlendLayer() resolves blend mode string
  -> pushes BlendingLayer onto layerStack
  -> subsequent draw actions target the blend layer
  -> endLayer() composites with gin::BlendMode function

Pair with:
  endLayer -- must close the blend layer

Anti-patterns:
  - Invalid blend mode strings silently fail -- no layer is created, no error reported.
    Subsequent draws go to the parent canvas instead of the intended blend layer.
  - Blend mode strings are case-sensitive: "Multiply" works, "multiply" does not

Source:
  ScriptingGraphics.cpp  GraphicsObject::beginBlendLayer()
    -> DrawActions::Handler::beginBlendLayer()
    -> BlendingLayer extends ActionLayer with gin::BlendMode compositing
  MiscComponents.h  BlendingLayer
