Builder::get(Integer buildIndex, String interfaceType) -> ScriptObject

Thread safety: UNSAFE -- allocates a new scripting wrapper object on the heap.
Returns a typed scripting wrapper for the module at the given build index. The
interfaceType must match one of the InterfaceTypes constant values. The module
must also be dynamically castable to the corresponding C++ type. Returns
undefined on failure.

Required setup:
  const var b = Synth.createBuilder();
  var synthIdx = b.create(b.SoundGenerators.SineSynth, "MySine", 0, b.ChainIndexes.Direct);
  b.flush();

Dispatch/mechanics:
  createdModules[buildIndex] -> match interfaceType string against class names
    -> dynamic_cast to corresponding C++ type -> construct scripting wrapper

Pair with:
  create -- create the module first, then get a typed reference
  getExisting -- register a pre-existing module, then get a typed reference

Anti-patterns:
  - Returns undefined silently if buildIndex is invalid or interfaceType does
    not match the module's actual type. No error or warning is produced.
    Always verify the type matches (e.g., use InterfaceTypes.Effect for effects).

Source:
  ScriptingApiObjects.cpp  ScriptBuilder::get()
    -> RETURN_IF_MATCH macro: className match + dynamic_cast -> new wrapper
