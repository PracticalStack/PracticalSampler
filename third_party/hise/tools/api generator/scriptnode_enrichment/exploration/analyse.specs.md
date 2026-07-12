# analyse.specs - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/AnalyserNodes.h` (no dedicated specs class; uses generic analyse_base)
**Base class:** `analyse_base<void>` or generic passthrough (no PropertyObject)
**Classification:** utility (debug/informational)

## Signal Path

Audio passes through unmodified. The specs node does not write to a display buffer. Instead, it renders UI directly showing processing context information (sample rate, block size, channel count, etc.). This is a debug/monitoring tool, not an analysis node.

Process flow: input audio -> analyse_base::process() -> no output -> (side effect: UI displays PrepareSpecs and processing context)

## Gap Answers

### description-accuracy: What information does specs display?

The specs node displays:
1. Sample rate (from PrepareSpecs.sampleRate)
2. Block size (from PrepareSpecs.blockSize)
3. Channel count (from PrepareSpecs.numChannels)
4. MIDI processing status (derived from node properties or polyphony state)
5. Polyphony enabled status (from PolyHandler via PrepareSpecs.voiceIndex)

The node reads PrepareSpecs in prepare() method and caches it, then a UI component displays this information. Existing phase3 doc correctly lists these fields.

### signal-path-passthrough: Does specs pass audio?

Yes, pure passthrough. The process() template (lines 482-486) is empty for specs (the specs "node" likely specializes to do nothing in process/processFrame). The node is ignored in C++ export (UncompileableNode: true in cppProperties).

### what-is-displayed: What exact information is shown?

From PrepareSpecs (infrastructure core.md, section 4):
- sampleRate (double)
- blockSize (int)
- numChannels (int)
- voiceIndex pointer (indicates polyphony enabled)

Derived state:
- Whether MIDI is being processed (from shouldProcessHiseEvent flag in OpaqueNode)
- Voice count (if polyphony enabled)
- Total polyphonic voice capacity (NUM_POLYPHONIC_VOICES if enabled)

The display is non-audio, purely informational. A UI component (likely simple_specs_display or similar, not in this header) renders these values.

### uncompileable-implications: What does UncompileableNode mean?

UncompileableNode: true means the specs node is skipped during C++ source code export/compilation. When a scriptnode network is compiled to a standalone C++ plugin, the specs node is completely removed -- it is a design-time debugging tool only, with no effect on compiled output. This is appropriate since the node displays runtime context rather than processing audio.

## Parameters

None.

## Conditional Behaviour

None. The specs node displays static PrepareSpecs information. It does not have conditional logic based on parameters or properties (other than UI visibility).

## Polyphonic Behaviour

Not polyphonic. The node monitors polyphony state but does not process per-voice.

## CPU Assessment

baseline: negligible
polyphonic: false
scalingFactors: []

## Notes

The specs node is unique among analyse nodes:
- No display buffer (unlike fft, oscilloscope, goniometer)
- No parameters or properties
- No audio processing
- Renders its own UI directly showing PrepareSpecs

It serves as a debug/information display only. The cppProperties flag UncompileableNode: true correctly identifies it as a design-time tool. This is appropriate for a node that displays processing context rather than analyzing audio.

The description field in scriptnodeList.json is empty, but existing phase3 doc correctly documents the displayed information.
