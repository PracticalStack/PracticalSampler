Engine::createTransportHandler() -> ScriptObject

Thread safety: UNSAFE -- heap allocation
Creates a transport handler for callback-based host transport events (play/stop, tempo,
time signature, beat position). Modern replacement for Engine.getPlayHead(), which
returns a stale/empty object. Registers as a TempoListener for real-time DAW updates.
Pair with:
  getHostBpm -- simpler BPM-only query
  getPlayHead -- deprecated, use this instead
Source:
  ScriptingApi.cpp  Engine::createTransportHandler()
    -> new TransportHandler -> registers as TempoListener
