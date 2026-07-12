# Forum Code Example Validation — Analysis Session

You are going to analyze HiseScript forum code examples and generate validation metadata so they can be tested against the HISE runtime.

## Context

- The validation pipeline lives in `forum-search/forum_validator.py`
- Raw batch files are in `forum-search/code_examples/batch_*.json` and `snippet_batch_*.json`
- Validated output goes to `forum-search/code_examples/validated/<same filename>`
- HISE is running on `localhost:1900` with a REST API for compilation and REPL evaluation
- Before each test, the validator calls `POST /api/builder/reset` (clean module tree), then `POST /api/builder/apply` (add modules), then `POST /api/ui/apply` (add components), then `POST /api/set_script` (compile the example code as-is)

## Key Resources

- **Full module list** (79 modules): `module_enrichment/base/moduleList.json` — this is the authoritative list. The MCP `list_module_types` only returns ~38 enriched modules and is incomplete.
- **Full API surface** (2053 methods): `enrichment/base/*.json` — each file has `className` and `methods` keys. Use for verifying whether an API method exists.
- **MCP tools** (`query_scripting_api`, `list_ui_components`): Use for looking up method signatures, return types, and usage patterns.

## Your Task

Process the specified batch file. For each example:
1. Read the code and classify it into a testability tier (1-4)
2. Look up any unfamiliar HISE APIs using MCP tools or the enrichment base files
3. Generate a validation JSON object per the schema below
4. Write a JSON file keyed by example index, then apply it:
   ```
   python3 forum_validator.py --apply-metadata <batch_file> <your_metadata_file.json>
   ```
5. Run validation:
   ```
   python3 forum_validator.py --validate --batch <batch_file> --keep-alive
   ```
6. For any failures, analyze the error, fix the metadata OR the code, re-apply, and re-validate

## Validation Object Schema

```json
{
  "tier": 1-4,
  "testable": true/false,
  "skipReason": "string or null",
  "modules": [
    {"type": "SimpleGain", "name": "Simple Gain1"}
  ],
  "components": [
    {"type": "ScriptSlider", "id": "Knob1"},
    {"type": "ScriptPanel", "id": "Panel1"}
  ],
  "verifyScript": [array of checks] or null,
  "notes": "string or null"
}
```

**Important:** There is NO `setupCode` or `testOnlyCode` field. Modules and components are created via REST API before the script compiles. The example code is compiled exactly as-is.

## Tier Classification

- **Tier 1**: Self-contained. No external components or modules needed.
- **Tier 2**: Needs UI components. Code calls `Content.getComponent("Knob1")` etc. Add them to `components` array.
- **Tier 3**: Needs audio modules + possibly components. Code calls `Synth.getEffect(...)`, `Synth.getSampler(...)` etc. Add them to `modules` array.
- **Tier 4**: Truly untestable. Mark `testable: false`.

## Skip Policy — Default to Testable

**Always try to make examples testable.** Only skip for truly impossible cases:

### Legitimate skips (testable: false):
- `FileSystem.browse` / `FileSystem.browseForDirectory` — modal dialogs that block execution
- `loadImage` / `loadFontAs` with `{PROJECT_FOLDER}` paths — files don't exist in test environment
- `Server.*` API calls — needs external network connectivity
- Non-HiseScript code (C++, FAUST, template syntax)
- Code referencing undefined variables/namespaces from a larger project (e.g., `TBLLaf.registerFunction`, `ErrorHandler.errorLabel`)
- Modules that need internal state to function (e.g., `HardcodedMasterFX` with `getDisplayBufferSource` — module exists but has no display buffer without a loaded scriptnode network)

### DO NOT skip these — they compile fine:
- `UserPresetHandler.setPostCallback` — compiles, callback just won't fire. **Note:** must be called on an instance from `Engine.createUserPresetHandler()`, NOT as a static call on the namespace.
- `Engine.saveUserPreset` / `Engine.getCurrentUserPresetName` — compiles fine
- CSS styling (`setInlineStyleSheet`) — always compiles regardless of visual result
- Typed inline functions (`inline function add(a: int, b: int): int`) — valid HiseScript, including `inline function: object`
- `Synth.getSampler` / `Synth.getMidiPlayer` / `Synth.getChildSynth` — create via `modules` array with Builder API types
- LAF/paint routines — compile-only test (set `verifyScript: null`)
- MIDI callbacks (`function onNoteOn`, `function onController`) — compile-only test
- Code that writes to disk (JSON save, preset save) — acceptable in test environment

