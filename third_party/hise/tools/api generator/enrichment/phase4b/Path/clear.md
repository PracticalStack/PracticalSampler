Path::clear() -> undefined

Thread safety: SAFE
Removes all lines, curves, and sub-paths, resetting the path to empty.
The bounding box is also reset. Use this to reuse a Path object for new
geometry rather than creating a new one.

Source:
  ScriptingGraphics.cpp  PathObject::clear()
    -> p.clear()
