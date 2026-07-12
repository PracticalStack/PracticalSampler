# Scriptnode Node Reference Enrichment Pipeline

**Purpose:** Transform 194 scriptnode nodes from basic auto-generated metadata into rich MDC reference pages with interactive pseudo-code, parameter tables, usage guidance, and LLM-optimised reference text. Consumed by the MCP server and Nuxt.js docs site.

**Base data:** `scriptnode_enrichment/base/scriptnodeList.json`
**Existing docs:** `scriptnode_enrichment/phase3/` (varying quality - read-only reference)
**Output location:** `scriptnode_enrichment/output/{factory}/{node}.md` (one MDC page per node)
**Infrastructure docs:** `scriptnode_enrichment/resources/infrastructure/` (produced by Step 0)
**Sub-step details:**
- `scriptnode-enrichment/preliminary-format.md` - Steps 1-2: preliminary JSON + gap listing
- `scriptnode-enrichment/exploration-guide.md` - Step 3: C++ source exploration + graph JSON
- `scriptnode-enrichment/reference-page-format.md` - Step 4: MDC reference page authoring

---

## Strategic Context: Nodes vs. Modules vs. Scripting API

The scriptnode enrichment pipeline runs parallel to the module enrichment pipeline (`module-enrichment.md`) and the scripting API enrichment pipeline (`scripting-api-enrichment.md`). All three share the same output format conventions (MDC markdown, canonical links, general style guide) but address fundamentally different documentation surfaces.

| Dimension | Scripting API | Audio Modules | Scriptnode Nodes |
|-----------|---------------|---------------|------------------|
| Unit count | ~60 classes | 79 modules | 194 nodes |
| Unit complexity | High (10-50 methods per class) | Medium (multi-stage signal paths) | Low (1-6 parameters, single operation) |
| C++ source per unit | 100-500+ lines per method | 100-500+ lines per module | 30-100 lines per node |
| Source of truth | Method body + Doxygen | `processBlock` + module metadata | `process`/`processFrame` + node metadata |
| Shape of knowledge | Parameter types, return values, thread safety | Signal flow topology, parameter interactions | Single DSP operation, parameter behaviour |
| Visual output | None (text only) | Interactive pseudo-code | Interactive pseudo-code (simpler) |
| Existing docs | Bare Doxygen signatures | ~5 good, ~10 medium, ~50+ stub | ~30 good, ~57 brief, ~100 stub |
| Variant groups | None | Minimal | Significant (pack2-8, oversample2-16x, etc.) |
| Agent judgment | Low (mechanical extraction) | High (topology authoring) | Medium (most are straightforward) |

### Why a Separate Pipeline

Scriptnode nodes cannot reuse the module pipeline because:

1. **No modulation chains.** Modules have built-in modulation chains with constrainer types. Nodes use external cable connections - the modulation system is orthogonal to the node itself.
2. **No type/subtype inference.** Modules are categorised by Modulator/Effect/SoundGenerator/MidiProcessor with predictable I/O. Nodes use `cppProperties` flags (IsPolyphonic, OutsideSignalPath, IsControlNode) which require different inference rules.
3. **Variant groups.** Many nodes are parameterised variants of a single template (pack2-8_writer, fix8-256_block, oversample2-16x, softbypass_switch2-8, freq_split2-5). The pipeline must handle these efficiently.
4. **Simpler signal paths.** Most nodes perform a single operation. The graph JSON and pseudo-code are simpler than module signal paths.

---

## Pipeline Overview

```
Step 0A: Infrastructure Exploration (one-time)
  -> scriptnode_enrichment/resources/infrastructure/*.md
  -> scriptnode_enrichment/issues.md (sidecar)

Step 0B: Project Usage Survey (one-time)
  -> scriptnode_enrichment/resources/usage_survey.md

Per-session (operator specifies scope: factory, node subset, or single node):

  [Step 1-2] Agent creates preliminary JSON per node
        |    - Parameter grouping, cppProperties inference, gap questions
        |    - Gate: All base JSON information used?
        |
  [Step 3]   Agent explores C++ source per node
        |    - Answers gap questions from process()/processFrame()
        |    - Writes exploration markdown + graph JSON
        |    - Flags issues in issues.md
        |    - Gate: All gaps answered? All issues flagged?
        |
  [Step 4]   Agent authors MDC reference page per node
        |    - Interactive pseudo-code from graph JSON
        |    - Parameter table, prose, seeAlso, llmRef
        |    - Gate: All parameters accounted for? No C++ leakage?
        v
  scriptnode_enrichment/output/{factory}/{node}.md
```

