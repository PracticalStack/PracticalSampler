ScriptModulationMatrix::canConnect(String source, String target) -> Integer

Thread safety: UNSAFE -- iterates ValueTree children internally.
Checks whether a modulation connection between the given source and target can
be made. Returns true if no connection exists between them, false if it already
exists or if the source ID is not found.

Required setup:
  const var mm = Engine.createModulationMatrix("Global Modulator Container0");

Dispatch/mechanics:
  Looks up sourceId in sourceList -> iterates matrixData ValueTree children
    -> checks if a connection with matching SourceIndex and TargetId already exists

Pair with:
  connect -- use canConnect to check before calling connect
  getSourceList/getTargetList -- validate IDs before passing to canConnect

Anti-patterns:
  - Do NOT use return value to distinguish "unknown source" from "connection
    exists" -- both return false with no way to tell them apart.

Source:
  ScriptModulationMatrix.cpp  canConnect()
    -> finds sourceIndex in sourceList
    -> iterates container->matrixData children checking SourceIndex + TargetId
