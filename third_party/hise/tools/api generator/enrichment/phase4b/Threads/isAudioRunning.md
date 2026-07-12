Threads::isAudioRunning() -> Integer

Thread safety: SAFE
Returns true if the audio callback is currently active. Returns false during
load operations or when audio processing has been suspended (e.g., by
killVoicesAndCall).

Dispatch/mechanics:
  KillStateHandler::isAudioRunning() -- checks internal audio state flag

Pair with:
  killVoicesAndCall -- suspends audio processing (makes isAudioRunning return false)

Source:
  ScriptingApi.h:1860  isAudioRunning() const
    -> KillStateHandler::isAudioRunning()