### Step summary

| Step | Agent type | Input | Output | Guide |
|------|-----------|-------|--------|-------|
| 0A | Explorer | C++ infrastructure source files | `resources/infrastructure/*.md` | This file (Infrastructure section) |
| 0B | General | DspNetwork XML files from projects/ | `resources/usage_survey.md` | This file (Usage Survey section) |
| 1-2 | General | scriptnodeList.json entry + infrastructure docs | `scriptnode_enrichment/preliminary/{factory}.{node}.json` | `preliminary-format.md` |
| 3 | Explorer | Preliminary JSON + C++ node source + infrastructure docs | `scriptnode_enrichment/exploration/{factory}.{node}.md` + `.json` | `exploration-guide.md` |
| 4 | General | Graph JSON + exploration + scriptnodeList.json | `scriptnode_enrichment/output/{factory}/{node}.md` | `reference-page-format.md` |

---

## Directory Structure

```
tools/api generator/
  doc_builders/
    scriptnode-enrichment.md                    # This file (pipeline orchestrator)
    scriptnode-enrichment/
      preliminary-format.md                     # Steps 1-2 guide + schema
      exploration-guide.md                      # Step 3 guide + graph JSON schema
      reference-page-format.md                  # Step 4 guide (MDC authoring)

  scriptnode_enrichment/
    base/
      scriptnodeList.json                       # Auto-generated node metadata (read-only)
    phase3/                                     # Existing docs (read-only reference)
      {factory}/
        {node}.md
        Readme.md
    resources/
      images/                                   # Screenshots (41 files)
      infrastructure/                           # Step 0A output
        core.md                                 # Tier 1: universal concepts
        wrap-templates.md                       # Tier 2: wrap:: namespace
        containers.md                           # Tier 2: container_base
        control-infrastructure.md               # Tier 2: cable bases + dynamic parameters
        modulation.md                           # Tier 2: modulation:: namespace
        hmath.md                                # Tier 2: math library
        index-types.md                          # Tier 2: index:: namespace
        hise-event.md                           # Tier 2: HiseEvent
        tempo.md                                # Tier 2: TempoListener
        runtime-target.md                       # Tier 2: runtime_target + ModulationNodes
        global-routing.md                       # Tier 2: GlobalRoutingManager
        duplicate.md                            # Tier 2: clone system
        opaque-node.md                          # Tier 2: OpaqueNode
        interpreted-wrappers.md                 # Tier 2: InterpretedNode et al.
        valuetree-builder.md                    # Tier 2: C++ export
        snex-overview.md                        # Tier 2: SNEX JIT overview
        bypass.md                               # Tier 2: bypass wrappers
      usage_survey.md                           # Step 0B output
    preliminary/                                # Steps 1-2 output
      {factory}.{node}.json                     # One per node
    exploration/                                # Step 3 output
      {factory}.{node}.md                       # Exploration findings
      {factory}.{node}.json                     # Graph JSON topology
    output/                                     # Step 4 output (final)
      {factory}/
        Readme.md                               # Factory-level overview page
        {node}.md                               # Per-node MDC reference page
    issues.md                                   # Sidecar: bugs found during exploration
```

---

## Base Data: scriptnodeList.json

The base data is auto-generated by instantiating every node and serialising its state. Each node entry contains:

| Field | Type | Description |
|-------|------|-------------|
| `ID` | string | Node ID within its factory (e.g., `"bitcrush"`) |
| `FactoryPath` | string | Full path `factory.node` (e.g., `"fx.bitcrush"`) |
| `Parameters` | object | Parameter definitions with min/max/step/skew/default/TextToValueConverter |
| `Properties` | object | Runtime-configurable properties (Mode, Code, NumParameters, etc.) |
| `description` | string | Short description from `SN_DESCRIPTION` macro |
| `cppProperties` | object | Compile-time/runtime flags (see below) |
| `ModulationTargets` | object | Modulation output configuration |
| `SwitchTargets` | object | Switch target configuration (for branch nodes) |
| `ComplexData` | object | External data slots (Tables, SliderPacks, AudioFiles) |

