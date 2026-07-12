Path::addArc(Array area, Number fromRadians, Number toRadians) -> undefined

Thread safety: SAFE
Adds an arc (section of an ellipse outline) to the path. The arc traces the
ellipse defined by the bounding rectangle from fromRadians to toRadians.
Angles are measured clockwise from the 3 o'clock position (JUCE convention).

Required setup:
  const var p = Content.createPath();

Dispatch/mechanics:
  ApiHelpers::getRectangleFromVar(area) -> juce::Path::addArc(rect, from, to, true)
  Always starts a new sub-path (startAsNewSubPath=true internally).

Pair with:
  addPieSegment -- for wedge shapes with inner cutout (knob arcs)
  startNewSubPath -- anchor bounds before addArc in normalized coordinate paths
  createStrokedPath -- convert thin arc to fillable shape for gradients

Anti-patterns:
  - Do NOT use addArc alone in a normalized [0,0,1,1] path without anchoring
    bounds via startNewSubPath(0,0) and startNewSubPath(1,1) first -- the arc's
    bounding box only covers the arc segment, causing unexpected scaling.
  - Do NOT expect addArc to connect to the current sub-path position -- it always
    starts a new sub-path. Use lineTo to the arc start point first if continuity
    is needed.

Source:
  ScriptingGraphics.cpp  PathObject::addArc()
    -> SANITIZED() on fromRadians, toRadians
    -> ApiHelpers::getRectangleFromVar(area)
    -> p.addArc(rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight(), from, to, true)
