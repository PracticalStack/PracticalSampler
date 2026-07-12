MidiList::setValue(int index, int value) -> undefined

Thread safety: SAFE
Sets the value at the given index. Out-of-range indices (negative or >= 128) are silently ignored. The internal numValues counter is updated branchlessly -- increments when a -1 slot receives a non-(-1) value, decrements on the reverse.
Required setup:
  const var list = Engine.createMidiList();
  list.setValue(60, 127);
Dispatch/mechanics:
  Bounds-checks with isPositiveAndBelow(index, 128). Computes counter delta branchlessly: numValues += doSomething * (elementIsClear * 2 - 1). Then writes data[index] = value.
Pair with: getValue -- read/write counterpart; setRange -- bulk write
Anti-patterns:
  - Despite Doxygen comment ("between -127 and 128"), values are plain ints with no clamping. Any int can be stored.
  - Out-of-range index access is silent -- no error reported. Check your indices if values seem missing.
Source:
  ScriptingApiObjects.cpp:122  setValue() -- bounds check, branchless numValues update, data[index] = value
