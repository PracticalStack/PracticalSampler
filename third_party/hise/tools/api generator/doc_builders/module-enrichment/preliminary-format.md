# Preliminary JSON Format (Steps 1-2)

**Purpose:** Create an inventory of everything extractable from `moduleList.json` plus agent judgment, and identify specific gaps requiring C++ exploration. This is a working document for the agent - it is never rendered or served to users.

**Input:** One module entry from `module_enrichment/base/moduleList.json`
**Output:** `module_enrichment/preliminary/{ModuleId}.json`
**Reference tables:** Inference tables in `module-enrichment.md`

---

## What This Format Is (And Isn't)

The preliminary JSON is a **structured analysis worksheet**. It captures:

- What we know from the base data (parameters, mod chains, I/O, interfaces)
- How parameters relate to each other (semantic grouping, composite blocks)
- What we cannot determine without reading C++ source (gaps)

It is **not** a signal-flow topology. The graph JSON (Step 3 output) and reference page (Step 4 output) will have completely different structures - a directed graph with parameters placed at their point of action, and an MDC markdown page with interactive pseudo-code. The preliminary JSON groups information by source; the downstream outputs group information by signal flow. You cannot transform one into the other mechanically.

---

## Schema

```json
{
  "moduleId": "string (from moduleList.json)",
  "prettyName": "string (display name)",
  "type": "string (Modulator|SoundGenerator|Effect|MidiProcessor)",
  "subtype": "string (EnvelopeModulator|VoiceStartModulator|...)",
  "category": "string (comma-separated categories from moduleList.json)",

  "io": {
    "midiIn": "string|null (noteOn+noteOff | noteOn | MIDI_in | null)",
    "audioIn": "string|null (stereo | mono | voice | null)",
    "audioOut": "string|null (stereo | mono | voice | null)",
    "modulationOut": "boolean (true for modulators)",
    "fxChain": "string|null (constrainer type if hasFX, else null)"
  },

  "voiceContext": "string (per-voice | monophonic)",

  "interfaces": [
    {
      "name": "string (interface name from moduleList.json)",
      "signalFlowRole": "string (node type or 'skip' with reason)"
    }
  ],

  "parameterGroups": [
    {
      "label": "string (semantic group name)",
      "rationale": "string (why these parameters belong together)",
      "parameters": [
        {
          "id": "string (parameter ID from moduleList.json)",
          "name": "string (display name)",
          "range": { "min": "number", "max": "number", "step": "number" },
          "default": "number",
          "description": "string (from moduleList.json)",
          "composite": "object|null (see composite blocks below)",
          "modChain": "object|null (linked modulation chain if any)"
        }
      ]
    }
  ],

  "standaloneModChains": [
    {
      "name": "string (chain name)",
      "chainIndex": "number",
      "modulationMode": "string (pitch|...)",
      "constrainer": "string",
      "note": "string (what this chain modulates, e.g. 'implicit base pitch')"
    }
  ],

  "disabledModChains": [
    {
      "name": "string",
      "chainIndex": "number",
      "note": "string (why disabled, or 'unknown - investigate in Step 3')"
    }
  ],

  "compositeBlocks": [
    {
      "type": "string (modMultiply | tempoSyncMux)",
      "param": "string (parameter ID)",
      "details": "object (type-specific fields, see below)"
    }
  ],

  "gaps": [
    {
      "id": "string (unique gap identifier, e.g. 'signal-path-order')",
      "category": "string (signal_path | parameter_behavior | conditional_logic | description_accuracy | interface_usage | performance)",
      "question": "string (specific question for C++ exploration)",
      "context": "string (why this matters for the reference page)"
    }
  ],

  "notes": "string (free-form observations, uncertainties, cross-references)"
}
```

### Field-by-field reference

