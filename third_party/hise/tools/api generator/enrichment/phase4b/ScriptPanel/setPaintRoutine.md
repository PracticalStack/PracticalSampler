ScriptPanel::setPaintRoutine(Function paintFunction) -> undefined

Thread safety: UNSAFE -- stores paint callback, may allocate canvas
Registers a paint function that receives a Graphics object for custom drawing.
Called when repaint() is invoked. Executes on the scripting thread via a
low-priority job. Canvas resolution accounts for high-DPI and global scale
factor (capped at 2x).
Callback signature: f(Graphics g)
Dispatch/mechanics:
  Stores WeakCallbackHolder(paintRoutine) with 1 parameter
  On repaint(): internalRepaintIdle() -> engine->callExternalFunction(paintRoutine)
  -> graphics->getDrawHandler().flush()
Anti-patterns:
  - Calling setImage() clears the paint routine -- conversely, setPaintRoutine()
    cancels fixed image mode
Pair with:
  repaint -- trigger the paint routine
Source:
  ScriptingApiContent.cpp  ScriptPanel::setPaintRoutine()