### cppProperties Flags

These are registered via `CustomNodeProperties` during node initialisation and define the node's runtime and C++ export behaviour:

| Flag | Meaning |
|------|---------|
| `IsPolyphonic` | Node maintains per-voice state (uses `PolyData`) |
| `OutsideSignalPath` | Node does not process audio (control/cable node) |
| `IsControlNode` | Node is a control signal source with modulation output |
| `UseUnnormalisedModulation` | Modulation output sends raw parameter values, not 0-1 normalised |
| `IsProcessingHiseEvent` | Node processes MIDI events (has `handleHiseEvent` callback) |
| `HasModeTemplateArgument` | Node has a mode property that maps to a C++ template argument |
| `IsCloneCableNode` | Node is designed for use with container.clone |

### Node counts by factory

| Factory | Count | C++ Source |
|---------|-------|-----------|
| analyse | 4 | `dsp_nodes/AnalyserNodes.h` |
| container | 28 | `node_api/nodes/Container_*.h` + `processors.h` |
| control | 47 | `dsp_nodes/CableNodes.h` + `CableNodeBaseClasses.h` |
| core | 25 | `dsp_nodes/CoreNodes.h` |
| dynamics | 5 | `dsp_nodes/DynamicsNode.h` |
| envelope | 7 | `dsp_nodes/EnvelopeNodes.h` |
| filters | 10 | `dsp_nodes/FilterNode.h` |
| fx | 6 | `dsp_nodes/FXNodes.h` |
| jdsp | 7 | `dsp_nodes/JuceNodes.h` |
| math | 26 | `dsp_nodes/MathNodes.h` |
| routing | 14 | `dsp_nodes/RoutingNodes.h` |
| template | 15 | Composite (wrappers around other nodes) |
| **Total** | **194** | |

### Variant Groups

These nodes are parameterised variants of a single C++ template. The enrichment agent reads the template/base class once and writes individual files per variant, noting only variant-specific differences.

| Group | Variants | Template parameter |
|-------|----------|-------------------|
| `control.packN_writer` | pack2 through pack8 | Number of Value parameters (2-8) |
| `container.fixN_block` | fix8, fix16, fix32, fix64, fix128, fix256 | Block size |
| `container.oversampleNx` | oversample2x, 4x, 8x, 16x | Oversampling factor |
| `template.softbypass_switchN` | switch2 through switch8 | Number of switch targets |
| `template.freq_splitN` | split2 through split5 | Number of frequency bands |

---

## Inference Tables

These tables define what can be mechanically inferred from scriptnodeList.json fields without C++ exploration. Steps 1-2 use these tables to build the preliminary JSON.

### cppProperties Inference Table

| cppProperties combination | Signal path role | Audio I/O | Voice context |
|---------------------------|-----------------|-----------|---------------|
| `OutsideSignalPath: true, IsControlNode: true` | Control signal source | None | Depends on IsPolyphonic |
| `OutsideSignalPath: true, IsControlNode: false` | Utility (no audio, no mod output) | None | Monophonic |
| `OutsideSignalPath: false, IsPolyphonic: true` | Polyphonic audio processor | Audio in/out | Per-voice |
| `OutsideSignalPath: false, IsPolyphonic: false` | Monophonic audio processor | Audio in/out | Monophonic |
| `IsProcessingHiseEvent: true` | Also processes MIDI events | (as above) | (as above) |
| `UseUnnormalisedModulation: true` | Modulation output sends raw values | (as above) | (as above) |

### Factory-to-Role Mapping

| Factory | Typical role | Notes |
|---------|-------------|-------|
| `analyse` | Display/analysis (no audio modification) | All have display buffers |
| `container` | Signal flow structure | Containers don't process audio themselves |
| `control` | Control signal routing/transformation | All are OutsideSignalPath |
| `core` | Core DSP (oscillators, gain, delays, file players) | Mixed polyphonic/monophonic |
| `dynamics` | Dynamic range processing | Compressors, envelope followers |
| `envelope` | Envelope generators + voice management | All polyphonic, have modulation outputs |
| `filters` | Filter processing | Various topologies |
| `fx` | Audio effects | Bitcrush, reverb, phase delay, etc. |
| `jdsp` | JUCE DSP wrappers | Delay, chorus, compressor, panner, filter |
| `math` | Mathematical operations on audio signals | Simple transforms |
| `routing` | Signal routing (send/receive, matrix, global cables) | Mixed audio/control |
| `template` | Pre-built composite nodes | Wrappers around other nodes |

