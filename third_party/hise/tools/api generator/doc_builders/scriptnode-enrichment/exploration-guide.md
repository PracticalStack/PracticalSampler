# C++ Exploration Guide (Step 3)

**Purpose:** Answer the gap questions from the preliminary JSON by reading the node's C++ source code. Produce a structured exploration markdown and a graph JSON topology that Step 4 will consume to author the MDC reference page. Flag any base data inaccuracies as sidecar issues.

**Input:** `scriptnode_enrichment/preliminary/{factory}.{node}.json`
**Output:**
- `scriptnode_enrichment/exploration/{factory}.{node}.md` (exploration findings)
- `scriptnode_enrichment/exploration/{factory}.{node}.json` (graph topology)
**Issues:** Append to `scriptnode_enrichment/issues.md`
**Infrastructure:** `resources/infrastructure/core.md` (always) + Tier 2 docs per loading rules

---

## What to Explore

### Locating the Node Source

Each factory maps to one or two C++ header files. All node source lives in `hi_dsp_library/dsp_nodes/`:

| Factory | Primary source |
|---------|---------------|
| analyse | `AnalyserNodes.h` |
| container | `../node_api/nodes/Container_*.h` + `processors.h` |
| control | `CableNodes.h` (+ `CableNodeBaseClasses.h` for base classes) |
| core | `CoreNodes.h` |
| dynamics | `DynamicsNode.h` |
| envelope | `EnvelopeNodes.h` |
| filters | `FilterNode.h` |
| fx | `FXNodes.h` |
| jdsp | `JuceNodes.h` |
| math | `MathNodes.h` |
| routing | `RoutingNodes.h` |
| template | Composite nodes (no single source - defined by ValueTree composition) |

Find the node class by searching for `SN_NODE_ID("{nodeId}")` or `SN_POLY_NODE_ID("{nodeId}")` in the relevant header.

### Signal Path Tracing

The primary exploration task. For each node, examine these methods:

| Method | What it reveals |
|--------|----------------|
| `process(ProcessDataType& d)` | Block-level audio processing |
| `processFrame(FrameDataType& data)` | Per-sample audio processing |
| `setParameter<P>(double v)` or `setXxx(double v)` | How parameters affect internal state |
| `handleHiseEvent(HiseEvent& e)` | MIDI event processing (if `IsProcessingHiseEvent`) |
| `prepare(PrepareSpecs ps)` | Initialisation with sample rate, block size, channels |
| `reset()` | State reset behaviour |
| `createParameters(ParameterDataList& data)` | Parameter definitions and ranges |
| `initialise(ObjectWithValueTree* n)` | Runtime property registration and CustomNodeProperties setup |

For each method, document:
- What happens to the input signal
- Where parameters are read and applied
- Conditional branches (mode-dependent behaviour)
- Polyphonic state access patterns (PolyData usage)

### Node Type-Specific Exploration

**Audio processors** (most core, fx, filters, dynamics, jdsp nodes):
- Focus on `process()` and `processFrame()` - these define what the node does
- Check for per-voice state (`PolyData` members)
- Check for parameter smoothing (ramp, smoothed values)

**Control nodes** (control.* factory):
- Focus on `setParameter()` callbacks - these define the value transformation
- Check `initialise()` for CustomNodeProperties registration (OutsideSignalPath, IsControlNode, etc.)
- Check modulation output handling: single output (`parameter_node_base<parameter::dynamic_base_holder>`) vs multi-output (`parameter_node_base<parameter::dynamic_list>`)
- For cable nodes: trace how `Value` parameter input maps to modulation output

**Container nodes** (container.* factory):
- These are typically template instantiations from `processors.h`, not standalone classes
- Focus on which `wrap::` template is used and what it implies:
  - `wrap::fix<C,T>` fixes the channel count
  - `wrap::frame<C,T>` enables per-sample processing
  - `wrap::fix_block<N>` fixes the block size
  - `wrap::oversample<N,T>` adds oversampling
  - `wrap::event<T>` adds MIDI event dispatching
- Check `container_base` dispatch logic for the container type (chain = serial, split = parallel copy+sum, multi = parallel independent)

**Envelope nodes** (envelope.* factory):
- Focus on gate/trigger behaviour - how the envelope responds to note-on/note-off
- Check modulation outputs (CV output, gate output)
- Check voice management integration

**Filter nodes** (filters.* factory):
- All inherit from `filter_base` - check what filter types/modes are available
- Focus on `setMode()` or the mode enum
- Note whether the filter is zero-delay feedback, biquad, etc.

**Math nodes** (math.* factory):
- Most are very simple (single operation per sample)
- Focus on the `process()` body - usually 2-5 lines
- Check if SIMD optimisations are used (via `hmath` or direct SSE)
- Note the relationship between parameter Value and the math operation

**Analyse nodes** (analyse.* factory):
- Focus on display buffer setup and FFT/analysis parameters
- These typically do not modify the audio signal

### Conditional Behaviour

