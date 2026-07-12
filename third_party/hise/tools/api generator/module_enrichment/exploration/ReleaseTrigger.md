# ReleaseTrigger - C++ Exploration

**Source:** `hi_scripting/scripting/HardcodedScriptProcessor.h` (lines 327-467)
**Base class:** `HardcodedScriptProcessor` (extends `ScriptBaseMidiProcessor`)

## Signal Path

ReleaseTrigger is a HardcodedScriptProcessor that transforms noteOff events into noteOn events for release-triggered sample layers. It consumes all incoming noteOn and noteOff events (ignoring both), then on each noteOff it reconstructs a new noteOn from the stored original noteOn event, applies optional time-based velocity attenuation, and injects the new noteOn into the MIDI buffer as an artificial event.

MIDI noteOn in -> store velocity + timestamp per note number -> ignore original noteOn
MIDI noteOff in -> ignore original noteOff -> retrieve stored noteOn -> [optional: time attenuation via table lookup] -> scale velocity -> inject artificial noteOn -> MIDI out

## Gap Answers

### noteoff-to-noteon-logic

**Question:** How does ReleaseTrigger transform noteOff events into noteOn events?

**Answer:** In `onNoteOn()`, the incoming noteOn is immediately ignored via `Message.ignoreEvent(true)`, and the entire HiseEvent is stored in a per-note-number `messageHolders[128]` array. The uptime is also recorded in `lengthValues[noteNumber]`.

In `onNoteOff()`, the original noteOff is also ignored. The processor retrieves a copy of the stored noteOn event via `messageHolders[noteNumber]->getMessageCopy()`. It modifies this copy's velocity (with optional attenuation), clears its ignore flag (`ignoreEvent(false)`), sets its timestamp to the noteOff's timestamp, and injects it via `Synth.addMessageFromHolder()`. The `addMessageFromHolder` method marks the event as artificial, pushes it through `pushArtificialNoteOn`, and adds it to the MIDI buffer. Both the original noteOn and noteOff are consumed (ignored) - they do not pass through.

### velocity-source

**Question:** What velocity is assigned to the generated release noteOn?

**Answer:** The velocity source depends on MPE mode:
- **MPE disabled** (default): Uses the original noteOn velocity retrieved from `messageHolders[noteNumber]` via `onEvent.getVelocity()`
- **MPE enabled**: Uses the noteOff velocity via `Message.getVelocity()`

The chosen velocity is then multiplied by `attenuationLevel` (a float 0.0-1.0 from the table lookup, or 1.0 if time attenuation is disabled). The result is cast to int: `v = (int)((float)velocityToUse * attenuationLevel)`. If the resulting velocity is 0 or less, no noteOn is generated (the release is silently dropped).

### table-lookup-position

**Question:** Where in the signal path does the TableProcessor lookup occur?

**Answer:** The table lookup happens inside `onNoteOff()`, before the velocity is applied. The input to `table->getTableValue(timeIndex)` is the normalised elapsed time: `timeIndex = (elapsedTime / timeKnobValue)` clamped to 0-1. The `getTableValue()` method on ScriptTable delegates to `SampleLookupTable::getInterpolatedValue()` which accepts a normalised 0-1 input and returns a 0-1 output. The output is used directly as `attenuationLevel`, which is a velocity multiplier. So the table maps normalised hold time (0 = just pressed, 1 = held for Time seconds or longer) to a velocity scale factor (0 = silent, 1 = full velocity).

### time-attenuate-gating

**Question:** When TimeAttenuate is off, what happens?

**Answer:** When `enableButton->getValue() == 0` (TimeAttenuate off), the else branch sets `attenuationLevel = 1.0f`, meaning velocity is passed through at full strength with no attenuation. The table lookup is skipped entirely. The `onControl` callback also hides both the Time knob and TimeTable when the button is off (`timeKnob->showControl(value)` and `table->showControl(value)`), so the UI reflects that these controls are inactive.

### time-default-zero

**Question:** What happens when Time is 0?

**Answer:** When `timeKnob->getValue()` is 0, the division `time / 0.0` produces infinity (IEEE 754 double division). This is then passed to `jlimit<double>(0, 1, infinity)` which clamps it to 1.0. So with Time = 0, any non-zero hold duration immediately maps to `timeIndex = 1.0` (the rightmost table position). At the exact instant of noteOn followed by immediate noteOff (time = 0.0), we get `0.0 / 0.0 = NaN`, which `jlimit` clamps to 0.0 (the leftmost table position). In practice, Time = 0 means even the briefest key press reaches the end of the attenuation curve, making the attenuation maximally aggressive.

### timetable-parameter-type

