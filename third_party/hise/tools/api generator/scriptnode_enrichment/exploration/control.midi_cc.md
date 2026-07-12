# control.midi_cc - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/CableNodes.h:951`
**Base class:** `pimpl::parameter_node_base<ParameterClass>`, `pimpl::no_processing`
**Classification:** control_source

## Signal Path

MIDI event -> handleHiseEvent() -> checks event type matches expectedType and CC number -> normalises value to 0..1 -> forwards to output parameter.

midi_cc listens for MIDI events and extracts a normalised 0..1 value when the configured CC number/type is received. Despite having `no_processing` as a base (OutsideSignalPath), it implements `handleHiseEvent()` directly (not via IsProcessingHiseEvent in cppProperties -- see Notes).

## Gap Answers

### cc-listening-mechanism: How does midi_cc receive MIDI events?

midi_cc implements `handleHiseEvent(HiseEvent& e)` directly. While it does not have `IsProcessingHiseEvent` in cppProperties, it does have a non-empty `handleHiseEvent()` method. The event forwarding must be arranged by the container or runtime system. The node inherits `SN_EMPTY_HANDLE_EVENT` from `no_processing`, but then overrides it with its own implementation in the class body (the class definition comes after the base class, so the member function in the derived class shadows the base).

### cc-range-mapping: CCNumber values 128-131.

From `MidiCCHelpers::SpecialControllers` and `getTypeForNumber()`:

- 0-127: Standard MIDI CC (expectedType = Controller). Named CCs: 1=Modwheel, 2=Breath Control, 7=Volume, 11=Expression, 64=Sustain.
- 128 (PitchWheelCCNumber): PitchBend. Value = `getPitchWheelValue() / 16384.0` (0..1).
- 129 (AfterTouchCCNumber): Aftertouch. Value = `getNoteNumber() / 127.0`.
- 130 (Stroke): NoteOn. Value = `getVelocity() / 127.0`.
- 131 (Release): NoteOff. Value = `getVelocity() / 127.0`.

### mpe-behaviour: What does EnableMPE=true change?

In the C++ source, `setEnableMPE(double)` sets `enableMpe` boolean but this flag is never read in `handleHiseEvent()`. The `isInPolyphonicContext` member is also declared but not used in the event handler. The MPE filtering appears to be vestigial or handled at a higher level in the runtime system.

### default-value-trigger: When is DefaultValue sent?

`setDefaultValue(double)` stores the value and immediately calls `this->getParameter().call(defaultValue)` if connected. This means DefaultValue is sent whenever the parameter is set -- on initialisation, and any time it is modulated. It serves as both a default and a manual override.

## Parameters

- **CCNumber**: Selects which MIDI message type to listen for (0-131). Setting this updates `midiNumber` and `expectedType` via `getTypeForNumber()`.
- **EnableMPE**: Sets `enableMpe` flag. Currently appears vestigial in the event handler.
- **DefaultValue**: Immediately sent to output when set. Acts as fallback/initial value.

## CPU Assessment

baseline: negligible
polyphonic: false
scalingFactors: []

## Notes

The `handleHiseEvent` implementation first checks for Controller type with CC number mismatch (returns early). Then checks if the event type matches `expectedType`. For PitchBend, the normalisation is `getPitchWheelValue() / 16384.0` (the full 14-bit range). For Aftertouch, it uses `getNoteNumber() / 127.0` which is the aftertouch pressure value stored in the note number field of HiseEvent.