Look for parameter-dependent or property-dependent control flow:
- `switch` / `if` on mode enum values set during `initialise()` or via property change
- `HasModeTemplateArgument` nodes have their mode baked into the C++ template at compile time
- Toggle parameters that enable/disable processing stages

### Performance Assessment

For CPU cost assessment (cpuProfile):
- Is processing per-sample or per-block?
- Are there expensive operations (FFT, convolution, transcendental functions)?
- Does SIMD apply (hmath operations, span iteration)?
- Do any parameters scale the cost?

---

## C++ Exploration Rules

1. **Follow the signal, not the code structure.** The goal is to understand what happens to the audio/control signal. Skip internal buffer management, thread synchronisation, and memory allocation details.

2. **Map parameter enum indices to parameter IDs.** The `Parameters` enum in the C++ class maps to the parameter list in scriptnodeList.json. Verify the correspondence.

3. **Use infrastructure docs.** The Tier 1 `core.md` document explains ProcessData, PolyData, ExternalData, etc. Do not re-discover these - reference the infrastructure knowledge.

4. **Note per-voice state.** Look for `PolyData<T, NumVoices>` members. These indicate per-voice processing that the documentation must describe.

5. **Check for vestigial code.** Parameters or properties that are defined but not used in processing are vestigial. Note these factually.

6. **Verify base data descriptions.** Compare the `description` field from scriptnodeList.json against actual behaviour. Flag inaccuracies.

7. **Note conditional behaviour precisely.** Don't just say "the mode changes behaviour" - describe what each mode value does.

8. **Keep it brief.** Most scriptnode nodes are 30-100 lines of C++. The exploration should be proportional - a simple math node needs 10-20 lines of exploration, not 100.

9. **Do not explore irrelevant code.** Skip: serialisation, UI layout, undo/redo, debug logging, `createEditor()`. Exception: check `initialise()` for CustomNodeProperties setup as this affects runtime behaviour.

10. **Variant efficiency.** For variant groups, explore the base template once. For subsequent variants, note only what differs (parameter count, template argument value).

---

## Output Format: Exploration Markdown

Write one markdown file per node at `scriptnode_enrichment/exploration/{factory}.{node}.md`.

### Template

```markdown
# {factory}.{node} - C++ Exploration

**Source:** `{path/to/header.h}:{line_number}`
**Base class:** `{base class name}`
**Classification:** {audio_processor | control_source | container | analysis | utility}

## Signal Path

[Brief description of what the node does to its input.
For audio processors: input -> operation -> output
For control nodes: Value parameter -> transformation -> modulation output
For containers: how children are dispatched]

## Gap Answers

### {gap-id-1}: {gap question}

[Answer with C++ evidence. Reference method names.
Keep answers concise - most can be 2-5 lines.]

### {gap-id-2}: {gap question}

[Answer...]

## Parameters

[For each parameter, one line describing what it does based on C++ source.
Only include if the behaviour is non-obvious from the parameter name and range.]

## Conditional Behaviour

[Mode switches, property-dependent paths.
For each condition: what controls it, what changes.
Omit section if no conditional behaviour.]

## Polyphonic Behaviour

[How per-voice state is managed.
Which PolyData members exist and what they store.
Omit section if not polyphonic.]

## CPU Assessment

baseline: {negligible | low | medium | high | very_high}
polyphonic: {true | false}
scalingFactors: [{parameter, impact, note}]

## Notes

[Anything else relevant. Cross-node observations.
Omit if nothing notable.]
```

### Exploration Rules

- **Signal Path** and **Gap Answers** are mandatory.
- **Parameters**, **Conditional Behaviour**, **Polyphonic Behaviour** can be omitted if not applicable.
- **CPU Assessment** is mandatory (even if just "baseline: negligible").
- Keep the entire exploration proportional to the node's complexity. A `math.add` node exploration might be 15 lines total. A `core.granulator` exploration might be 80 lines.
- Do not paste large C++ code blocks. Summarise the logic with method name references.

### Variant Exploration Format

For variant group members after the first, use a shortened format:

```markdown
# {factory}.{variant_node} - C++ Exploration (Variant)

**Base variant:** {factory}.{base_node}
**Variant parameter:** {what differs, e.g., "BlockSize = 32"}

## Variant-Specific Behaviour

[Only what differs from the base variant.
Usually just the template parameter value and any implications.]

## CPU Assessment

[Same format. May differ from base if the variant parameter affects performance.]
```

---

## Output Format: Graph JSON

Write one graph JSON file per node at `scriptnode_enrichment/exploration/{factory}.{node}.json`. This is the structured signal-flow topology that Step 4 uses to derive pseudo-code.

### Schema

```json
{
  "factoryPath": "string",
  "nodes": [
    {
      "id": "string (unique node ID)",
      "label": "string (display name)",
      "type": "string (input | output | process | decision | parameter | modulation_out | external_data)",
      "importance": "number (0.0-1.0)",
      "description": "string (what this node does)"
    }
  ],
  "edges": [
    {
      "from": "string (source node ID)",
      "to": "string (target node ID)",
      "type": "string (signal | control | feedback | conditional)",
      "label": "string|null (edge label)"
    }
  ]
}
```

