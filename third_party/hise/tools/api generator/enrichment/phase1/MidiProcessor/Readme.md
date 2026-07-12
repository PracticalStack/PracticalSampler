# MidiProcessor -- Class Analysis

## Brief
Script handle for controlling any MIDI processor module's attributes, bypass state, and serialization.

## Purpose
The `MidiProcessor` object is a script handle to any MIDI processor module in the signal chain. It provides attribute get/set by index or name, bypass control, full state export/restore as base64 strings, and the ability to cast to a `MidiPlayer` reference for MIDI file playback modules. It also supports bracket-based assignment syntax (`mp["paramName"] = value`) via the `AssignableObject` interface, and exposes a `ScriptParameters` object for accessing UI control indices on script-based MIDI processors.

## Details

### AssignableObject Interface

MidiProcessor implements `AssignableObject`, enabling bracket-based parameter access:

```javascript
// These are equivalent:
mp.setAttribute(mp.Intensity, 0.5);
mp["Intensity"] = 0.5;
```

**Important limitation:** Reading via bracket syntax (`var v = mp["Intensity"]`) always returns `1.0` due to an incomplete implementation of `getAssignedValue()`. Use `getAttribute()` for reading values.

### Dynamic Constants

Constants are registered dynamically at construction time based on the actual processor instance:

- **Parameter index constants:** Each parameter identifier (e.g., `Intensity`, `Speed`) maps to its integer index. Use these as named constants: `mp.setAttribute(mp.Intensity, 0.5)`.
- **ScriptParameters:** A nested object mapping UI component names to indices, only populated when the underlying module is a script processor (`ProcessorWithScriptingContent`). Access as `mp.ScriptParameters.KnobName`.

### Script-Only Methods

`exportScriptControls()` and `restoreScriptControls()` only work when the underlying MIDI processor is a script processor (JavascriptMidiProcessor). Calling them on built-in MIDI modules (Transposer, Arpeggiator, etc.) produces a script error. These methods save/restore only UI control values without triggering recompilation -- useful for preset-like state management across script processors.

### State Serialization

`exportState()` / `restoreState()` serialize the full processor state (all parameters, internal state) as base64. `exportScriptControls()` / `restoreScriptControls()` serialize only the scripting content (UI control values). The distinction matters for script processors where you want to restore knob positions without recompiling the script.

### Module Type Casting

`asMidiPlayer()` performs a runtime type check. It succeeds only if the underlying module is a `MidiPlayer`, returning a `MidiPlayer` handle with full playback control. Otherwise it throws a script error. The reverse operation (`MidiPlayer.asMidiProcessor()`) is also available.

## obtainedVia
`Synth.getMidiProcessor(name)` -- retrieves a MIDI processor by ID from the parent synth's subtree. Must be called in `onInit`. Cannot reference the calling script processor itself.

Also: `Builder.create()` (programmatic module construction), `MidiPlayer.asMidiProcessor()` (reverse cast from MidiPlayer handle).

## minimalObjectToken
mp

## Constants
None. All constants are dynamic -- registered at construction time from the specific processor instance's parameters.

## Dynamic Constants
| Name | Type | Description |
|------|------|-------------|
| *(parameter identifiers)* | int | One constant per processor parameter, mapping the parameter name to its integer index. Varies by module type. |
| ScriptParameters | Object | Nested object mapping UI component names to indices. Only populated for script-based MIDI processors. |

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| `var v = mp["Intensity"];` | `var v = mp.getAttribute(mp.Intensity);` | Bracket read always returns 1.0 due to incomplete getAssignedValue() implementation. Use getAttribute() instead. |
| `mp.exportScriptControls()` on a Transposer | `mp.exportState()` on a Transposer | exportScriptControls only works on script processors. Use exportState for built-in MIDI modules. |

## codeExample
```javascript
// Get a reference to a MIDI processor in onInit
const var mp = Synth.getMidiProcessor("Arpeggiator1");

// Set an attribute using the named constant
mp.setAttribute(mp.Intensity, 0.5);

// Or use bracket assignment syntax
mp["Intensity"] = 0.5;
```

## Alternatives
- `MidiPlayer` -- extended handle with MIDI file playback, editing, and visualization. Use `asMidiPlayer()` to cast.
- `Effect` -- same handle pattern but for audio effect modules instead of MIDI processors.

## Related Preprocessors
None.

## Diagnostic Ideas
Reviewed: Yes
Count: 0
Rationale: All methods are simple delegations with runtime validity checks. No timeline dependencies, no silent-failure preconditions, no mode-selector constants that could produce surprising behavior at parse time.
