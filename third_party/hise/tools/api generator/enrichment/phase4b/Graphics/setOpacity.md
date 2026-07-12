Graphics::setOpacity(Double alphaValue) -> undefined

Thread safety: UNSAFE -- allocates a new draw action
Sets global transparency for all subsequent drawing (0.0 = transparent, 1.0 = opaque).
Multiplies with the current colour's alpha. Unlike setColour (which sets complete colour
including alpha), setOpacity modifies transparency independently. Values outside 0-1
clamped by JUCE internally. NOT SANITIZED against NaN/Inf.

Pair with:
  setColour -- sets colour with its own alpha channel
  beginBlendLayer -- for blend-mode compositing with alpha

Source:
  ScriptingGraphics.cpp  GraphicsObject::setOpacity()
    -> new draw action calling JUCE Graphics::setOpacity()
