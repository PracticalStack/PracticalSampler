Path (object)
Obtain via: Content.createPath() or Content.createPath(data)
  Content.createPath(data) passes the argument to loadFromData, enabling
  one-liner initialisation from base64 strings or byte arrays.

Vector path object for constructing and manipulating 2D shapes for UI rendering.
Wraps JUCE's Path class with a full suite of drawing primitives (lines, curves,
arcs, geometric shapes), spatial queries (bounds, intersection, containment),
and serialization. Consumed by Graphics methods (fillPath, drawPath) in paint callbacks.

Complexity tiers:
  1. Simple shapes: startNewSubPath, lineTo, closeSubPath, addArc. Triangles,
     arrows, icons, basic rotary knob backgrounds. No curves, no stroking.
  2. Rotary knob LAF: addArc with computed angles, startNewSubPath for bounds
     anchoring, drawPath/fillPath with stroke thickness. Requires understanding
     JUCE angle convention (clockwise from 3 o'clock).
  3. Serialized icons: Content.createPath(data) for one-liner initialisation,
     or loadFromData with base64 strings or byte arrays. Build icon libraries
     as JSON objects with fillPath for rendering.
  4. Advanced geometry: + quadraticTo, cubicTo, createStrokedPath, scaleToFit,
     getYAt. Envelope editors, tooltip shapes, dashed lines, interactive curves.

Practical defaults:
  - Use normalized coordinates [0, 0, 1, 1] for paths rendered at different sizes
    via g.fillPath(path, targetArea). Graphics methods scale automatically.
  - For rotary knob arcs, use angular range ~2.3-2.7 radians from center
    (gap at bottom). -ARC to +ARC for background, -ARC to -ARC + 2*ARC*value
    for active arc.
  - Anchor bounds with startNewSubPath(0,0) and startNewSubPath(1,1) before
    addArc([0,0,1,1], ...) to ensure the full unit square is the reference frame.
  - Use createStrokedPath(thickness, []) to convert arc outlines into fillable
    shapes for gradients, shadows, or other fill effects.
  - Store icon paths as const var at init scope and reuse across paint calls.

Common mistakes:
  - Using setBounds expecting it to resize the path -- it only adds invisible
    anchor points to expand reported bounds. Use scaleToFit instead.
  - Creating Path objects inside paint routines for static shapes -- create as
    const var at init scope, only rebuild in paint when geometry depends on value.
  - Using addArc without anchoring bounds first -- arc bounding box only covers
    the arc segment, causing unexpected scaling when rendered with drawPath.
  - Passing NaN/Inf to quadraticTo or cubicTo -- these methods lack SANITIZED()
    protection and will silently corrupt path geometry.

Example:
  const var p = Content.createPath();
  p.startNewSubPath(0.0, 0.0);
  p.lineTo(1.0, 0.0);
  p.lineTo(0.5, 1.0);
  p.closeSubPath();

Methods (32):
  addArc                          addArrow
  addEllipse                      addPieSegment
  addPolygon                      addQuadrilateral
  addRectangle                    addRoundedRectangle
  addRoundedRectangleCustomisable addStar
  addTriangle                     clear
  closeSubPath                    contains
  createStrokedPath               cubicTo
  fromString                      getBounds
  getIntersection                 getLength
  getPointOnPath                  getRatio
  getYAt                          lineTo
  loadFromData                    quadraticTo
  roundCorners                    scaleToFit
  setBounds                       startNewSubPath
  toBase64                        toString
