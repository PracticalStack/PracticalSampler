LorisManager::createSnapshot(ScriptObject file, String parameter, Double time) -> Array

Thread safety: UNSAFE -- allocates heap buffer (NUM_MAX_CHANNELS * 512 doubles), DLL/library call.
Creates a snapshot of a partial parameter at a specific time point. Returns a nested
array: outer array has one entry per channel, each inner array has the parameter
value for each harmonic at that time. Time is interpreted per the current timedomain
setting (default: seconds).
parameter: "rootFrequency", "frequency", "phase", "gain", or "bandwidth".

Required setup:
  const var lm = Engine.getLorisManager();
  lm.analyse(audioFile, 440.0);

Dispatch/mechanics:
  initThreadController() -> LorisManager::getSnapshot()
    -> loris_snapshot() via C API
    -> internal buffer hardcoded to 512 harmonics per channel

Pair with:
  analyse -- file must be analysed first
  set -- configure timedomain before calling if not using seconds
  createEnvelopes -- full envelope over time instead of single-point snapshot

Anti-patterns:
  - Do NOT pass a non-File object -- silently returns an empty array.
  - Sounds with more than 512 tracked partials will have higher harmonics
    silently truncated.

Source:
  ScriptLorisManager.cpp  ScriptLorisManager::createSnapshot()
    -> LorisManager::getSnapshot() -> loris_snapshot() via C API
