# Module Enrichment Pipeline

**Purpose:** Transform 79 HISE audio processor modules from basic C++-extracted metadata into rich documentation with interactive pseudo-code reference pages, performance models, usage guidance, and LLM-optimized reference text. Consumed by the MCP server and Nuxt.js docs site.

**Base data:** `module_enrichment/base/moduleList.json`
**Output location:** `module_enrichment/pages/` (one MDC markdown file per module)
**Sub-phase details:**
- `module-enrichment/preliminary-format.md` - Steps 1-2: preliminary JSON + gap listing
- `module-enrichment/exploration-guide.md` - Step 3: C++ source exploration + graph JSON
- `module-enrichment/reference-page-format.md` - Step 4: MDC reference page authoring
- `module-enrichment/reference-page-renderer.md` - Nuxt.js Vue component spec

---

## Strategic Context: Modules vs. Scripting API

The module enrichment pipeline runs parallel to the scripting API enrichment pipeline (`scripting-api-enrichment.md`) but addresses a fundamentally different documentation surface.

| Dimension | Scripting API | Modules |
|-----------|---------------|---------|
| Unit of work | Method on a class | Audio processor |
| Source of truth | C++ method body + Doxygen | C++ `processBlock` + module metadata |
| Shape of knowledge | Parameter types, return values, thread safety | Signal flow topology, parameter interactions, CPU cost |
| Visual output | None (text only) | Interactive pseudo-code with glossary highlighting |
| User question | "What does this method do?" | "How does this module process audio?" |
| Agent judgment | Low (mostly mechanical extraction) | High (topology authoring, parameter placement) |

### Why Multiple Intermediate Formats

A single enrichment pass cannot produce the final reference page because the preliminary data (parameters, mod chains, I/O) maps to a fundamentally different structure than the graph topology or the authored pseudo-code. The preliminary JSON is an inventory organised by data source. The graph JSON is a topology organised by signal flow. The MDC reference page is a human-readable document with interactive pseudo-code derived from the graph. Each step transforms the structure, not just fills in gaps.

---

## Pipeline Overview

```
moduleList.json (base data, 79 modules)
        |
  [Step 1] Agent creates preliminary JSON
        |    - Parameter grouping, composite blocks, I/O inference
        |    - Uses inference tables (type/subtype, category, interface)
        |    - Gate: Have I used ALL base JSON information?
        |
  [Step 2] Agent lists exploration gaps
        |    - Structured questions for C++ exploration
        |    - Written into preliminary JSON's gaps field
        |    - Gate: Are gaps specific enough?
        |
  [Step 2b] Forum gap enrichment (optional)
        |    - /update-forum --modules {ModuleId}
        |    - Extracts user confusion points from forum posts
        |    - New gaps merged into exploration input
        |    - Also produces forum insights for Step 4
        |    - Output: forum/gaps/{ModuleId}.json, forum/insights/{ModuleId}.json
        |
  [Step 3] Agent explores C++ source
        |    - Answers gap questions from processBlock/renderNextBlock
        |    - Writes exploration markdown + graph JSON topology
        |    - Flags base JSON issues in issues.md
        |    - Gate: ALL gaps answered? ALL issues flagged?
        |
  [Step 4] Agent authors MDC reference page
        |    - Interactive pseudo-code derived from graph JSON
        |    - Parameter tables, modulation tables, prose, notes
        |    - Incorporates forum insights (Warning/Tip blocks) if available
        |    - See Also with scriptnode equivalents, UI components
        |    - Gate: ALL parameters accounted for?
        |           ALL exploration answers incorporated?
        |           No C++ leakage in prose?
        v
  module_enrichment/pages/{ModuleId}.md

  [Step 4b] Forum page triage (backport-only, for already-enriched modules)
        |    - Reads existing pages/{ModuleId}.md + forum/gaps/ + forum/insights/
        |    - Triages: what's already covered vs what needs additions
        |    - For unanswered VERIFY=yes insights: uses verify-forum-claim
        |    - Outputs targeted edit instructions (no full rewrite)
        v
  module_enrichment/pages/{ModuleId}.md (updated)
```

### Step summary

