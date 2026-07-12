ScriptModulationMatrix::clearAllConnections(String targetId) -> undefined

Thread safety: UNSAFE -- uses killVoicesAndCall to suspend audio before modifying the connection tree.
Removes all modulation connections for the specified target. If an empty string
is passed, removes all connections from the entire matrix.

Required setup:
  const var mm = Engine.createModulationMatrix("Global Modulator Container0");

Dispatch/mechanics:
  callSuspended() -> killVoicesAndCall() suspends audio
    -> if targetId empty: removes all children from matrixData ValueTree
    -> if targetId specified: removes children matching TargetId property

Pair with:
  connect -- to add individual connections back after clearing
  fromBase64 -- to restore a saved state after clearing

Anti-patterns:
  - Do NOT pass an empty variable expecting "no-op" -- empty string clears ALL
    connections from the entire matrix. Guard against accidental empty strings.

Source:
  ScriptModulationMatrix.cpp  clearAllConnections()
    -> callSuspended() -> MatrixIds::Helpers with undo manager
