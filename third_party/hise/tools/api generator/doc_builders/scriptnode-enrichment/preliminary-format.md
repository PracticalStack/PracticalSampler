# Preliminary JSON Format (Steps 1-2)

**Purpose:** Create a structured inventory of everything extractable from `scriptnodeList.json` plus agent judgment, and identify specific gaps requiring C++ exploration. This is a working document for the agent - it is never rendered or served to users.

**Input:** One or more node entries from `scriptnode_enrichment/base/scriptnodeList.json`
**Output:** `scriptnode_enrichment/preliminary/{factory}.{node}.json` (one per node)
**Reference tables:** Inference tables in `scriptnode-enrichment.md`
**Infrastructure:** `scriptnode_enrichment/resources/infrastructure/core.md` (always) + Tier 2 docs per loading rules

---

## What This Format Is (And Isn't)

The preliminary JSON is a **structured analysis worksheet**. It captures:

- What we know from the base data (parameters, properties, cppProperties, ComplexData, description)
- How parameters relate to each other (semantic grouping)
- What we cannot determine without reading C++ source (gaps)

It is **not** a signal-flow topology. The graph JSON (Step 3 output) and reference page (Step 4 output) will have different structures. The preliminary JSON groups information by source; the downstream outputs group information by signal flow.

---

## Schema

```json
{
  "factoryPath": "string (e.g., 'fx.bitcrush')",
  "factory": "string (e.g., 'fx')",
  "nodeId": "string (e.g., 'bitcrush')",
  "description": "string (from base JSON)",

  "classification": {
    "signalPathRole": "string (audio_processor | control_source | container | analysis | utility)",
    "isPolyphonic": "boolean",
    "isControlNode": "boolean",
    "outsideSignalPath": "boolean",
    "isProcessingHiseEvent": "boolean",
    "voiceContext": "string (per-voice | monophonic)"
  },

  "parameterGroups": [
    {
      "label": "string (semantic group name)",
      "rationale": "string (why these parameters belong together)",
      "parameters": [
        {
          "id": "string (parameter ID)",
          "range": { "min": "number", "max": "number", "step": "number", "skew": "number" },
          "default": "number",
          "textConverter": "string|null (encoded converter or 'Undefined')",
          "unnormalisedModulation": "boolean (true if listed in UseUnnormalisedModulation)"
        }
      ]
    }
  ],

  "properties": [
    {
      "id": "string (property ID, e.g., 'Mode', 'Code')",
      "defaultValue": "string|number|boolean",
      "type": "string (inferred: enum | string | boolean | integer)",
      "note": "string (what this property likely controls)"
    }
  ],

  "complexData": {
    "tables": "number (count of Table slots, 0 if none)",
    "sliderPacks": "number (count of SliderPack slots, 0 if none)",
    "audioFiles": "number (count of AudioFile slots, 0 if none)"
  },

  "modulationOutput": {
    "hasOutput": "boolean",
    "isUnnormalised": "boolean",
    "namedOutputs": ["string (output names, if multi-output)"]
  },

  "switchTargets": {
    "hasSwitchTargets": "boolean",
    "count": "number"
  },

  "variantGroup": {
    "isVariant": "boolean",
    "groupName": "string|null (e.g., 'packN_writer')",
    "templateParameter": "string|null (what varies, e.g., 'number of Value parameters')",
    "variantValue": "string|null (e.g., '5' for pack5_writer)"
  },

  "existingDoc": {
    "path": "string|null (path to existing phase3 doc)",
    "tier": "string (STUB | BRIEF | GOOD)",
    "lines": "number"
  },

  "availableImages": ["string (image filenames from resources/images/ that match this node)"],

  "gaps": [
    {
      "id": "string (unique gap identifier)",
      "category": "string (signal_path | parameter_behaviour | conditional_logic | description_accuracy | performance)",
      "question": "string (specific question for C++ exploration)",
      "context": "string (why this matters for the reference page)"
    }
  ],

  "notes": "string (free-form observations, cross-references, uncertainties)"
}
```

---

## Field-by-Field Reference

