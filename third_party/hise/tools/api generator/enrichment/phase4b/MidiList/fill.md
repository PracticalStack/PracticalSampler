MidiList::fill(int valueToFill) -> undefined

Thread safety: SAFE
Sets all 128 slots to valueToFill. Updates numValues to 128 if value != -1, or 0 if value == -1. Fastest way to initialize a MidiList to a uniform value.
Required setup:
  const var list = Engine.createMidiList();
  list.fill(64);
Dispatch/mechanics:
  Loops over all 128 slots setting each to valueToFill. numValues is computed branchlessly: (int)(valueToFill != -1) * 128.
Pair with: clear -- clear() delegates to fill(-1); setRange -- for partial fills
Source:
  ScriptingApiObjects.cpp:74  fill() -- loops data[0..127], sets numValues branchlessly