### ComplexData Inference

| ComplexData type | Signal path role |
|-----------------|-----------------|
| `Tables` | Lookup table for waveshaping or value mapping |
| `SliderPacks` | Multi-value data source (step sequencer, multi-band, etc.) |
| `AudioFiles` | Audio file playback or analysis source |

---

## Step 0A: Infrastructure Exploration

**One-time, reusable.** Run before any per-node enrichment. Produces distilled reference documents from C++ source that enrichment agents load as context.

### Tier 1: Core (always loaded)

**Output:** `scriptnode_enrichment/resources/infrastructure/core.md`

**Source files to explore:**

| Class/System | Source file | Lines |
|-------------|-----------|-------|
| `ProcessData<C>`, `ProcessDataDyn` | `hi_dsp_library/snex_basics/snex_ProcessDataTypes.h` | 660 |
| `FrameProcessor` | `hi_dsp_library/snex_basics/snex_FrameProcessor.h` | 246 |
| `span<T,Size>`, `dyn<T>` | `hi_dsp_library/snex_basics/snex_ArrayTypes.h` | 865 |
| `PrepareSpecs`, `PolyData<T,N>` | `hi_dsp_library/snex_basics/snex_Types.h` | 1276 |
| `polyphonic_base`, `HiseDspBase`, `SingleWrapper<T>` | `hi_dsp_library/node_api/nodes/Base.h` | 98 |
| `ExternalData`, `data::base`, `filter_base`, `display_buffer_base` | `hi_dsp_library/snex_basics/snex_ExternalData.h` | 1309 |
| `mothernode` | `hi_dsp_library/node_api/nodes/OpaqueNode.h` | extract ~30 lines |
| Node macros (`SN_NODE_ID`, `DEFINE_PARAMETERS`, etc.) | `hi_dsp_library/node_api/helpers/node_macros.h` | 206 |
| `CustomNodeProperties` system | `hi_dsp_library/node_api/helpers/node_ids.h` | extract ~200 lines |
| `ParameterData`, `parameter::dynamic` | `hi_dsp_library/node_api/helpers/ParameterData.h` | 582 |
| `NodeProperty`, `NodePropertyT<T>` | `hi_dsp_library/node_api/helpers/NodeProperty.h` | 263 |

**Distillation target:** ~500-600 lines covering:
- How audio data flows through nodes (ProcessData, FrameProcessor, span/dyn)
- How polyphonic state works (PolyData, polyphonic_base)
- What prepare() receives (PrepareSpecs)
- How nodes access external data (ExternalData, data::base hierarchy)
- The node class hierarchy (mothernode, HiseDspBase, polyphonic_base)
- How parameters are defined and connected (ParameterData, node macros)
- What cppProperties mean (CustomNodeProperties registry)
- How runtime properties work (NodeProperty system)

### Tier 2: Domain-Specific (loaded selectively)

Each document is loaded only when enriching nodes that depend on the covered concepts.

