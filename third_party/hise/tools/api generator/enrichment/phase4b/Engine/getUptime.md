Engine::getUptime() -> Double

Thread safety: SAFE -- reads double (atomic/lock-free)
Returns engine uptime in seconds. When called from a MIDI callback with an active
HiseEvent, includes the event's sub-buffer timestamp offset for sample-accurate timing.
Source:
  ScriptingApi.cpp  Engine::getUptime()
    -> MainController::getUptime()
    -> if MIDI callback: adds event timestamp / sampleRate
