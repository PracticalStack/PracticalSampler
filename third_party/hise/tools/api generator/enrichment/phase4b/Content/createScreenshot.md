Content::createScreenshot(var area, ScriptObject directory, String name) -> undefined

Thread safety: UNSAFE -- blocking operation, waits for screenshot listeners (shaders, etc.) to be ready, involves file I/O.
Creates a PNG screenshot of a specified area and saves it to a directory. The area
parameter can be a ScriptComponent reference (captures that component's bounds) or a
[x,y,w,h] array for arbitrary coordinates. Blocks until all screenshot listeners are ready.

Required setup:
  const var dir = FileSystem.getFolder(FileSystem.Desktop);
  Content.createScreenshot(myPanel, dir, "screenshot");

Dispatch/mechanics:
  area -> ScriptComponent: uses getGlobalBounds()
  area -> Array: uses [x,y,w,h] directly
  Waits for all screenshotListeners to be ready
  Saves PNG to directory (creates directory if needed)

Pair with:
  addVisualGuide -- add layout guides before capturing
  FileSystem.getFolder -- obtain a ScriptFile directory reference

Anti-patterns:
  - Does nothing if no screenshot listeners are registered (screenshotListeners
    array is empty).

Source:
  ScriptingApiContent.cpp:8764  Content::createScreenshot()
    -> resolves area (ScriptComponent or [x,y,w,h])
    -> waits for screenshotListeners readiness
    -> PNG file I/O
