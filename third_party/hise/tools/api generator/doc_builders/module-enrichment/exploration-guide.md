# C++ Exploration Guide (Step 3)

**Purpose:** Answer the gap questions from the preliminary JSON by reading the module's C++ source code. Produce a structured exploration markdown and a graph JSON topology that Step 4 will consume to author the MDC reference page. Flag any base data inaccuracies as sidecar issues.

**Input:** `module_enrichment/preliminary/{ModuleId}.json`
**Output:**
- `module_enrichment/exploration/{ModuleId}.md` (exploration findings)
- `module_enrichment/exploration/{ModuleId}.json` (graph topology)
**Issues:** Append to `module_enrichment/issues.md`

---

## What to Explore

### Locating the Module Source

1. Use the `moduleId` from the preliminary JSON to find the C++ class. Module classes are in `hi_core/hi_modules/` organized by type:
   - `modulators/mods/` - Modulator classes
   - `synthesisers/synths/` - SoundGenerator classes
   - `effects/fx/` - Effect classes
   - `midi_processor/` - MidiProcessor classes

2. The class typically has a header (`.h`) and implementation (`.cpp`) file. Some simple modules are header-only.

3. For modules that extend base classes, also read the base class. Key base classes:
   - `EnvelopeModulator` - base for all envelope modulators
   - `VoiceStartModulator` - base for voice start modulators
   - `TimeVariantModulator` - base for time-variant modulators
   - `MasterEffectProcessor` - base for master effects
   - `VoiceEffectProcessor` - base for voice effects
   - `ModulatorSynth` - base for sound generators
   - `MidiProcessor` - base for MIDI processors

### Signal Path Tracing

The primary exploration task. Follow these methods in order:

| Method | Module types | What it reveals |
|--------|-------------|-----------------|
| `processBlock()` | Effects | Main audio processing chain |
| `applyEffect()` | Effects (called by processBlock) | Per-buffer effect application |
| `renderNextBlock()` | SoundGenerators | Voice rendering pipeline |
| `calculateBlock()` | SoundGenerators, Modulators | Per-voice block calculation |
| `calculateVoiceStartValue()` | VoiceStartModulators | Note-on value calculation |
| `processHiseEvent()` | MidiProcessors | MIDI event transformation |
| `startNote()` / `stopNote()` | Per-voice modules | Voice lifecycle |

For each method, document:
- The processing order (what happens first, second, third)
- Where parameters are read and applied
- Where modulation chains are applied
- Feedback paths (write-then-read of the same buffer)
- Conditional branches (parameter-gated behavior)

### Voice Architecture

For per-voice modules, look for:
- `VoiceData` or similar per-voice state structs
- Voice index usage in array access
- `startNote()` / `stopNote()` for voice initialization/cleanup
- Whether any state is shared across voices (shared buffers, lookup tables)

### Conditional Behavior

Look for parameter-dependent control flow:
- `switch` / `if` on `getAttribute()` or member variables set by `setInternalAttribute()`
- Mode enums that change the processing algorithm
- Toggle parameters that enable/disable processing stages
- Parameters that gate other parameters (e.g., "this slider only matters when mode is X")

### Interface Implementations

For modules that implement processor interfaces, trace how the interface is used:
- `TableProcessor`: Where is `getTableValue()` called in the signal path?
- `SliderPackProcessor`: Where is the slider pack data read?
- `AudioSampleProcessor`: Where is the audio sample read/played?
- `SlotFX`: How is the hosted effect called?

### Performance-Relevant Details

For CPU cost assessment (cpuProfile):
- Is processing per-sample or per-block?
- Are there expensive operations (FFT, convolution, oversampling)?
- Do any parameters scale the computational cost (e.g., unisono count, filter order)?
- Is there downsampling or early-exit optimization?

### Editor / UI Component Discovery

Inspect `createEditor()` or the associated editor class for FloatingTile content types. These become `ui_component` seeAlso entries. Look for:
- Panel class names (e.g., `AhdsrEnvelopePanel`, `SampleMapEditor`)
- FloatingTile registrations
- Custom editor components

---

## C++ Exploration Rules