| Document | Source files | Lines | Loaded for |
|----------|------------|-------|------------|
| `wrap-templates.md` | `node_api/nodes/processors.h` | 1877 | container.* factory, understanding wrapper chains |
| `containers.md` | `node_api/nodes/Containers.h` + `Container_Chain.h` + `Container_Split.h` + `Container_Multi.h` + `container_base.h` | ~805 | container.* factory |
| `control-infrastructure.md` | `dsp_nodes/CableNodeBaseClasses.h` (190) + `DynamicProperty.h` (235) + `DynamicParameterList.h` (278) + parameter templates from `parameter.h` (~300) | ~1000 | control.* factory, any modulation source |
| `modulation.md` | `node_api/helpers/modulation.h` | 448 | Modulation source nodes |
| `hmath.md` | `snex_basics/snex_Math.h` | 332 | math.* factory |
| `index-types.md` | `snex_basics/snex_IndexTypes.h` + `snex_IndexLogic.h` | 892 | Nodes with indexed table/pack access |
| `hise-event.md` | `hi_tools/hi_tools/HiseEventBuffer.h` | 721 | MIDI-processing nodes (IsProcessingHiseEvent) |
| `tempo.md` | `hi_tools/hi_tools/MiscToolClasses.h` (extract TempoListener ~100 lines) | ~100 | control.tempo_sync, core.clock_ramp, control.ppq |
| `runtime-target.md` | `hi_tools/hi_tools/runtime_target.h` (259) + `dsp_nodes/ModulationNodes.h` base templates (~200) | ~460 | core.global_mod, extra_mod, hise_mod, envelope.*_mod_gate |
| `global-routing.md` | `hi_scripting/.../GlobalRoutingManager.h` | 465 | routing.global_cable, global_send/receive |
| `duplicate.md` | `node_api/nodes/duplicate.h` | 647 | container.clone, control.clone_cable/forward/pack |
| `opaque-node.md` | `node_api/nodes/OpaqueNode.h` | 608 | Runtime node instantiation |
| `interpreted-wrappers.md` | `hi_scripting/.../StaticNodeWrappers.h` | 925 | Interpreter-to-compiled boundary |
| `valuetree-builder.md` | `hi_snex/snex_cpp_builder/snex_jit_ValueTreeBuilder.h` | 917 | C++ export implications |
| `snex-overview.md` | `hi_snex/` (170k LOC - overview only) | distill ~400 | core.snex_node, snex_shaper, snex_osc, math.expr, control.cable_expr |
| `bypass.md` | `node_api/nodes/Bypass.h` | 388 | container.soft_bypass, bypass behaviour |

### Infrastructure Loading Rules

When enriching a set of nodes, the agent loads:

1. **Always:** `core.md`
2. **Per factory:**
   - `container.*` -> also load `wrap-templates.md`, `containers.md`, `bypass.md`
   - `control.*` -> also load `control-infrastructure.md`, `modulation.md`
   - `math.*` -> also load `hmath.md`
   - `envelope.*` -> also load `modulation.md`
   - `routing.*` -> also load `global-routing.md` (for global_cable/send/receive)
   - `template.*` -> also load `wrap-templates.md`, `containers.md`
3. **Per node flags:**
   - `IsProcessingHiseEvent: true` -> also load `hise-event.md`
   - Node uses `runtime_target` (global_mod, extra_mod, hise_mod, *_mod_gate) -> also load `runtime-target.md`
   - Node uses indexed data (table, cable_pack, cable_table) -> also load `index-types.md`
   - Node is tempo-related (tempo_sync, clock_ramp, ppq) -> also load `tempo.md`
   - Node is clone-related (clone, clone_cable, clone_forward, clone_pack) -> also load `duplicate.md`
   - Node is SNEX-based (snex_node, snex_shaper, snex_osc, expr, cable_expr) -> also load `snex-overview.md`

### Exploration Agent Instructions

Spawn one `explore-hise-cpp` subagent per infrastructure document. The agent reads the source files, distills the concepts relevant to scriptnode node documentation, and writes the output.

**Tier 1 prompt template:**

```
Explore and distill the core scriptnode infrastructure for the node enrichment pipeline.

Read these source files:
  {list of source files for Tier 1}

Write: scriptnode_enrichment/resources/infrastructure/core.md

Distill ~500-600 lines covering:
  - How audio data flows through nodes (ProcessData, FrameProcessor, span/dyn)
  - How polyphonic state works (PolyData, polyphonic_base)
  - What prepare() receives (PrepareSpecs)
  - How nodes access external data (ExternalData, data::base hierarchy)
  - The node class hierarchy (mothernode, HiseDspBase, polyphonic_base)
  - How parameters are defined and connected (ParameterData, node macros)
  - What cppProperties mean (CustomNodeProperties registry)
  - How runtime properties work (NodeProperty system)

Focus on what a documentation writer needs to understand about each concept
to accurately describe any scriptnode node. Do not document implementation
details that do not affect user-visible behaviour.

If you discover bugs or non-obvious behaviours, append them to:
  scriptnode_enrichment/issues.md
Do NOT include bugs in the distilled reference.

ASCII-only output.
```

**Tier 2 prompt template:**

