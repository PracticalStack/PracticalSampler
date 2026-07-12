LorisManager::process(ScriptObject file, String command, JSON data) -> undefined

Thread safety: UNSAFE -- JSON serialization/parsing, DLL/library call, heap allocations during partial list manipulation.
Processes the analysed partial list using a predefined command. Modifies the cached
partial list in place. Use process(file, "reset", {}) to revert to original state.
Commands: "reset" ({}), "shiftTime" ({"offset": n}), "shiftPitch" ({"offset": n}
or [[time,val],...]), "scaleFrequency" ([[time,val],...]), "dilate"
([[inputTimes],[targetTimes]]), "applyFilter" ([[freq,val],...]).

Required setup:
  const var lm = Engine.getLorisManager();
  lm.analyse(audioFile, 440.0);

Dispatch/mechanics:
  initThreadController() -> LorisManager::process(file, command, jsonString)
    -> loris_process() via C API
    -> MultichannelPartialList::process() dispatches by command string
    -> "shiftPitch" supports constant offset or envelope; "applyFilter" temporarily
       switches to frequency time domain internally

Pair with:
  analyse -- file must be analysed first
  synthesise -- resynthesise after processing
  processCustom -- for per-breakpoint control instead of bulk commands

Anti-patterns:
  - Do NOT pass plain values as data -- must be JSON object or array.
    e.g. process(f, "shiftPitch", 100) is wrong; use {"offset": 100}.
  - Do NOT pass a non-File object -- silently does nothing.

Source:
  ScriptLorisManager.cpp  ScriptLorisManager::process()
    -> LorisManager::process() -> loris_process() via C API
    -> MultichannelPartialList::process() (MultichannelPartialList.cpp:173-310)