**Question:** Is TimeTable a conventional parameter or a table data reference?

**Answer:** TimeTable is a `ScriptTable` component created via `Content.addTable("TimeTable", ...)` and registered at parent index 0 via `table->registerAtParent(0)`. It is not a conventional slider parameter - it is a table editor UI component. The "parameter" with range 0-1 and step 0 in the metadata is the table's internal connection index, not a user-facing value. In the UI it renders as a draggable curve editor. The `withComplexDataInterface(ExternalData::DataType::Table)` in `createMetadata()` correctly declares the Table interface.

### original-noteon-tracking

**Question:** How does ReleaseTrigger track noteOn-to-noteOff correspondence?

**Answer:** The processor maintains a fixed array of 128 `ScriptingMessageHolder` objects (`messageHolders`), indexed by MIDI note number (0-127). On each noteOn, the entire HiseEvent is stored at `messageHolders[noteNumber]` and the uptime is stored at `lengthValues[noteNumber]`. On noteOff, the stored event is retrieved by note number. This supports full polyphonic tracking - each of the 128 possible note numbers can independently store its noteOn data. However, overlapping notes on the same note number will overwrite - only the most recent noteOn for a given note number is retained.

## Processing Chain Detail

1. **NoteOn capture** (negligible): Store the incoming HiseEvent in `messageHolders[noteNumber]`, record uptime in `lengthValues[noteNumber]`, ignore the original event
2. **NoteOff processing** (negligible): Ignore original noteOff, compute elapsed hold time
3. **Time attenuation** (negligible, conditional): If TimeAttenuate enabled, normalise elapsed time by Time parameter, look up attenuation curve via `table->getTableValue()`, store as `attenuationLevel`. If disabled, set `attenuationLevel = 1.0`
4. **Velocity scaling** (negligible): Multiply source velocity by `attenuationLevel`, cast to int
5. **Velocity gate** (negligible): If velocity > 0, inject artificial noteOn via `Synth.addMessageFromHolder()`; if velocity == 0, drop silently
6. **Event injection** (negligible): `addMessageFromHolder` marks the event as artificial, registers it in the event handler, and adds it to the MIDI buffer with the noteOff's timestamp

## Conditional Behavior

- **TimeAttenuate = Off (0)**: Table lookup skipped, `attenuationLevel = 1.0`, velocity passed through at full strength. Time knob and table editor hidden in UI.
- **TimeAttenuate = On (1)**: Elapsed hold time normalised by Time parameter, looked up in table curve, result used as velocity multiplier.
- **MPE enabled**: Velocity source switches from original noteOn velocity to the noteOff velocity (`Message.getVelocity()`). MPE mode is tracked via `MPEData::Listener` callback.
- **Velocity = 0 after attenuation**: No noteOn is generated (silent drop). This naturally occurs when the table curve reaches 0 at certain time positions.

## Interface Usage

### TableProcessor (via ScriptTable)

The table is created as `Content.addTable("TimeTable", ...)` and registered at parent index 0. It provides the attenuation curve shape. Input domain: normalised time (0.0 = note just pressed, 1.0 = held for >= Time seconds). Output range: 0.0-1.0 velocity multiplier. The table is only consulted when TimeAttenuate is enabled. The default table curve (identity line from 0,0 to 1,1) would produce linear velocity decay over the Time window.

## CPU Assessment

- **Overall baseline**: negligible
- All processing is event-driven (per MIDI event, not per audio sample/block)
- The table lookup is a single interpolated read from a pre-computed lookup table
- No per-sample processing, no audio buffers, no DSP operations
- No parameters that scale cost

## UI Components

- Backend editor: Uses the default `HardcodedScriptProcessor` editor which renders the Content components - a toggle button (TimeAttenuate), a time knob (Time), and a table curve editor (TimeTable). The `onControl` callback shows/hides the Time and TimeTable controls based on the TimeAttenuate toggle state.

## Notes

- The module is a "hardcoded script processor" - a C++ class that mimics the ScriptProcessor callback API (`onNoteOn`, `onNoteOff`, `onControl`) without actually running HiseScript. It uses the same `Message`, `Synth`, `Engine`, and `Content` API objects.
- The `messageHolders` array is pre-allocated for all 128 note numbers in `onInit()`, avoiding runtime allocation.
- The Time parameter uses `HiSlider::Time` mode which displays with "ms" suffix, but the range is 0-20 with step 0.1 and the code operates in seconds (`Engine.getUptime()` returns seconds). The slider mode suffix is misleading - the parameter represents seconds, not milliseconds.
- The TimeTable parameter in the metadata (range 0-1, step 0) represents the table's complex data interface, not a conventional parameter value.