```
Explore and distill {concept} for the scriptnode node enrichment pipeline.

Read: {source file(s)}
Write: scriptnode_enrichment/resources/infrastructure/{document}.md

Distill the concepts relevant to documenting scriptnode nodes that use {concept}.
Focus on: {specific focus areas for this document}

{any additional context specific to this document}

If you discover bugs or non-obvious behaviours, append them to:
  scriptnode_enrichment/issues.md

ASCII-only output.
```

---

## Step 0B: Project Usage Survey

**One-time, reusable.** Scans DspNetwork XML files from the project collection to produce usage statistics and common patterns.

**Input:** 92 DspNetwork XML files across 6 projects (hise_tutorial, dm_piano, Collab3, Magic7, PercX, Triaz)

**Output:** `scriptnode_enrichment/resources/usage_survey.md`

### Survey Contents

1. **Node usage frequency** - how many times each node appears across all networks
2. **Per-factory usage summary** - which factories are most/least used
3. **Common node combinations** - which nodes frequently appear together in the same network
4. **Notable parameter configurations** - real-world parameter values from actual networks
5. **Unused nodes** - nodes that appear in scriptnodeList.json but never in any project (81 of 194)

### Survey Agent Prompt

```
Scan all DspNetwork XML files in the project collection and produce a usage survey.

Find all XML files matching: projects/*/DspNetworks/Networks/*.xml
                        and: projects/hise_tutorial/*/DspNetworks/Networks/*.xml

For each file, extract all FactoryPath attributes from Node elements.

Write: scriptnode_enrichment/resources/usage_survey.md

Include:
1. Node usage frequency table (sorted by count, descending)
2. Per-factory usage summary (total instances, unique nodes used)
3. Top 20 node co-occurrence pairs (nodes that appear in the same network)
4. Notable parameter configurations (non-default parameter values for frequently used nodes)
5. List of unused nodes (present in scriptnodeList.json but absent from all projects)

This survey is used by Step 4 agents to write better "when to use" guidance.
```

---

## Steps 1-4: Per-Node Enrichment

### Invocation

The operator specifies the enrichment scope per session:

```
# Full factory
"Enrich all nodes from the filters factory"

# Subset of nodes
"Enrich the control nodes pma, pma_unscaled, tempo_sync"

# Single node
"Enrich core.oscillator"
```

The agent resolves the scope to a list of `FactoryPath` values, then processes each node through Steps 1-4.

### Batching Strategy

Process nodes in related groups for cross-reference quality:

| Batch | Nodes | Rationale |
|-------|-------|-----------|
| **Filters** (pilot) | svf, svf_eq, biquad, one_pole, moog, ladder, linkwitzriley, allpass, convolution, ring_mod | Shared filter_base, good for format validation |
| **Math operators** | add, sub, mul, div, clip, abs, clear, fill1, tanh, sin, sqrt, square, rect, pow, fmod | Simple nodes, fast batch, test variant handling |
| **Control (value processing)** | pma, pma_unscaled, minmax, bipolar, intensity, normaliser, converter, change, compare, logic_op | Core control signal manipulation |
| **Control (routing)** | bang, blend, input_toggle, branch_cable, xfader, cable_expr, cable_table, cable_pack | Signal routing/switching |
| **Control (clone)** | clone_cable, clone_forward, clone_pack | Clone-specific control |
| **Control (pack writers)** | pack2_writer through pack8_writer, pack_resizer | Variant group |
| **Routing** | send, receive, global_cable, global_send, global_receive, matrix, selector, ms_encode, ms_decode, public_mod, event_data_reader, event_data_writer, local_cable, local_cable_unscaled | Signal routing |
| **Core (generators)** | oscillator, phasor, phasor_fm, fm, ramp, clock_ramp | Tone/signal generators |
| **Core (processors)** | gain, smoother, fix_delay, table, peak, peak_unscaled, mono2stereo, recorder | Audio utilities |
| **Core (external)** | file_player, stretch_player, granulator, snex_node, snex_shaper, snex_osc, jit, faust | External/complex nodes |
| **Core (modulators)** | extra_mod, global_mod, hise_mod, pitch_mod, matrix_mod | HISE modulation bridges |
| **Envelopes** | ahdsr, flex_ahdsr, simple_ar, ramp_envelope, silent_killer, voice_manager, global_mod_gate, extra_mod_gate | Voice lifecycle |
| **Containers** | chain, split, multi, branch, clone, midichain, modchain, sidechain, no_midi, soft_bypass, repitch, offline | Signal flow structure |
| **Containers (block)** | fix8-256_block, frame1_block, frame2_block, framex_block, dynamic_blocksize | Block processing variants |
| **Containers (oversample)** | oversample, oversample2-16x | Oversampling variants |
| **FX** | bitcrush, reverb, phase_delay, sampleandhold, haas, pitch_shift | Audio effects |
| **Dynamics** | comp, limiter, gate, envelope_follower, updown_comp | Dynamic range |
| **JDSP** | jdelay, jdelay_cubic, jdelay_thiran, jchorus, jcompressor, jlinkwitzriley, jpanner | JUCE DSP wrappers |
| **Analyse** | fft, oscilloscope, goniometer, specs | Display/analysis |
| **Templates** | dry_wet, mid_side, bipolar_mod, feedback_delay, freq_split2-5, softbypass_switch2-8 | Composite nodes |
| **Math (advanced)** | expr, map, table, pack, sig2mod, mod2sig, mod_inv, inv, neural, pi | Complex math nodes |

