Engine::createMessageHolder() -> ScriptObject

Thread safety: UNSAFE -- heap allocation
Creates a persistent MIDI event storage object. Unlike the implicit Message object in
MIDI callbacks, a MessageHolder persists across callbacks and can be stored in arrays.
Used to construct MIDI events for renderAudio() or to pass structured MIDI data between
script sections.
Pair with:
  renderAudio -- accepts arrays of MessageHolder objects as input
Source:
  ScriptingApi.cpp  Engine::createMessageHolder()
    -> new ScriptingMessageHolder