### Node Types

| Type | Colour hint | What it represents |
|------|------------|-------------------|
| `input` | - | Audio or control signal input |
| `output` | - | Audio or modulation signal output |
| `process` | Orange | A processing operation (the node's core function) |
| `decision` | - | A conditional branch (mode switch) |
| `parameter` | Blue | A user-controllable parameter |
| `modulation_out` | Green | Modulation output (for control nodes) |
| `external_data` | - | Table, SliderPack, or AudioFile reference |

### Importance Values

| Range | Step 4 treatment |
|-------|-----------------|
| >= 0.7 | Becomes a pseudo-code statement |
| 0.3 - 0.7 | May become a comment or be folded into adjacent statements |
| < 0.3 | Typically omitted from pseudo-code |

For most scriptnode nodes, the graph is simple:
- 1-2 input nodes (audio in, or Value parameter)
- 1 process node (the core operation)
- 1-6 parameter nodes controlling the process
- 1 output node (audio out, or modulation output)
- All nodes at importance >= 0.7

Complex nodes (granulator, file_player, containers) will have more nodes and varying importance.

### Graph Construction Rules

1. Walk the signal path from input to output.
2. Parameters that control the process become `parameter` nodes with `control` edges.
3. Decision nodes for mode switches use `conditional` edges.
4. Control nodes have a `modulation_out` node instead of an audio `output`.
5. Nodes with ComplexData have an `external_data` node feeding into the process.

### Simple Node Example

For `math.add`:

```json
{
  "factoryPath": "math.add",
  "nodes": [
    { "id": "in", "label": "Audio In", "type": "input", "importance": 1.0, "description": "Input audio signal" },
    { "id": "add", "label": "Add", "type": "process", "importance": 1.0, "description": "Adds a constant value to each sample" },
    { "id": "value", "label": "Value", "type": "parameter", "importance": 1.0, "description": "DC offset to add" },
    { "id": "out", "label": "Audio Out", "type": "output", "importance": 1.0, "description": "Signal with DC offset applied" }
  ],
  "edges": [
    { "from": "in", "to": "add", "type": "signal", "label": null },
    { "from": "value", "to": "add", "type": "control", "label": null },
    { "from": "add", "to": "out", "type": "signal", "label": null }
  ]
}
```

### Container Node Graphs

Container nodes describe how children are dispatched, not what the children do:

```json
{
  "factoryPath": "container.split",
  "nodes": [
    { "id": "in", "label": "Audio In", "type": "input", "importance": 1.0, "description": "Input signal copied to each child" },
    { "id": "copy", "label": "Copy to children", "type": "process", "importance": 0.8, "description": "Each child receives a copy of the input" },
    { "id": "sum", "label": "Sum outputs", "type": "process", "importance": 1.0, "description": "Child outputs are summed together" },
    { "id": "out", "label": "Audio Out", "type": "output", "importance": 1.0, "description": "Combined output" }
  ],
  "edges": [
    { "from": "in", "to": "copy", "type": "signal", "label": null },
    { "from": "copy", "to": "sum", "type": "signal", "label": "N children" },
    { "from": "sum", "to": "out", "type": "signal", "label": null }
  ]
}
```

---

## Flagging Base JSON Issues

### When to Flag

Flag an issue when you discover:
- A parameter description that contradicts actual behaviour
- A parameter range that does not match the code
- A vestigial parameter or property (defined but has no effect)
- A missing parameter that exists in the C++ enum but not in scriptnodeList.json
- A cppProperties flag that is incorrect or missing

### Issue Format

```markdown
### {factory}.{node} -- {short description}

- **Type:** {silent-fail | missing-validation | inconsistency | vestigial | ux-issue}
- **Severity:** {critical | high | medium | low}
- **Location:** {file path}:{line number}
- **Observed:** {what you found}
- **Expected:** {what should happen}
```

### Severity Guidelines

| Severity | Criteria |
|----------|----------|
| `critical` | Node produces incorrect audio output or crash |
| `high` | Parameter behaviour contradicts its description |
| `medium` | Vestigial parameter, minor inconsistency, misleading description |
| `low` | Cosmetic issue, redundant logic |

### Bug Discovery Policy

Issues must NOT appear in documentation output. The exploration markdown may mention vestigial or non-functional features factually (so Step 4 knows to handle them), but line numbers, fix suggestions, and bug analysis go only to issues.md.

---

## Step 3 Gate Checklist

Before handing off to Step 4, verify:

- [ ] Every gap question from the preliminary JSON has a corresponding answer
- [ ] The Signal Path section describes what the node does to its input
- [ ] Every parameter from the preliminary JSON has been located in C++ and its behaviour understood
- [ ] All discovered vestigial parameters or inaccurate descriptions are flagged in issues.md
- [ ] CPU assessment is provided (baseline tier + polyphonic flag)
- [ ] Conditional behaviour (if any) is documented with specific values and effects
- [ ] The graph JSON captures the signal-flow topology
- [ ] The exploration is proportional to the node's complexity
- [ ] The exploration is self-contained - Step 4 can write the reference page without re-reading C++