| Step | Agent type | Input | Output | Guide |
|------|-----------|-------|--------|-------|
| 1-2 | General | moduleList.json entry | `module_enrichment/preliminary/{ModuleId}.json` | `preliminary-format.md` |
| 2b | /update-forum | Module name | `module_enrichment/forum/gaps/{ModuleId}.json` + `forum/insights/{ModuleId}.json` | `forum-insights-guide.md` |
| 3 | Explorer | Preliminary JSON + forum gaps + C++ source | `module_enrichment/exploration/{ModuleId}.md` + `.json` | `exploration-guide.md` |
| 4 | General | Graph JSON + exploration markdown + forum insights + moduleList.json | `module_enrichment/pages/{ModuleId}.md` | `reference-page-format.md` |
| 4b | Sonnet triage | Existing page + forum gaps + forum insights | Targeted edits to `pages/{ModuleId}.md` | `forum-insights-guide.md` |

---

## Directory Structure

```
tools/api generator/
  doc_builders/
    module-enrichment.md                      # This file (pipeline orchestrator)
    module-enrichment/
      preliminary-format.md                   # Steps 1-2 guide + schema
      exploration-guide.md                    # Step 3 guide + graph JSON schema
      reference-page-format.md                # Step 4 guide (MDC authoring)
      reference-page-renderer.md              # Nuxt.js Vue component spec

  module_enrichment/
    base/
      moduleList.json                         # Phase 0 output: 79 modules, read-only
    preliminary/                              # Step 1-2 output
      {ModuleId}.json                         # One per module
    forum/                                    # Step 2b output
      gaps/
        {ModuleId}.json                       # Forum-derived gap questions
      insights/
        {ModuleId}.json                       # Forum-derived Warning/Tip candidates
    exploration/                              # Step 3 output
      {ModuleId}.md                           # Exploration findings
      {ModuleId}.json                         # Graph JSON topology
    pages/                                    # Step 4 output (final)
      {ModuleId}.md                           # MDC markdown reference page
    resources/
      reference/                              # Design reference HTML prototypes
        ahdsr-demo.html
        delay-demo.html
        macromodulator-demo.html
        channelfilter-demo.html
        pseudocode-demo.html
    issues.md                                 # Sidecar: bugs found during exploration
```

---

## Base Data: moduleList.json

The base data is extracted mechanically from C++ source code by the Phase 0 tool. Each module entry contains:

| Field | Type | Description |
|-------|------|-------------|
| `id` | string | Internal module ID (e.g., `"AHDSR"`, `"Delay"`) |
| `name` | string | Display name |
| `type` | string | Top-level type: `Modulator`, `SoundGenerator`, `Effect`, `MidiProcessor` |
| `subtype` | string | Specific subtype (see inference table below) |
| `category` | string | Comma-separated category tags |
| `description` | string | Short description from C++ metadata |
| `parameters` | array | Parameter definitions with id, name, range, default, description |
| `modulationChains` | array | Modulation chain definitions with cross-references |
| `interfaces` | array | Implemented processor interfaces |
| `hasChildren` | boolean | Whether the module hosts child processors |
| `hasFX` | boolean | Whether the module has an FX chain slot |
| `constrainer` | string | What types of child processors are allowed |
| `fx_constrainer` | string | What effects can go in the FX chain |

Each parameter may have:
- `chainIndex` linking it to a modulation chain
- `tempoSyncIndex` linking it to a tempo sync toggle parameter

Each modulation chain may have:
- `parameterIndex` linking it back to the parameter it modulates
- `constrainer` restricting what modulator types can be added

### Module counts

| Type/Subtype | Count |
|-------------|-------|
| Effect/MasterEffect | 18 |
| SoundGenerator/SoundGenerator | 14 |
| MidiProcessor/MidiProcessor | 12 |
| Modulator/EnvelopeModulator | 11 |
| Modulator/VoiceStartModulator | 9 |
| Modulator/TimeVariantModulator | 7 |
| Effect/VoiceEffect | 7 |
| Effect/MonophonicEffect | 1 |
| **Total** | **79** |

Of the 79 modules, 14 are in the `custom` category (user-defined signal paths). These get a brief structural reference page showing the callback/network slots rather than a fixed signal flow. Detailed treatment is deferred.

---

## Inference Tables

These tables define what can be mechanically inferred from moduleList.json fields without C++ exploration. Steps 1-2 use these tables to build the preliminary JSON.

### Type/Subtype Inference Table

| type/subtype | MIDI in | Voice context | Output | Audio in |
|---|---|---|---|---|
| Modulator/EnvelopeModulator | noteOn + noteOff | per-voice | modulation | none |
| Modulator/VoiceStartModulator | noteOn | per-voice | modulation | none |
| Modulator/TimeVariantModulator | none | monophonic | modulation | none |
| SoundGenerator/SoundGenerator | noteOn + noteOff | per-voice | audio L/R | none |
| Effect/MasterEffect | none | monophonic | audio L/R | audio L/R |
| Effect/VoiceEffect | none | per-voice | audio (voice) | audio (voice) |
| Effect/MonophonicEffect | none | monophonic | audio | audio |
| MidiProcessor/MidiProcessor | MIDI in | monophonic | MIDI out | none |