1. **Follow the signal path, not the code structure.** The goal is to understand what happens to the audio/MIDI/modulation signal, not to document the class hierarchy. Internal buffer management, thread synchronization, and memory allocation are not signal flow stages.

2. **Map `setAttribute` indices to parameter IDs.** The `Parameters` enum in the C++ class maps to the parameter list in moduleList.json. Verify the correspondence - discrepancies are issues to flag.

3. **Trace `getChildProcessorChain` for modulation inputs.** This reveals which modulation chains the module exposes and how they connect to the processing.

4. **Look for internal buffers** (AudioSampleBuffer, delay lines, IR buffers) - these indicate `shared_resource` scope nodes that persist across voices.

5. **Look for per-voice state** (VoiceData structs, voice-indexed arrays) - these indicate `per_voice` scope nodes.

6. **Check for vestigial code.** Parameters that are defined, serialized, and exposed in metadata but not used in any processing method are vestigial. Note these factually - they will be marked as vestigial in the reference page.

7. **Verify base data descriptions.** Compare the `description` field in moduleList.json against actual behavior. Flag inaccuracies.

8. **Note conditional behavior precisely.** Don't just say "the mode parameter changes behavior" - describe what each mode value does to the signal path.

9. **Assess CPU cost at the node level.** Use the 5-tier scale from `module-enrichment.md` (negligible, low, medium, high, very_high). Note any parameters that scale the cost.

10. **Do not explore code that is irrelevant to the signal flow.** Skip: serialization (`restoreFromValueTree`/`exportAsValueTree`), UI layout code (unless checking `createEditor()` for FloatingTile types), undo/redo, debug logging.

---

## Output Format: Exploration Markdown

Write one markdown file per module at `module_enrichment/exploration/{ModuleId}.md`.

### Template

```markdown
# {ModuleName} - C++ Exploration

**Source:** `{path/to/header.h}`, `{path/to/impl.cpp}`
**Base class:** `{BaseClassName}`

## Signal Path

[Narrative description of the processing chain in execution order.
What happens first, what happens next, what comes out.
Use arrow notation for clarity: input -> stage1 -> stage2 -> output]

## Gap Answers

### {gap-id-1}: {gap question}

[Answer with specific C++ evidence. Reference method names and
describe the behavior. Do not paste large code blocks - summarize
the logic.]

### {gap-id-2}: {gap question}

[Answer...]

## Processing Chain Detail

[Ordered list of processing stages with:
- What each stage does
- Which parameters control it
- Whether it's per-voice or shared
- Approximate CPU weight tier]

## Modulation Points

[Where modulation chains are applied in the signal path.
Which chains modulate which parameters or implicit values.]

## Conditional Behavior

[Mode switches, toggles, parameter-gated paths.
For each condition: what parameter controls it, what changes.]

## Interface Usage

[How each implemented interface participates in the signal path.
Skip if no relevant interfaces.]

## Vestigial / Notable

[Parameters or features that are defined but not functional.
Cross-reference with issues.md entries if flagged.]

## CPU Assessment

[Per-stage CPU weight assessment.
Parameters that scale cost.
Overall baseline tier.]

## UI Components

[FloatingTile content types found in createEditor().
Skip if none found.]

## Notes

[Anything else relevant that doesn't fit above sections.
Cross-module observations, unusual patterns, design decisions.]
```

### Exploration Markdown Rules

- **Signal Path** and **Gap Answers** are mandatory. Every other section can be omitted if not applicable.
- **Gap Answers** must have one subsection per gap from the preliminary JSON. Use the exact gap ID as the heading.
- **Processing Chain Detail** should be ordered by execution order, not parameter order.
- Do not paste large C++ code blocks. Summarise the logic in plain language with method name references.

---

## Output Format: Graph JSON

Write one graph JSON file per module at `module_enrichment/exploration/{ModuleId}.json`. This is the structured signal-flow topology that Step 4 uses to derive pseudo-code for the reference page.

### Schema