Start with the **Filters** pilot batch. Filter nodes share `filter_base`, have consistent parameter patterns (Frequency, Q, Mode), and produce structurally similar reference pages - good for validating the output format.

---

## Gate Conditions Overview

| Step | Gate | Key check |
|------|------|-----------|
| 1 | Completeness | All base JSON information used for this node? |
| 2 | Specificity | Gap questions specific enough for targeted C++ exploration? |
| 3 | Coverage | All gaps answered? All issues flagged? Graph JSON complete? |
| 4 | Accountability | All parameters in table? All exploration answers incorporated? No C++ leakage? |

Full gate checklists are in each step's sub-document.

---

## Issues Sidecar

All pipelines share the same issue tracking pattern. Bugs found during exploration go to `scriptnode_enrichment/issues.md`, never into user-facing documentation.

### Issue Format

```markdown
### {factory}.{node} -- {short description}

- **Type:** {silent-fail | missing-validation | inconsistency | vestigial | ux-issue}
- **Severity:** {critical | high | medium | low}
- **Location:** {file path}:{line number}
- **Observed:** {what you found}
- **Expected:** {what should happen}
```

### Bug Discovery Policy

Issues found during exploration must NOT appear in any documentation output (description, commonMistakes, llmRef, warnings). Users should not see implementation bugs in their documentation. Bugs are transient; documentation is long-lived. Fix the bugs instead.

The exploration markdown may mention vestigial or non-functional features as factual observations (e.g., "Mode parameter accepts value 3 but it behaves identically to value 2"). This tells Step 4 to note the limitation. Do not include line numbers, fix suggestions, or bug analysis in exploration notes - put those in issues.md only.

---

## Cross-Cutting Output Fields

The reference page (Step 4) is fully specified in `reference-page-format.md`. This section documents the cross-cutting frontmatter fields.

### seeAlso

Each entry: `{id, type, reason}`. Relationship types:

| Type | Meaning | Example |
|------|---------|---------|
| `alternative` | Different node, similar purpose | svf vs biquad |
| `companion` | Commonly used together | send + receive |
| `upgrade` | Newer/better version | (rare for nodes) |
| `disambiguation` | Easily confused | pma vs pma_unscaled |
| `module` | Equivalent HISE audio module | filters.svf vs PolyphonicFilter |
| `api` | Related scripting API | routing.global_cable vs GlobalCable |

Use `$SN.factory.node$`, `$MODULES.ModuleId$`, and `$API.ClassName$` canonical link tokens.

### cpuProfile

| Field | Type | Description |
|-------|------|-------------|
| `baseline` | enum | negligible, low, medium, high, very_high |
| `polyphonic` | boolean | Per-voice (true) or per-buffer (false) |
| `scalingFactors` | array | Parameters that increase CPU beyond baseline |

### commonMistakes (0-3 per node)

Each entry: `{wrong, right, explanation}`. Sources: C++ exploration, existing phase3 docs, DSP domain knowledge. Explanations reference observable behaviour, never implementation details. Most simple nodes (math operators) will have 0 entries.

