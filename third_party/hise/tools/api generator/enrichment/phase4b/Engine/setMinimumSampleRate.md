Engine::setMinimumSampleRate(Number minimumSampleRate) -> Integer

Thread safety: INIT -- calls refreshOversampling(), re-initializes oversampling chain
Sets minimum sample rate -- if device rate is lower, enables oversampling.
Clamped to 1.0..384000.0. Returns true if oversampling config changed.
Source:
  ScriptingApi.cpp  Engine::setMinimumSampleRate()
    -> MainController::setMinimumSamplerate() -> refreshOversampling()
