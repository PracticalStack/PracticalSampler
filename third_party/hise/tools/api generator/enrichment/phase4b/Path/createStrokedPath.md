Path::createStrokedPath(NotUndefined strokeData, Array dotData) -> ScriptObject

Thread safety: UNSAFE -- allocates new PathObject, JUCE Array for dash data, and performs path geometry computation
Creates and returns a new Path representing the outlined (stroked) version of
the current path. The original path is not modified.

Required setup:
  const var p = Content.createPath();
  p.startNewSubPath(0.0, 0.0);
  p.lineTo(200.0, 0.0);

Dispatch/mechanics:
  strokeData: numeric thickness OR JSON {"Thickness", "EndCapStyle", "JointStyle"}
    -> ApiHelpers::createPathStrokeType(strokeData)
  dotData: [] for solid, [dash, gap, ...] for dashed
    -> empty: PathStrokeType::createStrokedPath()
    -> non-empty: PathStrokeType::createDashedStroke()
  New PathObject returned with original bounds anchored via invisible startNewSubPath calls.

Pair with:
  Graphics.fillPath -- fill the stroked path for gradient/shadow effects on arcs
  roundCorners -- smooth corners before or after stroking

Anti-patterns:
  - Do NOT pass a non-array as dotData (e.g., a number) -- silently ignored,
    produces a solid stroke with no error.
  - Do NOT confuse parameter order: first param is stroke config, second is dash
    array. Passing thickness as dotData produces wrong results.

Source:
  ScriptingGraphics.cpp  PathObject::createStrokedPath()
    -> ApiHelpers::createPathStrokeType(strokeData) for EndCapStyle/JointStyle
    -> new PathObject(getScriptProcessor()) allocated
    -> np->p.startNewSubPath(p.getBounds().getTopLeft()/getBottomRight()) for bounds
    -> strokeType.createStrokedPath() or createDashedStroke()
