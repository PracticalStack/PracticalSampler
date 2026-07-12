Engine::createBackgroundTask(String name) -> ScriptObject

Thread safety: UNSAFE -- heap allocation (extends Thread), WeakCallbackHolder setup
Creates a background task that executes heavyweight functions on a separate thread
without blocking audio or UI. The name parameter is the thread name for debugging.
The background thread is automatically aborted on script recompile.
Pair with:
  renderAudio -- another background processing pattern
Source:
  ScriptingApi.cpp  Engine::createBackgroundTask()
    -> new ScriptBackgroundTask(name) with pre-compile listener registration