```json
{
  "moduleId": "string",
  "nodes": [
    {
      "id": "string (unique node ID)",
      "label": "string (display name)",
      "type": "string (input|output|process|decision|parameter|modulation|external)",
      "importance": "number (0.0-1.0)",
      "group": "string|null (group ID)",
      "description": "string (what this node does)"
    }
  ],
  "edges": [
    {
      "from": "string (source node ID)",
      "to": "string (target node ID)",
      "type": "string (signal|control|feedback|conditional)",
      "label": "string|null (edge label)"
    }
  ],
  "groups": [
    {
      "id": "string (group ID)",
      "label": "string (group display name)"
    }
  ]
}
```

### Node Importance

The `importance` value controls pseudo-code abstraction in Step 4:

| Range | Step 4 treatment |
|-------|-----------------|
| >= 0.7 | Becomes a code statement or expression |
| 0.3 - 0.7 | May become a comment or be folded into adjacent statements |
| < 0.3 | Typically omitted from pseudo-code |

### Graph Construction Rules

1. Walk the signal path from input to output, creating nodes for each processing stage.
2. Parameters that control a processing stage become `parameter` type nodes connected via `control` edges.
3. Modulation chains become `modulation` type nodes connected via `control` edges.
4. Decision nodes (mode switches, conditionals) use the `decision` type with `conditional` edges for branches.
5. Feedback paths use `feedback` type edges (write-then-read patterns).
6. Group related nodes logically (e.g. "Envelope Stages", "Feedback Loop").

---

## Flagging Base JSON Issues

### When to Flag

Flag an issue when you discover:
- A parameter description that contradicts actual behavior
- A parameter that is defined but has no effect on processing (vestigial)
- A parameter range that doesn't match the code (e.g., range says 0-1 but code treats it as 0-100)
- A missing parameter that exists in the C++ enum but not in moduleList.json
- A modulation chain that is incorrectly linked or described

### Issue Format

Follow the existing format in `module_enrichment/issues.md`:

```markdown
### {ModuleName} -- {short description}

- **Type:** {silent-fail | missing-validation | inconsistency | code-smell | ux-issue | vestigial}
- **Severity:** {critical | high | medium | low}
- **Location:** {file path}:{line number}
- **Observed:** {what you found, with enough detail to reproduce}
- **Expected:** {what should happen or what should be fixed}
```

### Severity Guidelines

| Severity | Criteria |
|----------|----------|
| `critical` | Parameter causes incorrect audio output or crash |
| `high` | Parameter behavior contradicts its description in a user-visible way |
| `medium` | Vestigial parameter, minor inconsistency, or misleading description |
| `low` | Code smell, redundant logic, or cosmetic issue |

### Where to Write

Append new issues to `module_enrichment/issues.md` under the appropriate severity heading. Do not modify existing issues.

### Bug Discovery Policy

Issues found during exploration must NOT appear in any documentation output (description, commonMistakes, llmRef). Users should not see implementation bugs in their documentation. Bugs are transient; documentation is long-lived.

The `notes` field in the exploration markdown may mention vestigial or non-functional features as factual observations about the DSP path (e.g., "LowPassFreq parameter is defined but not applied in applyEffect()"). This tells Step 4 to mark the feature as vestigial in the reference page. Do not include line numbers, fix suggestions, or bug analysis in the notes - put those in issues.md only.

---

## Step 3 Gate Checklist

Before handing off to Step 4, verify:

- [ ] Every gap question from the preliminary JSON has a corresponding answer in the Gap Answers section (no gaps left unanswered)
- [ ] The Signal Path section describes the complete processing chain from input to output
- [ ] Every parameter from the preliminary JSON has been located in the C++ source and its behavior documented (either in Gap Answers, Processing Chain Detail, or Vestigial/Notable)
- [ ] Every interface marked with a signal flow role in the preliminary JSON has its usage documented in the Interface Usage section
- [ ] All discovered vestigial parameters or inaccurate descriptions have been flagged in issues.md
- [ ] CPU weight assessments are provided for all significant processing stages
- [ ] Conditional behavior (if any) is documented with specific parameter values and their effects
- [ ] The exploration markdown is self-contained - Step 4 can author the reference page without needing to re-read C++ source
- [ ] The graph JSON captures the signal-flow topology with nodes, edges, groups, and importance values
