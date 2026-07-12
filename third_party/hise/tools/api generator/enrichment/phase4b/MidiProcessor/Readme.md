MidiProcessor (object)
Obtain via: Synth.getMidiProcessor(name)

Script handle for controlling any MIDI processor module's attributes, bypass
state, and serialization. Supports bracket assignment syntax (mp["param"] = v)
via AssignableObject. Can cast to MidiPlayer for MIDI file playback modules.

Dynamic constants:
  Parameter index constants -- one per processor parameter, mapping name to
    integer index (e.g., mp.Intensity -> 0). Varies by module type.
  ScriptParameters -- nested object mapping UI component names to indices.
    Only populated for script-based MIDI processors.

Complexity tiers:
  1. Basic parameter control: setAttribute, getAttribute, setBypassed with
     dynamic constants. Control built-in modules (Transposer, Arpeggiator)
     from a UI callback.
  2. Multi-channel mute arrays: Build arrays of MidiProcessor handles in a
     loop, toggle bypass or attributes by index. Mute/solo and articulation
     switching.
  3. State serialization: + exportState, restoreState for custom preset
     management. + exportScriptControls, restoreScriptControls for script
     processor UI values without recompilation.

Practical defaults:
  - Use Synth.getMidiProcessor() in onInit and store in a const var. Never
    call it repeatedly at runtime.
  - Use dynamic constants (mp.Intensity) rather than raw indices (0) for
    setAttribute/getAttribute. They are module-specific and self-documenting.
  - When building arrays of similar processors, use a loop with string
    concatenation: Synth.getMidiProcessor("MidiMuter" + (i + 1)).
  - Use exportState/restoreState for full module snapshots. Use
    exportScriptControls/restoreScriptControls only for script processors
    when you want UI values without recompilation.

Common mistakes:
  - Reading via bracket syntax (var v = mp["Intensity"]) always returns 1.0
    due to incomplete getAssignedValue() implementation -- use getAttribute().
  - Calling exportScriptControls() on built-in MIDI modules (Transposer,
    Arpeggiator) -- throws script error. Use exportState() instead.
  - Calling Synth.getMidiProcessor() in a callback instead of onInit --
    factory is onInit-only. Cache all references at init time.
  - Using raw index numbers (mp.setAttribute(0, v)) instead of dynamic
    constants (mp.setAttribute(mp.Intensity, v)) -- fragile and unreadable.

Example:
  // Get a reference to a MIDI processor in onInit
  const var mp = Synth.getMidiProcessor("Arpeggiator1");

  // Set an attribute using the named constant
  mp.setAttribute(mp.Intensity, 0.5);

  // Or use bracket assignment syntax
  mp["Intensity"] = 0.5;

Methods (14):
  asMidiPlayer              exists
  exportScriptControls      exportState
  getAttribute              getAttributeId
  getAttributeIndex         getId
  getNumAttributes          isBypassed
  restoreScriptControls     restoreState
  setAttribute              setBypassed
