ScriptPanel::repaint() -> undefined

Thread safety: SAFE
Schedules an asynchronous repaint of this panel. Safe to call from any thread --
checks the current thread and dispatches appropriately.
Dispatch/mechanics:
  On scripting/sample-loading/message thread: internalRepaint() directly
  On other threads (e.g. audio): deferred via JavascriptThreadPool.addDeferredPaintJob()
  -> LowPriorityCallbackExecution -> internalRepaintIdle()
  -> engine->callExternalFunction(paintRoutine) with GraphicsObject
  -> graphics->getDrawHandler().flush()
Pair with:
  setPaintRoutine -- register the paint function called on repaint
Anti-patterns:
  - Do NOT call repaint() unconditionally in every timer tick -- compare the new
    value to the stored value first to avoid redundant paint cycles
Source:
  ScriptingApiContent.cpp  ScriptPanel::repaint()
    -> internalRepaint(false) or addDeferredPaintJob(this)