### Category Inference Table

| Category | Inference for reference page |
|----------|----------------------|
| `input` | Module reads from a specific MIDI value source (velocity, CC, key, pitch wheel) |
| `note_processing` | MIDI event transformation (MIDI in -> transform -> MIDI out) |
| `sequencing` | Temporal/pattern behavior, may generate MIDI events |
| `container` + `hasChildren: true` | Child module slots with constrainer-defined types |
| `delay` | Delay-line topology with potential feedback paths |
| `dynamics` | Gain/compression processing chain |
| `filter` | Filter topology with frequency/resonance modulation points |
| `reverb` | Reverb algorithm structure |
| `routing` | Signal routing emphasis (send/receive, channel mapping) |
| `oscillator` | Periodic waveform output (also applies to modulators like LFO) |
| `sample_playback` | Sample streaming, looping, pitch shifting |
| `mixing` | Signal combination (usually simple, may have mid/side) |
| `utility` | Mostly passthrough or non-audio (often no signal path section needed) |
| `custom` | Show callbacks/network slots as nodes (framework, not fixed signal path) |
| `generator` | Signal generation, envelope stages |

### Interface-to-Signal-Path Mapping

Interfaces declared in moduleList.json map to specific elements in the signal path pseudo-code and graph JSON.

| Interface | Signal path role |
|-----------|-----------------|
| `TableProcessor` | Lookup table step in signal path |
| `SliderPackProcessor` | Slider pack / step sequencer data source |
| `AudioSampleProcessor` | Audio sample resource |
| `SlotFX` | Hot-swappable effect slot |
| `DisplayBufferSource` | UI communication point (not in signal path) |
| `RoutingMatrix` | Skip - inferred from type/subtype |
| `Sampler` | Skip - single-consumer, redundant with module identity |
| `WavetableController` | Skip - single-consumer, redundant |
| `MidiPlayer` | Skip - single-consumer, redundant |

### Composite Block Patterns

Two reusable sub-graph patterns appear across multiple modules. The agent identifies these in Step 1 and carries them through to the graph JSON and reference page. Full schema and examples are in `preliminary-format.md`.

- **modMultiply** - Parameter modulated by a modulation chain. Trigger: parameter has a `chainIndex`. Pseudo-code pattern: `value = Param * ModChainName`
- **tempoSyncMux** - Time parameter with tempo sync option. Trigger: parameter has a `tempoSyncIndex`. Pseudo-code pattern: `time = TempoSync ? tempoToMs(HostBPM) : Param`. Only 3 parameters use this pattern (Delay: DelayTimeLeft, DelayTimeRight; LFO: Frequency).

### Standalone Modulation Chains

All modulation chains with `modulationMode: "pitch"` (15 total across all modules) have no `parameterIndex` - they modulate an implicit base pitch rather than a named parameter. These are represented in the pseudo-code as standalone modulation inputs feeding into the voice's pitch calculation, not as modMultiply composites.

Similarly, `Gain Modulation` chains at `chainIndex:1` with `parameterIndex:0` follow a base-class pattern on nearly all SoundGenerators. These are standard modMultiply blocks.

Four modulation chains across all modules are disabled. These should be omitted from the pseudo-code and parameter table, or noted briefly in the Notes section, based on agent judgment.

---

## Gate Conditions Overview

Every step ends with a gate checklist that must be satisfied before proceeding. Gates prevent incomplete work from cascading into downstream steps.

| Step | Gate | Key check |
|------|------|-----------|
| 1 | Completeness | Have I used ALL information available in the base JSON entry? |
| 2 | Specificity | Are gap questions specific enough for targeted C++ exploration? |
| 3 | Coverage | Have I answered ALL gap questions? Flagged ALL description issues? Graph JSON complete? |
| 4 | Accountability | Are ALL parameters in the table? ALL exploration answers incorporated? No C++ leakage? At least 2 See Also entries? Glossary consistent with pseudo-code? |

Full gate checklists are in each step's sub-document.

---

## Batching Strategy

Process modules in seeAlso clusters so cross-module relationships can be identified during production:

