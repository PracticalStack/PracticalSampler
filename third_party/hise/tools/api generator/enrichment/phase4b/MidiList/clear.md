MidiList::clear() -> undefined

Thread safety: SAFE
Resets all 128 slots to sentinel value -1 and sets the internal non-empty counter to zero. Delegates to fill(-1).
Required setup:
  const var list = Engine.createMidiList();
  list.clear();
Dispatch/mechanics:
  Calls fill(-1) internally, which loops over all 128 slots setting each to -1 and sets numValues = 0.
Pair with: fill -- clear() is fill(-1); use fill(value) for non-sentinel initialization
Source:
  ScriptingApiObjects.cpp:82  clear() -> fill(-1)
