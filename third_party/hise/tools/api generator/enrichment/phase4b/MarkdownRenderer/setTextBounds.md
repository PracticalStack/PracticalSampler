MarkdownRenderer::setTextBounds(Array area) -> Double

Thread safety: UNSAFE -- acquires CriticalSection lock, iterates all parsed elements for layout height calculation (may trigger font metrics and cached layout allocations)
Sets the rendering area and returns the actual height required to display the full
parsed text at the specified width. Must be called before Graphics.drawMarkdownText()
or a script error is thrown. Height calculation is cached -- same width returns
cached result without recomputing.

Required setup:
  const var md = Content.createMarkdownRenderer();
  md.setText("# Title\nSome text.");
  var height = md.setTextBounds([0, 0, 400, 1000]);

Dispatch/mechanics:
  action.area = getRectangleFromVar(area) -- stores [x, y, w, h]
  -> ScopedLock on action.lock
  -> renderer.getHeightForWidth(width)
    -> if width == lastWidth, returns cached lastHeight
    -> otherwise iterates elements: sum of getTopMargin() + getHeightForWidthCached()

Pair with:
  setText -- set text before calling setTextBounds() for correct height
  Graphics.drawMarkdownText -- renders using the stored area; throws if area is empty

Anti-patterns:
  - Using the height from a previous setTextBounds() call after setText() changes
    the content -- the old height is stale. Call setTextBounds() again after each
    setText() to get the correct height.

Source:
  ScriptingGraphics.cpp  MarkdownObject::setTextBounds()
    -> ApiHelpers::getRectangleFromVar(area)
    -> ScopedLock(obj->lock)
    -> renderer.getHeightForWidth(width)
  MarkdownRenderer.h:40  getHeightForWidth() -- cached element iteration
  ScriptingGraphics.cpp:2165  GraphicsObject::drawMarkdownText() -- checks area.isEmpty()
