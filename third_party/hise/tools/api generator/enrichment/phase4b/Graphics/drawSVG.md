Graphics::drawSVG(ScriptObject svgObject, Array bounds, Double opacity) -> undefined

Thread safety: UNSAFE -- allocates a new draw action, stores a reference to the SVG object
Draws an SVG object within the specified bounds at the given opacity.
svgObject must be created via Content.createSVG(base64String). Does not require a layer.

Anti-patterns:
  - Not a valid SVG object triggers script error "not a SVG object"

Source:
  ScriptingGraphics.cpp  GraphicsObject::drawSVG()
    -> draw action stores var reference to SVG object
    -> SVG::draw(g, bounds, opacity) called on UI thread via JUCE Drawable system