**When in doubt, mark `testable: true` with `verifyScript: null` (compile-only). Let the HISE runtime decide.**

## Handling Compile Failures

### "function not found" errors

When a compile fails with `function not found`, do NOT skip it as an environment issue. Instead:

1. Identify the method call that failed from the error callstack
2. Use MCP tool `query_scripting_api` to check if the method exists in the HISE API
3. If the method **exists**: the problem is your setup (wrong component type, missing module, wrong API usage pattern). Fix and re-test.
4. If the method **does not exist**: mark the example as **invalid** — the code references a non-existent API

Mark invalid examples with:
```json
{
  "testable": false,
  "invalid": true,
  "skipReason": "API does not exist: ClassName.methodName — not part of the HISE scripting API"
}
```

This catches forum posts that propose unimplemented features, hallucinated APIs, or code from unreleased branches. These should be **removed from the dataset**, not silently skipped.

### Fixable code errors

When an example has a small, obvious bug, **fix the code** instead of skipping. Common fixes found in previous batches:

- **Missing arguments**: e.g., `setSyncToMasterClock()` → `setSyncToMasterClock(true)`. Check the API signature via `query_scripting_api`.
- **Missing setup**: e.g., `setSyncToMasterClock(true)` requires `Engine.createTransportHandler().setEnableGrid(true, 8)` first. Check API docs for prerequisites.
- **Static vs instance calls**: e.g., `UserPresetHandler.setPostCallback(...)` should be `Engine.createUserPresetHandler().setPostCallback(...)`.
- **Incomplete snippets**: Missing closing braces for namespaces, missing function bodies. Add the minimal fix.
- **Argument mismatch**: Check the method signature and add/fix arguments.
- **Renamed/deprecated APIs**: Use `query_scripting_api` to find the current method name. Forum code may use old API names that have been renamed. Fix the code to use the current name.
- **Undefined variables**: If only 1-2 variables are missing and their purpose is obvious from context, add declarations with sensible defaults rather than skipping.
- **Callback guard clauses**: If a callback crashes because it fires with an invalid default value (e.g., ComboBox value 0 causing `array[value-1]` to access index -1), add a guard like `if (value < 1) return;`.

To fix code, modify it directly in the validated JSON file. Add a note explaining what was changed.

### Before skipping — exhaust alternatives

**Do not skip until you have verified there is no workaround.** Before marking `testable: false`:

1. **Check if the API exists under a different name.** Use `query_scripting_api` with the class overview (e.g., `MidiPlayer`) to see all available methods. Forum code often uses old method names that have been renamed in current HISE.
2. **Check if required state can be created programmatically.** Many modules have `create()` or factory methods that set up the state a callback needs. For example, `MidiPlayer.create(bars, nom, denom)` creates an empty MIDI sequence so `getEventList()` works. Don't assume "no data loaded" means untestable.
3. **Check the Builder API's full capabilities.** The `builder/apply` endpoint supports adding modules to any parent, not just Master Chain. Modulators can be added to sub-chains of sound generators by specifying `"parent"` and `"chain"` in the modules array. Read the `/api/builder/tree` endpoint to discover available chain indices on any module.
4. **Check if the error is from init vs callback.** If an error only occurs inside a callback that fires on user interaction, the example may compile fine. If a callback fires automatically on registration (like `setSequenceCallback`), you need to ensure the required state exists before registration.

### Other compile errors

- `"CSS Error: can't find CSS for component X"` — legitimate skip, CSS resolution needs specific FloatingTile content types
- `"X was not found"` for module references — check if the module type is correct, check the full module list
- `"No display buffer available"` — module exists but needs internal state (scriptnode network). Legitimate skip.

## Module Setup (modules array)

Each entry needs `type` (module type ID) and `name` (instance name the script expects).

### Chain index mapping

By default, the validator adds modules to `"Master Chain"` with the correct chain index:
- Effects → FX chain (chain **3**)
- Sound generators → children/direct chain (chain **-1**)
- MIDI processors → MIDI chain (chain **0**)

**Important:** On Master Chain, MIDI is index 0 and children/direct is index -1. This is the opposite of what you might expect.

### Adding modules to non-root parents (modulators, nested modules)