### llmRef

Pre-synthesised text blob for the MCP server. Fixed section order:

```
factory.node_name

[1-2 sentence overview]

Signal flow:
  [arrow notation from pseudo-code, or "Control node - no audio processing"]

CPU: [baseline], [polyphonic/monophonic]

Parameters:
  [grouped, with practical notes and defaults]

When to use:
  [practical guidance, alternatives]

Common mistakes:
  [if any]

See also:
  [type] factory.node -- reason
```

---

## Feedback Loop Prevention

- Steps 1-2 use ONLY scriptnodeList.json, the inference tables, and infrastructure docs. Never reference exploration output or reference pages.
- Step 3 uses ONLY the preliminary JSON, C++ source, and infrastructure docs. Never reference other nodes' exploration markdown (cross-contamination). Exception: variant group base exploration may be referenced by variant members.
- Step 4 uses the graph JSON, exploration markdown, scriptnodeList.json, existing phase3 docs, and usage survey. Never reference other nodes' reference pages (except within a batch for seeAlso).

---

## Session Launcher Templates

### Steps 1-2: Preliminary JSON

```
Process nodes {list of FactoryPath values} through Steps 1-2.

Read the guide: doc_builders/scriptnode-enrichment/preliminary-format.md
Read the base data from: scriptnode_enrichment/base/scriptnodeList.json
Read the inference tables in: doc_builders/scriptnode-enrichment.md
Read the infrastructure: scriptnode_enrichment/resources/infrastructure/core.md
{Load additional Tier 2 infrastructure docs per the loading rules}

Output: scriptnode_enrichment/preliminary/{factory}.{node}.json (one per node)

Follow the gate checklists.
```

### Step 3: C++ Exploration

```
Explore the C++ source for nodes {list of FactoryPath values}.

Read the guide: doc_builders/scriptnode-enrichment/exploration-guide.md
Read the preliminary JSON files from: scriptnode_enrichment/preliminary/
Read the infrastructure: scriptnode_enrichment/resources/infrastructure/core.md
{Load additional Tier 2 infrastructure docs per the loading rules}

C++ source for this factory: {C++ header file path}

Output:
  scriptnode_enrichment/exploration/{factory}.{node}.md (one per node)
  scriptnode_enrichment/exploration/{factory}.{node}.json (one per node)
Issues: append to scriptnode_enrichment/issues.md

Follow the gate checklist. Answer ALL gap questions.
```

### Step 4: MDC Reference Page

```
Author MDC reference pages for nodes {list of FactoryPath values}.

Read the guide: doc_builders/scriptnode-enrichment/reference-page-format.md
Read the graph JSON from: scriptnode_enrichment/exploration/
Read the exploration markdown from: scriptnode_enrichment/exploration/
Read the base data from: scriptnode_enrichment/base/scriptnodeList.json
Read the existing docs from: scriptnode_enrichment/phase3/{factory}/
Read the usage survey: scriptnode_enrichment/resources/usage_survey.md

Output:
  scriptnode_enrichment/output/{factory}/{node}.md (one per node)
  scriptnode_enrichment/output/{factory}/Readme.md (factory overview, if not yet written)

Follow the gate checklist. No C++ leakage.
```

---

## Deferred Work

### User-Facing Glossary

The infrastructure exploration (Step 0A) produces internal reference documents. A separate post-processing step could transform these into a user-facing glossary explaining scriptnode architectural concepts (processing model, polyphony, modulation connections, block processing, C++ export). Deferred until the core pipeline is validated and the quality of infrastructure docs can be assessed.

### C++ Node Authoring Guide

The infrastructure exploration covers the same API that custom C++ nodes implement (ProcessData, prepare(), setParameter(), handleHiseEvent(), etc.). A "How to write custom C++ scriptnode nodes" guide could be derived from the infrastructure docs as a byproduct. Deferred until after the node reference is complete.

### Project Example Extraction (Phase 2 equivalent)

The scripting API pipeline has a Phase 2 that extracts real-world code examples from analysed projects. For scriptnode, the equivalent would be extracting notable DspNetwork configurations that demonstrate specific nodes in context. The usage survey (Step 0B) provides the raw data. Deferred until the core pipeline is validated.
