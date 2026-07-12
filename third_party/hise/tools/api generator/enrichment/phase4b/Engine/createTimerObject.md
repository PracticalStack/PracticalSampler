Engine::createTimerObject() -> ScriptObject

Thread safety: UNSAFE -- heap allocation
Creates a timer object for periodic callback execution. After creation, use
setTimerCallback(callback) and startTimer(intervalMs). The callback runs on the
UI thread -- suitable for display updates, polling, animation, deferred processing.
Multiple independent timers can coexist.
Source:
  ScriptingApi.cpp  Engine::createTimerObject()
    -> new TimerObject