| Batch | Modules | Rationale |
|-------|---------|-----------|
| Envelopes (pilot) | AHDSR, FlexAHDSR, SimpleEnvelope, TableEnvelope | Nearly identical topologies, good for format validation |
| Time modulators | LFO, ConstantModulator, RandomModulator, ArrayModulator | Monophonic signal generators, varying complexity |
| Voice start | Velocity, KeyNumber, PitchWheel, Aftertouch | MIDI-to-modulation converters, simple graphs |
| Global routing | GlobalModulatorContainer, GlobalVoiceStart*, GlobalTimeVariant*, GlobalEnvelope* | Producer/consumer chain, tests external_input nodes |
| Delays | Delay, Chorus, PhaseFX | Shared delay-line topology with variations |
| Filters | PolyphonicFilter, CurveEq, HarmonicFilter | Different filter topologies |
| Dynamics | Dynamics, SimpleGain, RouteFX, SendFX | Gain/routing effects |
| Samplers | StreamingSampler, Looper | Complex shared_resource + per_voice interaction |
| Containers | SynthChain, SynthGroup, GlobalModulatorContainer | Structural modules, test hasChildren |
| MIDI processors | Arpeggiator, TransposerMidiProcessor, ChannelFilter, ChannelRouter, ReleaseTrigger | Event transformation patterns |
| Custom | ScriptSynth, ScriptFX, Hardcoded* | Dynamic modules - show framework slots, defer internals |

Start with the **Envelopes** pilot batch. AHDSR already has exploration notes and HTML prototypes as design reference, providing immediate format validation. The four envelope modules should produce structurally similar reference pages, validating cross-module consistency.

### Which modules need signal flow exploration

Not all 79 modules need C++ exploration. Based on category:

| Category | C++ exploration? | Rationale |
|----------|-----------------|-----------|
| `oscillator`, `generator` | yes | Synthesis algorithm, waveform generation |
| `sample_playback` | yes | Playback engine, streaming, looping |
| `filter`, `delay`, `reverb` | yes | DSP topology, feedback paths, modulation points |
| `dynamics` | yes | Processing chain, stage order, sidechain paths |
| `input` | yes | Event-to-signal conversion logic |
| `note_processing`, `sequencing` | yes | Event transformation, timing logic |
| `container` | optional | Voice mixing, FM routing (if applicable) |
| `routing`, `mixing` | optional | Usually trivial; include for mid/side or complex routing |
| `utility` | no | Mostly passthrough or non-audio |
| `custom` | no | User-defined signal path depends on loaded network |

---

## Cross-Cutting Output Fields

The reference page (produced in Step 4) is fully specified in `reference-page-format.md`. This section documents the cross-cutting fields that appear in the MDC frontmatter for consumption by the MCP server, search, and sidebar display.

### seeAlso (2-8 entries per module)

Each entry: `{id, type, reason}` in the frontmatter `seeAlso` array. Relationship types:

| Type | Meaning | Reciprocal | Primary source |
|------|---------|-----------|---------------|
| `alternative` | Different module, similar purpose | `alternative` (symmetric) | Step 3 (similar topology) |
| `source` | Produces signal/data this module needs | `target` | Step 3 (external_input nodes) |
| `target` | Consumes signal/data this module produces | `source` | Step 3 (external_input nodes) |
| `companion` | Commonly used together, no signal dependency | `companion` (symmetric) | Deferred (Phase 2a) |
| `ui_component` | FloatingTile or editor tied to this module | (none) | Step 3 (C++ `createEditor()`) |
| `upgrade` | Replaces or extends this module's functionality | (none, directional) | Step 3 + documentation |
| `disambiguation` | Easily confused with this module | `disambiguation` (symmetric) | Documentation + judgment |
| `scriptnode` | Equivalent scriptnode node wrapping the same DSP | `scriptnode` (symmetric) | C++ factory registry |

**Source/target examples:**

| Source module | Target module | What flows |
|---------------|---------------|-----------|
| GlobalModulatorContainer | GlobalVoiceStart/TimeVariant/EnvelopeModulator | Modulation values |
| MacroModulationSource | MacroModulator | Macro control values |
| SendFX | SendContainer | Audio signal (send bus) |

**UI component discovery:** Inspect `createEditor()` for FloatingTile content types (e.g., `AhdsrEnvelopePanel`, `SampleMapEditor`, `WaveformDisplay`).

**Scriptnode equivalents:** Known pairs include FlexAHDSR / `envelope.flex_ahdsr`, SimpleReverb / `fx.reverb`, PhaseFX / `fx.phase_delay`, Chorus / `fx.chorus`, PolyphonicFilter / `filters.*`.

