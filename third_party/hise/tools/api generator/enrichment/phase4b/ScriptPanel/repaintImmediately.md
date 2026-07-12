ScriptPanel::repaintImmediately() -> undefined

Thread safety: SAFE
Schedules a repaint. Despite the name, this currently behaves identically to
repaint() -- it schedules an asynchronous repaint, not a synchronous one.
Anti-patterns:
  - Do NOT rely on the paint having completed after this call returns -- the name
    is misleading, it is asynchronous like repaint()
Pair with:
  repaint -- identical behavior
  setPaintRoutine -- register the paint function
Source:
  ScriptingApiContent.cpp  ScriptPanel::repaintImmediately()
    -> calls repaint() internally