| Field | Required | Source | Description |
|-------|----------|-------|-------------|
| `factoryPath` | yes | scriptnodeList.json key | Full factory.node path |
| `factory` | yes | Derived from factoryPath | Factory name |
| `nodeId` | yes | scriptnodeList.json `ID` | Node ID within factory |
| `description` | yes | scriptnodeList.json | Short description |
| `classification` | yes | cppProperties + inference table | Node role classification |
| `parameterGroups` | yes | Agent judgment + scriptnodeList.json | Semantically grouped parameters |
| `properties` | if any | scriptnodeList.json `Properties` | Runtime-configurable properties |
| `complexData` | yes | scriptnodeList.json `ComplexData` | External data slot counts |
| `modulationOutput` | yes | scriptnodeList.json `ModulationTargets` + cppProperties | Modulation output configuration |
| `switchTargets` | yes | scriptnodeList.json `SwitchTargets` | Switch target configuration |
| `variantGroup` | yes | Pattern matching (see below) | Variant group membership |
| `existingDoc` | yes | Cross-reference with phase3/ | Existing documentation status |
| `availableImages` | yes | Cross-reference with resources/images/ | Available screenshots |
| `gaps` | yes | Agent judgment | Questions requiring C++ exploration |
| `notes` | no | Agent judgment | Free-form observations |

---

## Step 1: Create Preliminary JSON

### Inputs

- The node's entry from `scriptnode_enrichment/base/scriptnodeList.json`
- The inference tables from `scriptnode-enrichment.md`
- The infrastructure reference: `resources/infrastructure/core.md`
- Tier 2 infrastructure docs per the loading rules

### Procedure

1. **Extract identity fields.** Copy `factoryPath`, `factory`, `nodeId`, `description` from the base data.

2. **Classify the node.** Use the cppProperties inference table:
   - Map cppProperties flags to `signalPathRole`, `voiceContext`
   - Set boolean flags: `isPolyphonic`, `isControlNode`, `outsideSignalPath`, `isProcessingHiseEvent`

3. **Group parameters semantically.** Do not list parameters alphabetically. Group by function:
   - Parameters that control the same aspect belong together (e.g., "Value" + "Multiply" + "Add" for pma)
   - Parameters that form enable/value pairs belong together
   - For control nodes, "Value" is typically the input signal - group it separately from configuration parameters
   - Give each group a descriptive label and rationale

4. **Catalogue properties.** For each entry in the `Properties` object:
   - Record the ID, default value, and inferred type
   - Add a note about what it likely controls (from the ID name and context)
   - Properties named "Mode" typically select between algorithm variants
   - Properties named "Code" indicate user-editable code (SNEX expressions)

5. **Count ComplexData slots.** Record how many Tables, SliderPacks, and AudioFiles the node uses. Zero if the `ComplexData` object is absent or empty.

6. **Assess modulation output.** Check `ModulationTargets` and cppProperties:
   - `IsControlNode: true` with `ModulationTargets` present = has modulation output
   - `UseUnnormalisedModulation` = output sends raw values
   - Check for named outputs in `ModulationTargets` (multi-output nodes like xy have named outputs)

7. **Check switch targets.** Record whether `SwitchTargets` is present and its count.

8. **Identify variant group membership.** Check if the node matches a known variant pattern:
   - `control.packN_writer` (N = 2-8): varies by parameter count
   - `container.fixN_block` (N = 8-256): varies by block size
   - `container.oversampleNx` (N = 2-16): varies by factor
   - `template.softbypass_switchN` (N = 2-8): varies by switch count
   - `template.freq_splitN` (N = 2-5): varies by band count

9. **Check existing documentation.** Look up `phase3/{factory}/{nodeId}.md`:
   - Count lines. Classify: STUB (<=8), BRIEF (9-20), GOOD (21+)
   - Record the path and tier

10. **Match available images.** Scan `resources/images/` for filenames that match or relate to this node.

11. **Write notes.** Record observations, uncertainties, or cross-node relationships.

### Step 1 Gate Checklist

- [ ] Every parameter from the base data appears in exactly one parameter group
- [ ] All properties are catalogued with type inference
- [ ] ComplexData slot counts are recorded (even if all zero)
- [ ] Modulation output is assessed (even if none)
- [ ] Switch targets are checked (even if none)
- [ ] Variant group membership is determined (even if not a variant)
- [ ] Existing doc tier is recorded
- [ ] Classification flags match the cppProperties from the base data
- [ ] No field from the base data entry has been ignored without justification

