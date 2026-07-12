ScriptModulationMatrix::fromBase64(String b64) -> undefined

Thread safety: UNSAFE -- uses killVoicesAndCall to suspend audio before restoring the connection tree.
Restores the modulation matrix state from a previously exported Base64 string.
Decodes and decompresses (zstd) the data, then replaces all current connections.
Silently does nothing if the string is invalid or decompression fails.

Required setup:
  const var mm = Engine.createModulationMatrix("Global Modulator Container0");
  var savedState = mm.toBase64(); // save first

Dispatch/mechanics:
  Base64 decode -> zstd decompress -> ValueTree
    -> callSuspended() -> replaces all children of matrixData with undo manager

Pair with:
  toBase64 -- export state before restoring
  clearAllConnections -- alternative for clearing without restoring

Source:
  ScriptModulationMatrix.cpp  fromBase64()
    -> ZDefaultCompressor::expand() -> callSuspended() -> restoreFromValueTree()