Modules can be added to any parent module, not just Master Chain. Use the `parent` and `chain` fields in the modules array entry to specify where the module should be added. The validator will split the `builder/apply` calls so parent modules are created first.

**Common modulator chain indices on sound generators:**
- **1** = Gain modulation chain
- **2** = Pitch modulation chain

To find chain indices for any module, query `GET /api/builder/tree` and look at the `chainIndex` field on each parameter.

Example — adding an AHDSR envelope to a SineSynth's gain chain:
```json
"modules": [
  {"type": "SineSynth", "name": "SineSynth1"},
  {"type": "AHDSR", "name": "AHDSR Envelope1", "parent": "SineSynth1", "chain": 1}
]
```

Modules without a `parent` field default to `"Master Chain"`. Modules without a `chain` field are auto-assigned based on their type.

### Finding module type IDs

The **authoritative module list** is in `module_enrichment/base/moduleList.json` (79 modules). Do NOT rely solely on the MCP `list_module_types` tool — it only returns ~38 enriched modules and is missing many valid types.

Common module types by category:

**Effects (chain 3):**
`SimpleGain`, `SimpleReverb`, `Delay`, `Dynamics`, `PolyphonicFilter`, `Chorus`, `Saturator`, `StereoFX`, `ShapeFX`, `Convolution`, `CurveEq`, `PhaseFX`, `PolyshapeFX`, `HarmonicFilter`, `HardcodedMasterFX`, `SendFX`, `Analyser`, `SlotFX`, `RouteFX`, `EmptyFX`

**Sound generators (chain -1):**
`SineSynth`, `StreamingSampler`, `SynthGroup`, `Noise`, `AudioLooper`, `WavetableSynth`, `WaveSynth`, `HardcodedSynth`, `GlobalModulatorContainer`, `SilentSynth`

**MIDI processors (chain 0):**
`Arpeggiator`, `Transposer`, `MidiPlayer`, `MidiMuter`, `ReleaseTrigger`, `CC2Note`, `CCSwapper`, `ChannelFilter`, `ChannelSetter`, `ChokeGroupProcessor`, `MidiMetronome`, `LegatoWithRetrigger`, `ScriptProcessor`

**Modulators (various chains):**
`LFO`, `AHDSR`, `Velocity`, `FlexAHDSR`, `Constant`, `Random`, `KeyNumber`, `PitchWheel`, `MidiController`, `TableEnvelope`, `SimpleEnvelope`, `MPEModulator`

### Script API → Module type mapping

| Script call | Module type |
|---|---|
| `Synth.getSampler("X")` | `StreamingSampler` |
| `Synth.getMidiPlayer("X")` | `MidiPlayer` |
| `Synth.getEffect("X")` | Match by name — check `moduleList.json` for the right Effect type |
| `Synth.getAudioSampleProcessor("X")` | `AudioLooper` (provides AudioFile data interface) |
| `Synth.getWavetableController("X")` | `WavetableSynth` |
| `Synth.getDisplayBufferSource("X")` | Depends — often `HardcodedMasterFX` but needs scriptnode network |
| `Synth.getChildSynth("X")` | Any SoundGenerator type |
| `Synth.getSliderPackProcessor("X")` | Any ExternalDataHolder with SliderPack data (e.g., `Arpeggiator`) |
| `Synth.getTableProcessor("X")` | Any ExternalDataHolder with Table data |
| `Synth.getModulator("X")` | Any Modulator type — add to a sound generator's modulation chain via `parent` + `chain` fields |

### Common effect name → type ID mapping

