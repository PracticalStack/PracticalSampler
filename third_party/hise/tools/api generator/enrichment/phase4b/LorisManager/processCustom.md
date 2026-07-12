LorisManager::processCustom(ScriptObject file, Function processCallback) -> undefined

Thread safety: UNSAFE -- iterates every breakpoint in every partial, calling script function synchronously for each.
Processes the analysed partial list using a custom callback invoked per breakpoint.
The callback receives a JSON object with read-only properties (channelIndex,
partialIndex, sampleRate, rootFrequency) and read-write properties (time, frequency,
phase, gain, bandwidth). Modify read-write properties directly on the object.
Callback signature: f(Object data)

Required setup:
  const var lm = Engine.getLorisManager();
  lm.analyse(audioFile, 440.0);

Dispatch/mechanics:
  initThreadController() -> WeakCallbackHolder wrapping script function (1 arg)
    -> LorisManager::processCustom(file, lambda)
    -> for each breakpoint: CustomPOD::toJSON() -> callSync() -> CustomPOD::writeJSON()
    -> writeJSON reads back only mutable properties (time, frequency, phase, gain, bandwidth)

Pair with:
  analyse -- file must be analysed first
  synthesise -- resynthesise after processing
  process -- for predefined bulk commands instead of per-breakpoint control

Anti-patterns:
  - Do NOT modify read-only properties (channelIndex, partialIndex, sampleRate,
    rootFrequency) -- changes are silently ignored (writeJSON skips them).
  - Do NOT pass a non-File object -- silently does nothing.

Source:
  ScriptLorisManager.cpp:112-133  ScriptLorisManager::processCustom()
    -> WeakCallbackHolder(processCallback, 1)
    -> LorisManager::processCustom() -> CustomPOD::toJSON() / writeJSON()