| Field | Required | Source | Description |
|-------|----------|-------|-------------|
| `moduleId` | yes | moduleList.json | Exact module ID string |
| `prettyName` | yes | moduleList.json | Human-readable display name |
| `type` | yes | moduleList.json | Top-level processor type |
| `subtype` | yes | moduleList.json | Specific processor subtype |
| `category` | yes | moduleList.json | Category tags (comma-separated) |
| `io` | yes | Type/subtype inference table | I/O configuration inferred from type/subtype |
| `io.fxChain` | yes | moduleList.json `hasFX` + `fx_constrainer` | FX chain slot availability |
| `voiceContext` | yes | Type/subtype inference table | Per-voice or monophonic |
| `interfaces` | yes | moduleList.json | All interfaces with signal flow role assessment |
| `parameterGroups` | yes | Agent judgment + moduleList.json | Semantically grouped parameters |
| `standaloneModChains` | if any | moduleList.json (chains without `parameterIndex`) | Mod chains that don't target a named parameter |
| `disabledModChains` | if any | moduleList.json (disabled chains) | Mod chains marked as disabled |
| `compositeBlocks` | if any | Pattern matching (see below) | Identified modMultiply and tempoSyncMux patterns |
| `gaps` | yes | Agent judgment | Questions requiring C++ exploration |
| `notes` | no | Agent judgment | Free-form observations |

---

## Composite Block Details

### modMultiply

Identified when a parameter has a `chainIndex` linking it to a modulation chain.

```json
{
  "type": "modMultiply",
  "param": "Gain",
  "details": {
    "chainName": "Gain Modulation",
    "chainIndex": 1,
    "constrainer": "VoiceStartModulator|*"
  }
}
```

The constrainer type determines what modulators can be added to this chain (e.g. `VoiceStartModulator` means per-voice note-on scaling).

### tempoSyncMux

Identified when a parameter has a `tempoSyncIndex` pointing to a toggle parameter.

```json
{
  "type": "tempoSyncMux",
  "param": "DelayTimeLeft",
  "details": {
    "syncParam": "TempoSync",
    "externalInput": "HostBPM"
  }
}
```

The `HostBPM` external input is implicit - it exists whenever any `tempoSyncIndex` is present.

---

## Step 1: Create Preliminary JSON

### Inputs

- The module's entry from `module_enrichment/base/moduleList.json`
- The inference tables from `module-enrichment.md`

### Procedure

1. **Extract identity fields.** Copy `moduleId`, `prettyName` (from `name`), `type`, `subtype`, `category` directly from the base data.

2. **Infer I/O.** Use the type/subtype inference table to determine MIDI input, audio input/output, and modulation output. Check `hasFX` and `fx_constrainer` for FX chain slot.

3. **Infer voice context.** Use the type/subtype inference table to determine per-voice or monophonic.

4. **Assess interfaces.** For each interface in the base data, determine its signal flow role using the interface-to-signal-path mapping table. Mark interfaces that map to signal path elements, and mark skipped interfaces with the reason.

5. **Group parameters semantically.** Do not simply list parameters alphabetically. Group them by function:
   - Parameters that control the same processing stage belong together
   - Parameters that form an enable/value pair belong together (e.g., a toggle and the parameter it gates)
   - Tempo sync toggle parameters belong with the time parameters they control
   - Give each group a descriptive label and a one-sentence rationale

6. **Identify composite blocks.** Scan all parameters for:
   - `chainIndex` present -> modMultiply block (resolve the linked mod chain, determine constrainer)
   - `tempoSyncIndex` present -> tempoSyncMux block (resolve the sync toggle parameter, add HostBPM external input)
   - Record each composite block with full details

7. **Identify standalone modulation chains.** Find mod chains with no `parameterIndex` (typically pitch modulation chains). Record them separately with a note about what they modulate.

8. **Identify disabled modulation chains.** Find mod chains that are disabled. Record them with whatever context is available about why.

9. **Link modulation chains to parameters.** For each parameter with a `chainIndex`, include the linked mod chain details (name, constrainer) in the parameter entry.

10. **Write notes.** Record any observations, uncertainties, or cross-module relationships noticed during analysis.

### Step 1 Gate Checklist

Before proceeding to Step 2, verify:

- [ ] Every parameter from the base data appears in exactly one parameter group
- [ ] Every modulation chain is accounted for (in a composite block, as a standalone chain, or as a disabled chain)
- [ ] Every interface is listed with a signal flow role or skip reason
- [ ] I/O matches the type/subtype inference table
- [ ] All `tempoSyncIndex` cross-references are resolved to tempoSyncMux composites
- [ ] All `chainIndex` / `parameterIndex` cross-references are resolved to modMultiply composites or standalone chains
- [ ] The `hasFX` field is reflected in `io.fxChain`
- [ ] The `constrainer` field is noted (for child processor slots, relevant to container modules)
- [ ] No field from the base data entry has been ignored without justification