### cpuProfile

CPU performance model stored in the MDC frontmatter. Hardware-independent, relative to "a simple gain multiply."

**Base cost tiers:**

| Tier | Examples |
|------|----------|
| `negligible` | Parameter read, bypass check, simple decision |
| `low` | Gain multiply, simple mix, table lookup |
| `medium` | Biquad filter, delay line read/write, envelope calculation |
| `high` | Multi-mode filter, FFT-based processing, per-sample nonlinearity |
| `very_high` | Convolution, large FFT, heavy oversampling |

**Module-level** (in MDC frontmatter):

| Field | Type | Description |
|-------|------|-------------|
| `baseline` | enum | Overall CPU cost at default settings (same tier enum) |
| `polyphonic` | boolean | Per-voice (true) or per-buffer (false) |
| `scalingFactors` | array | Parameters that significantly increase CPU beyond baseline. Each entry: `{parameter, impact, note}` |

**Derivation:**
- `baseline`: determined from the exploration findings (Step 3 CPU Assessment section)
- `polyphonic`: from the module's subtype
- `scalingFactors`: parameters that significantly increase CPU beyond baseline, identified during exploration

### commonMistakes (0-5 entries per module)

Each entry: `{wrong, right, explanation}`.

Sources: documentation warnings, C++ gotchas from Step 3, general audio anti-patterns. Explanations must reference observable behavior ("causes clicks", "wastes CPU"), never implementation details. Skip modules where no meaningful mistakes exist.

### customEquivalent

Guides users to rebuild stock module behavior using custom modules.

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `approach` | enum | yes | `hisescript`, `scriptnode`, or `snex` |
| `moduleType` | string | yes | Custom module type to host the reproduction |
| `complexity` | enum | yes | `trivial`, `simple`, `medium`, `complex` |
| `description` | string | yes | One sentence explaining the approach |
| `snippetId` | string | no | ID into the snippet database |

| Approach | When to use | Host module types |
|----------|-------------|-------------------|
| `hisescript` | Simple callback logic, event processing, table lookups | ScriptVoiceStartModulator, ScriptTimeVariantModulator, ScriptEnvelopeModulator, ScriptProcessor |
| `scriptnode` | DSP chains, effects with routing, node graph flexibility | HardcodedFX, HardcodedSynth, HardcodedTimeVariantModulator, etc. |
| `snex` | Per-sample math, waveshaping formulas, custom filter topologies | Any Hardcoded* module with a SNEX node |

Skip for: `custom` category (already custom), containers, routing, utility.

### llmRef (multi-paragraph reference)

Pre-synthesised text blob served verbatim by the MCP server. Stored in the MDC frontmatter. Fixed section order:

```
ModuleName (type/subtype)

[1-2 sentence overview]

Signal flow:
  [arrow notation derived from the pseudo-code]

CPU: [baseline tier], [polyphonic/monophonic]
  [scaling factors if any]

Parameters:
  [grouped by function, with practical notes]
  [include default values and typical ranges]

When to use:
  [practical guidance]

Common mistakes:
  [summary of commonMistakes entries]

Custom equivalent:
  [approach] via [moduleType]: [description]

See also:
  [type] ModuleName - reason
```

---

## Complexity Budget

The graph JSON (Step 3 output) includes an `importance` value (0.0-1.0) on each node, controlling how much detail appears in the Step 4 pseudo-code. High-importance nodes (>= 0.7) become code statements; medium (0.3-0.7) may become comments; low (< 0.3) are typically omitted. See `exploration-guide.md` for the full importance table and assignment guidelines.

---

## Session Launcher Templates

Each step is launched as an agent session. The sub-documents are self-contained instruction manuals. These templates are the launcher prompts.

### Steps 1-2: Preliminary JSON

```
Process module "{ModuleId}" through Steps 1-2 of the module enrichment pipeline.

Read the guide: doc_builders/module-enrichment/preliminary-format.md
Read the base data entry for {ModuleId} from: module_enrichment/base/moduleList.json
Read the inference tables in: doc_builders/module-enrichment.md (Inference Tables section)

Output: module_enrichment/preliminary/{ModuleId}.json

Follow the gate checklists at the end of each step.
```

### Step 2b: Forum Gap Enrichment (optional)

```
/update-forum --modules {ModuleId}
```

