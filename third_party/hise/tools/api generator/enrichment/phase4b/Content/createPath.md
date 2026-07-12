Content::createPath() -> ScriptObject

Thread safety: UNSAFE -- heap-allocates a PathObject.
Creates a new Path object for vector drawing. Use Path methods (startNewSubPath,
lineTo, quadraticTo, cubicTo) to define the shape, or loadFromData() to load from
base64 data. Draw with Graphics.fillPath() or Graphics.drawPath() in paint routines.

Required setup:
  const var p = Content.createPath();
  p.loadFromData(base64Data);

Pair with:
  Path.startNewSubPath/lineTo -- define shape programmatically
  Path.loadFromData -- load from base64 encoded path data
  Graphics.fillPath/drawPath -- render in paint routines

Anti-patterns:
  - Do NOT create paths inside paint routines -- allocates a new object on every
    repaint. Create once at init or namespace scope, reuse everywhere.

Source:
  ScriptingApiContent.cpp:8460  Content::createPath()
    -> new PathObject (heap allocation)
    -> Takes NO arguments (wrapper calls CHECK_ARGUMENTS(0))
