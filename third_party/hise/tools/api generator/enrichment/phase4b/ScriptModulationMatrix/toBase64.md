ScriptModulationMatrix::toBase64() -> String

Thread safety: UNSAFE -- compresses ValueTree data and performs Base64 encoding (heap allocations).
Creates a Base64-encoded string representing the current state of all modulation
connections. The ValueTree data is compressed using zstd before encoding.

Required setup:
  const var mm = Engine.createModulationMatrix("Global Modulator Container0");

Dispatch/mechanics:
  container->matrixData ValueTree -> ZDefaultCompressor::compress()
    -> MemoryBlock -> Base64 string

Pair with:
  fromBase64 -- restore the state exported by this method

Source:
  ScriptModulationMatrix.cpp  toBase64()
    -> exportAsValueTree() -> ZDefaultCompressor::compress() -> Base64 encode