This invokes the `/update-forum` agent which:
1. Generates search queries for the module
2. Searches the HISE forum and fetches relevant topics
3. Extracts gap questions to `module_enrichment/forum/gaps/{ModuleId}.json`
4. Extracts insight candidates to `module_enrichment/forum/insights/{ModuleId}.json`

For unenriched modules, new gaps (those with `maps_to_existing_gap: null`) are added
to the preliminary JSON's gaps array before Step 3. Forum insights with `verify: true`
are verified during the Step 3 C++ exploration (no separate verification needed).

### Step 3: C++ Exploration

```
Explore the C++ source for module "{ModuleId}".

Read the guide: doc_builders/module-enrichment/exploration-guide.md
Read the preliminary JSON: module_enrichment/preliminary/{ModuleId}.json
Explore the C++ source at the repository root.

Output:
  module_enrichment/exploration/{ModuleId}.md (exploration findings)
  module_enrichment/exploration/{ModuleId}.json (graph JSON topology)
Issues: append to module_enrichment/issues.md

Follow the gate checklist. Answer ALL gap questions from the preliminary JSON.
```

### Step 4: MDC Reference Page

```
Author the MDC reference page for module "{ModuleId}".

Read the guide: doc_builders/module-enrichment/reference-page-format.md
Read the graph JSON: module_enrichment/exploration/{ModuleId}.json
Read the exploration markdown: module_enrichment/exploration/{ModuleId}.md
Read the base data for metadata resolution: module_enrichment/base/moduleList.json
Review the design reference prototypes: module_enrichment/resources/reference/

Output: module_enrichment/pages/{ModuleId}.md

Follow the gate checklist. Account for ALL parameters. No C++ leakage.
```

### Step 4b: Forum Page Triage (backport-only)

```
Triage forum content for already-enriched module "{ModuleId}".

Read the existing page: module_enrichment/pages/{ModuleId}.md
Read forum gaps: module_enrichment/forum/gaps/{ModuleId}.json
Read forum insights: module_enrichment/forum/insights/{ModuleId}.json
Read exploration output: module_enrichment/exploration/{ModuleId}.md

For each forum gap:
  - Check if the existing page already answers it. If yes, mark as "covered".
  - If not, check if the exploration output answers it. If yes, note the answer.
  - If neither covers it, flag for targeted C++ verification using verify-forum-claim.

For each forum insight:
  - Check if the existing page already covers it. If yes, mark as "covered".
  - If VERIFY=yes and not covered, verify via verify-forum-claim or exploration output.
  - If verified, draft a Warning/Tip block with placement recommendation.

Output: targeted edit instructions for the existing page (not a full rewrite).
```

Use this step only for modules that already have a completed reference page.
Do NOT use this for modules going through the normal Steps 1-4 pipeline.

### Batch processing

For a batch, process all modules in the batch through Steps 1-2 together, then Step 3 together, then Step 4 together. This ensures cross-module consistency within the seeAlso cluster.

---

## Feedback Loop Prevention

- Steps 1-2 use ONLY moduleList.json and the inference tables in this document. Never reference exploration output or reference pages.
- Step 3 uses ONLY the preliminary JSON and C++ source code. Never reference previous exploration markdown for other modules (cross-contamination).
- Step 4 uses the graph JSON, exploration markdown, and moduleList.json. Never reference other modules' reference pages (except within a batch for seeAlso cross-referencing).

---

## Deferred Work

### Phase 2a: Real-World Project Analysis

Scanning 14 HISE projects for parameter histograms, co-occurrence matrices, and common configurations. This would feed `companion` seeAlso entries and "typical ranges" in llmRef. Not yet started; deferred until the core pipeline (Steps 1-4) is validated on the pilot batch.

### Phase 2b: Documentation Salvage

Extracting prose from the `hise_documentation` GitHub repo. Coverage is sparse (only ~5 modules have substantial docs, ~10 medium, ~50+ frontmatter only). Deferred until core pipeline validation.

---

## Open Questions

1. **Importance calibration:** Should importance values be assigned during Step 4 (agent judgment) or derived mechanically from the graph topology (e.g., nodes on the shortest path from input to output get higher importance)?

2. **seeAlso batch consistency:** When processing a batch, should the agent produce seeAlso entries for all modules in the batch simultaneously, or produce them per-module and reconcile at the end?

3. **Custom category depth:** The 14 custom modules show callback/network slots as nodes. How much internal structure should be shown? Just the slot types, or also the available callback signatures?

4. **cpuProfile validation:** The hardware-independent tier system is inherently subjective. Should we validate tiers against actual profiling data, or accept expert judgment?