---

## Step 2: List Exploration Gaps

### Inputs

- The preliminary JSON from Step 1
- The module's category and type/subtype

### Procedure

Review the preliminary JSON and identify everything that cannot be determined from the base data alone. Write specific, answerable questions.

### Gap Categories

| Category | What it covers |
|----------|---------------|
| `signal_path` | Processing order, signal routing, what happens between input and output |
| `parameter_behavior` | How a parameter affects the DSP (e.g., "Does Feedback apply before or after the filter?") |
| `conditional_logic` | Mode switches, bypasses, parameter-gated paths (e.g., "What does Mode=1 vs Mode=2 change?") |
| `description_accuracy` | Base data descriptions that seem wrong, vague, or suspicious |
| `interface_usage` | How interfaces are used in the signal path (e.g., "Where does the TableProcessor lookup occur?") |
| `performance` | CPU-relevant behavior (e.g., "Is the filter per-sample or per-block?", "Does oversampling exist?") |

### Writing Good Gap Questions

**Good gaps are specific and answerable from C++ source:**
- "What is the processing order in `applyEffect()`? Specifically: does the feedback path include filtering?"
- "Parameter `Mix` has range 0-1. Does 0 mean fully dry, or is there a crossfade curve?"
- "The `EcoMode` parameter (index 10) has description 'Enables 16x downsampling'. Is this still functional or vestigial?"

**Bad gaps are vague or not answerable from source:**
- "How does this module work?" (too broad)
- "Is this module good for bass sounds?" (subjective, not in source)
- "What are typical user settings?" (requires project analysis, not C++ exploration)

### Gap ID Convention

Use descriptive kebab-case IDs: `signal-path-order`, `feedback-filter-placement`, `mode-switch-behavior`, `ecomode-vestigial`, `table-lookup-position`.

### Category-Specific Gap Patterns

Different module categories tend to have predictable gap patterns:

**Envelopes** (`generator` category, EnvelopeModulator subtype):
- Stage calculation order and curve shapes
- Retrigger behavior
- How parameters interact (e.g., does Attack time affect Hold?)

**Effects** (`delay`, `filter`, `dynamics`, `reverb` categories):
- Processing chain order within `applyEffect()`
- Feedback path topology
- Dry/wet mixing implementation
- Whether processing is per-sample or per-block

**Sound generators** (`oscillator`, `sample_playback` categories):
- Waveform generation algorithm
- Multi-oscillator mixing order
- Pitch modulation application point

**MIDI processors** (`note_processing`, `sequencing` categories):
- Event transformation logic in `processHiseEvent()`
- Which MIDI properties are read/modified
- Event generation patterns (for sequencers)

**Modulators** (`input` category):
- MIDI value source and scaling
- Voice start vs continuous behavior
- Table/curve application

### Custom Category

Modules in the `custom` category (14 total) have user-defined signal paths. For these modules, gaps should focus on:
- What callback/network slots are available
- What the framework provides (I/O routing, modulation chain hosting)
- What the user is expected to supply

Detailed signal flow analysis is deferred - the reference page shows the framework structure, not a specific implementation.

### Step 2 Gate Checklist

Before handing off to Step 3, verify:

- [ ] At least one gap exists for `signal_path` (unless the module is trivially simple - e.g., a passthrough utility)
- [ ] Every parameter group has been considered for `parameter_behavior` gaps (even if the behavior is obvious, confirm it)
- [ ] If the module has mode switches or toggle parameters, there is a `conditional_logic` gap
- [ ] If any base data description seems inaccurate or vague, there is a `description_accuracy` gap
- [ ] If the module implements `TableProcessor`, `SliderPackProcessor`, or `AudioSampleProcessor`, there is an `interface_usage` gap
- [ ] If the module is in a CPU-relevant category (filter, reverb, delay, oscillator), there is a `performance` gap
- [ ] Each gap has a unique ID, a category, a specific question, and context explaining why it matters for the reference page
- [ ] Gaps are ordered by importance (signal_path gaps first, performance gaps last)