| Script name pattern | Module type |
|---|---|
| `"Simple Gain*"` | `SimpleGain` |
| `"Parametriq EQ*"` | `CurveEq` (NOT "ParametriqEQ" — that type doesn't exist) |
| `"Convolution*"` | `Convolution` |
| `"Delay*"` | `Delay` |
| `"Compressor*"`, `"Dynamics*"` | `Dynamics` |
| `"Filter*"` | `PolyphonicFilter` |
| `"Reverb*"` | `SimpleReverb` |
| `"Chorus*"` | `Chorus` |

Example:
```json
"modules": [
  {"type": "SimpleGain", "name": "Simple Gain1"},
  {"type": "StreamingSampler", "name": "Sampler1"},
  {"type": "AudioLooper", "name": "AudioReference"},
  {"type": "WavetableSynth", "name": "Wavetable Synthesiser1"},
  {"type": "MidiPlayer", "name": "MIDI Player1"}
]
```

## Component Setup (components array)

Each entry needs `type` (full component type name) and `id` (component ID the script expects).

Component type mapping:
| Script pattern | Component type |
|---|---|
| `Knob*`, `knb*`, `Slider*` | `ScriptSlider` |
| `Button*`, `btn*` | `ScriptButton` |
| `Panel*`, `pnl*` | `ScriptPanel` |
| `Label*`, `lbl*` | `ScriptLabel` |
| `ComboBox*`, `cmb*` | `ScriptComboBox` |
| `FloatingTile*`, `ft_*` | `ScriptFloatingTile` |
| `Table*` | `ScriptTable` |
| `SliderPack*` | `ScriptSliderPack` |
| `Viewport*` | `ScriptedViewport` |
| `AudioWaveform*` | `ScriptAudioWaveform` |
| `Image*` | `ScriptImage` |
| `WebView*` | `ScriptWebView` |

Also infer from method usage:
- `.setPaintRoutine` → `ScriptPanel`
- `.addItem` → `ScriptComboBox`
- `.setRange` → `ScriptSlider`

Default to `ScriptSlider` if ambiguous.

**Don't add components the example creates itself** via `Content.addKnob`, `Content.addPanel`, etc.

Example:
```json
"components": [
  {"type": "ScriptSlider", "id": "Knob1"},
  {"type": "ScriptPanel", "id": "Panel1"},
  {"type": "ScriptFloatingTile", "id": "FloatingTile1"}
]
```

## Verification

### REPL checks
```json
{"type": "REPL", "expression": "variableName", "value": expectedValue, "delay": 0}
```
- `expression`: any valid HiseScript expression accessible in onInit scope
- `value`: expected result (number, string, bool, or "undefined")
- `delay`: ms to wait before checking (use 100-300 for async, 0 for sync)
- Variables declared with `const var` in onInit are accessible via REPL
- `reg` variables are also accessible

### Callback triggers via set_component_value
```json
{"type": "set_value", "component": "Knob1", "value": 0.5, "delay": 0}
```
Uses `POST /api/set_component_value` — directly triggers the control callback. Place this BEFORE the REPL checks that verify the callback's side effects.

Example — testing a callback that maps knob value to effect attribute:
```json
"verifyScript": [
  {"type": "REPL", "expression": "TheGains.length", "value": 2, "delay": 0},
  {"type": "set_value", "component": "Knob1", "value": 0.5, "delay": 0},
  {"type": "REPL", "expression": "TheGains[0].getAttribute(0)", "value": 0.5, "delay": 200}
]
```
The **200ms delay** after `set_value` is essential — it gives the callback time to fire.

### Log output verification
```json
{"type": "log-output", "values": ["expected", "console", "lines"]}
```
Matches `Console.print` output during compilation, in order.

### Error expectation
```json
{"type": "expect-error", "errorMessage": "substring match"}
```

## What NOT to verify via REPL

- LAF/paint routines — compile-only, set `verifyScript: null`
- MIDI callbacks (`function onNoteOn`, `function onController`) — can't trigger via API
- Mouse/drag events — can't simulate
- Values depending on unpredictable runtime state (`Math.random`, system time)
- Default knob range is 0.0-1.0 — integer array indexing won't work without reconfiguring the range

## Batch Processing

Work in chunks of ~15 examples. For each chunk:
1. Read the examples from the batch file
2. Generate a metadata JSON (keyed by index as strings: `{"0": {...}, "1": {...}}`)
3. Write to `code_examples/validated/<batch>_metadata_<start>_<end>.json`
4. Apply: `python3 forum_validator.py --apply-metadata <batch_file> <metadata_file>`
5. Validate: `python3 forum_validator.py --validate --batch <batch_file> --keep-alive`
6. Check failures, fix metadata OR code, re-apply, re-validate
7. Repeat until all examples pass or are correctly skipped

After all examples are done, run coverage:
```
python3 forum_validator.py --coverage --batch <batch_file>
```

## Target Metrics

- Skip rate below 20% (be aggressive about marking things testable)
- All testable examples should pass (fix metadata on failures, don't just skip them)
- REPL verification where observable state exists (not just compile-only)
- Zero false skips — every skip must be justified by a genuinely untestable condition
- Invalid examples must be flagged with `"invalid": true`, not silently skipped
