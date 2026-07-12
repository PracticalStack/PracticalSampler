MidiList::setRange(int startIndex, int numToFill, int value) -> undefined

Thread safety: SAFE
Sets a contiguous range of slots to the same value. startIndex is clamped to [0, 127] and numToFill is clamped to min(numToFill, 127 - startIndex).
Required setup:
  const var list = Engine.createMidiList();
  list.setRange(0, 12, 100); // Fill slots 0-11
Dispatch/mechanics:
  Clamps inputs via jlimit/jmin. Loops from startIndex to numToFill (used as absolute end bound, NOT relative count). Updates numValues by accumulating a branchless delta per slot, then applies delta after loop.
Pair with: fill -- sets all 128 slots; setValue -- sets individual slots
Anti-patterns:
  - numToFill is used as an absolute end index in the loop, not a count relative to startIndex. setRange(10, 5, 99) fills zero slots because the loop condition (10 < 5) is immediately false. Always ensure numToFill > startIndex.
Source:
  ScriptingApiObjects.cpp:134  setRange() -- jlimit/jmin clamping, loop i=startIndex; i<numToFill with branchless delta
