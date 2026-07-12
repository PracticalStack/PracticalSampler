Path::setBounds(Array boundingBox) -> undefined

Thread safety: SAFE
Expands the path's reported bounding box by adding invisible anchor points at
the top-left and bottom-right corners of the given rectangle. Does NOT transform
or resize any visible geometry. Useful for ensuring consistent bounding box
for layout purposes (e.g., when Graphics.fillPath scales from bounds).

Dispatch/mechanics:
  ApiHelpers::getRectangleFromVar(boundingBox)
  -> p.startNewSubPath(rect.getTopLeft())
  -> p.startNewSubPath(rect.getBottomRight())
  Two invisible points added -- no visible lines or curves.

Anti-patterns:
  - Do NOT use setBounds expecting it to resize the path -- this is the most
    common Path mistake. Use scaleToFit to transform path geometry into a
    target area.
  - Invisible anchor points are permanent. Calling setBounds multiple times
    accumulates points (only outermost coordinates affect reported bounds).
    Call clear() to remove them.

Source:
  ScriptingGraphics.cpp  PathObject::setBounds()
    -> p.startNewSubPath(tb.getTopLeft())
    -> p.startNewSubPath(tb.getBottomRight())
