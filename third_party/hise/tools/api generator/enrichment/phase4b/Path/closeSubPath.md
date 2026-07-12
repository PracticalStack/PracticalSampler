Path::closeSubPath() -> undefined

Thread safety: SAFE
Closes the current sub-path by adding a straight line back to the sub-path's
start point. Required for shapes that need to be filled -- Graphics.fillPath
fills the interior of closed sub-paths only.

Pair with:
  startNewSubPath -- begins the sub-path that closeSubPath closes
  lineTo/quadraticTo/cubicTo -- drawing commands between start and close

Source:
  ScriptingGraphics.cpp  PathObject::closeSubPath()
    -> p.closeSubPath()