---

## Step 2: List Exploration Gaps

### Inputs

- The preliminary JSON from Step 1
- The node's classification and factory

### Procedure

Review the preliminary JSON and identify everything that cannot be determined from the base data alone. Write specific, answerable questions.

### Gap Categories

| Category | What it covers |
|----------|---------------|
| `signal_path` | What the node does to the audio/control signal, processing order |
| `parameter_behaviour` | How a parameter affects the processing (e.g., "Does BitDepth operate continuously or in discrete steps?") |
| `conditional_logic` | Mode switches, property-gated behaviour (e.g., "What does Mode=Bipolar vs Mode=DC_Offset change?") |
| `description_accuracy` | Base data descriptions that seem wrong, vague, or incomplete |
| `performance` | CPU-relevant details (per-sample vs per-block, SIMD usage, etc.) |

### Writing Good Gap Questions

**Good gaps are specific and answerable from C++ source:**
- "What is the processing order in `process()`? Does it iterate channels then samples, or samples then channels?"
- "Parameter `BitDepth` has range 1-16. Is the quantisation continuous (fractional bits) or discrete (integer steps only)?"
- "The `Mode` property defaults to 'Peak'. What modes are available and what does each measure?"

**Bad gaps are vague or unanswerable:**
- "How does this node work?" (too broad)
- "Is this a good node for bass?" (subjective)
- "What settings do people typically use?" (requires project analysis, not C++ exploration)

### Gap ID Convention

Use descriptive kebab-case IDs: `processing-order`, `bitdepth-quantisation`, `mode-variants`, `frequency-range-mapping`.

### Category-Specific Gap Patterns

Different node types have predictable gap patterns:

**Audio processors** (OutsideSignalPath: false):
- Processing order within `process()` / `processFrame()`
- Whether processing is per-sample or per-block
- Parameter smoothing behaviour
- Polyphonic state handling

**Control nodes** (IsControlNode: true):
- How the input Value is transformed before output
- Whether the output is normalised (0-1) or raw
- Edge cases: what happens at parameter boundaries
- For multi-output nodes: how outputs are distributed

**Container nodes** (container.* factory):
- How children are dispatched (serial, parallel, branched)
- What wrap:: template is used and what it implies
- Bypass behaviour

**Nodes with Properties:**
- What each Mode/option value does
- How the property affects the C++ template instantiation (if `HasModeTemplateArgument`)

**Nodes with ComplexData:**
- How the table/pack/audiofile is used in processing
- Whether the data is read per-sample or per-block
- Index mapping behaviour

### Variant Group Handling

For nodes in a variant group, the first variant processed should have full gaps. Subsequent variants reference the base exploration and only need gaps for variant-specific behaviour:

```json
{
  "id": "variant-difference",
  "category": "parameter_behaviour",
  "question": "pack5_writer has 5 Value parameters vs pack2_writer's 2. Is the write logic identical except for the parameter count?",
  "context": "If identical, the doc can reference the base variant and note only the parameter count difference."
}
```

### Minimum Gap Requirements

| Node type | Minimum gaps |
|-----------|-------------|
| Audio processor | At least 1 `signal_path` gap |
| Control node | At least 1 `signal_path` or `parameter_behaviour` gap |
| Container | At least 1 `signal_path` gap (how children are dispatched) |
| Node with Mode property | At least 1 `conditional_logic` gap |
| Trivially simple node (e.g., math.abs) | 0 gaps acceptable if behaviour is self-evident |

### Step 2 Gate Checklist

- [ ] Minimum gap requirements are met for the node type
- [ ] Every property with non-obvious behaviour has a `conditional_logic` gap
- [ ] If the base data description seems vague or suspicious, there is a `description_accuracy` gap
- [ ] If the node has ComplexData, there is a gap about how the data is used
- [ ] Each gap has a unique ID, a category, a specific question, and context
- [ ] Gaps are ordered by importance (signal_path first, performance last)
- [ ] For variant nodes: base variant has full gaps, subsequent variants have only variant-specific gaps
