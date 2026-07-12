DspNetwork::processBlock(Array data) -> undefined

Thread safety: SAFE -- no allocations. Reads array/buffer pointers, constructs ProcessDataDyn on stack, delegates to process() which uses a non-blocking ScopedTryReadLock.
Processes audio data through the network's node graph. The data parameter is an array
of Buffer objects, one per channel. All buffers must have the same sample count.
If the network has not been initialized via prepareToPlay, processing is silently skipped.
If the connection lock is held (network being modified), processing is also skipped.
Required setup:
  const var nw = Engine.createDspNetwork("MyNetwork");
  nw.prepareToPlay(44100.0, 512);
Pair with:
  prepareToPlay -- must be called before processBlock has any effect
Anti-patterns:
  - Do NOT call without prior prepareToPlay -- buffers are left unmodified silently.
  - Do NOT pass buffers with mismatched sample counts -- triggers a script error.
  - Do NOT write manual processBlock logic when hosting in a ScriptFX processor --
    the processor handles audio routing automatically.
Source:
  DspNetwork.cpp:783  processBlock()
    -> converts Buffer array to ProcessDataDyn (stack allocation)
    -> validates matching sample counts across channels
  DspNetwork.cpp:704  process()
    -> ScopedTryReadLock on connectionLock (non-blocking, skips if locked)
    -> exceptionHandler.isOk() check
    -> getRootNode()->process(data)
