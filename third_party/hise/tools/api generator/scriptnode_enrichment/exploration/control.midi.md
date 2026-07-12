# control.midi - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/EventNodes.h:72`
**Base class:** `pimpl::templated_mode`
**Classification:** control_source

## Signal Path

MIDI event -> handleHiseEvent() -> MidiType::getMidiValue(e, value) -> if changed, ModValue::setModValueIfChanged() -> handleModulation() consumed by wrap::mod -> normalised output to connected parameters.

control.midi does NOT inherit from parameter_node_base or no_processing. It uses the wrap::mod pattern: it has non-empty handleHiseEvent() and handleModulation() methods. The process() and processFrame() methods are empty (SN_EMPTY_PROCESS/SN_EMPTY_PROCESS_FRAME). It sits inside the signal path.

## Gap Answers

### mode-variants: Confirm all available Mode values.

The mode namespace is `"midi_logic"`. From `logic_classes.h`, the available modes are:

1. **gate**: Triggers on NoteOn (value=1.0) and NoteOff (value=0.0). Output 0..1.
2. **velocity**: Triggers on NoteOn only. Output = `velocity / 127.0`. Range 0..1.
3. **notenumber**: Triggers on NoteOn only. Output = `getNoteNumberIncludingTransposeAmount() / 127.0`. Range 0..1.
4. **frequency**: Triggers on NoteOn only. Output = `getFrequency() / 20000.0`. Range 0..~1.
5. **random**: Triggers on NoteOn only. Output = `Random(0.0, 1.0)`. Range 0..1.

All outputs are normalised 0..1 (`isNormalisedModulation() == true`).

### processing-pattern: Does control.midi have non-empty process()/processFrame()?

No. Both are empty (SN_EMPTY_PROCESS, SN_EMPTY_PROCESS_FRAME). The node is purely event-driven via handleHiseEvent(). However, it DOES sit in the signal path (no OutsideSignalPath property) and uses wrap::mod to forward modulation values. The wrap::mod wrapper calls checkModValue() after handleHiseEvent(), which consumes the ModValue and forwards to connected parameters.

### polyphonic-template: Does the Mode template control polyphonic behaviour?

The `TemplateArgumentIsPolyphonic` property is set, meaning the mode template argument also carries a polyphonic voice count. However, `isPolyphonic()` always returns `false` on the midi class itself. The polyphonic aspect is that some MidiType classes (like `frequency`) have `IsProcessingHiseEvent() = true` as a static constexpr which affects how events route in polyphonic containers. The node itself stores no per-voice state -- it uses a single `ModValue v` member.

## Parameters

No parameters. The node is controlled entirely by the Mode property and incoming MIDI events.

## CPU Assessment

baseline: negligible
polyphonic: false
scalingFactors: []

## Notes

The midi node's `prepare()` forwards to `mType.prepare(ps)` and `reset()` calls `v.reset()`. The `initialise()` method forwards to the MidiType if it has an initialise method. The constructor explicitly sets `IsProcessingHiseEvent` and `TemplateArgumentIsPolyphonic` as custom node properties.
