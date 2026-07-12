Content::addVisualGuide(var guideData, var colour) -> undefined

Thread safety: UNSAFE -- iterates screenshotListeners and may trigger repaint operations.
Adds a visual overlay guide (line or rectangle) for layout debugging. Pass [x,y,w,h]
for a rectangle, [0,y] for a horizontal line, [x,0] for a vertical line. Passing any
non-array value (e.g., 0) clears all guides.

Dispatch/mechanics:
  Array length determines guide type:
    4 elements -> VisualGuide::Rectangle
    2 elements -> [0,y]=HorizontalLine, [x,0]=VerticalLine
    non-array  -> clears guides array
  Notifies screenshotListeners after adding

Pair with:
  createScreenshot -- capture the interface with guides visible

Anti-patterns:
  - Do NOT pass a 2-element array with both values non-zero (e.g., [50, 100]) --
    neither horizontal nor vertical code path matches, producing an uninitialized guide type.

Source:
  ScriptingApiContent.cpp:8973  Content::addVisualGuide()
    -> VisualGuide struct with Type enum {HorizontalLine, VerticalLine, Rectangle}
    -> stored in Array<VisualGuide> guides
