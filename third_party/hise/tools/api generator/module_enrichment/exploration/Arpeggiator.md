# Arpeggiator - C++ Exploration

**Source:** `hi_scripting/scripting/hardcoded_modules/Arpeggiator.h`, `Arpeggiator.cpp`
**Base class:** `HardcodedScriptProcessor` (a C++ class that emulates script processor callbacks using the scripting API internally)

## Signal Path

The Arpeggiator is a MIDI event transformer. It captures incoming note-on/note-off events into a held-note list, then emits new MIDI note events on a tempo-synced timer. Incoming notes are consumed (ignored) by default; output notes are generated synthetically.

MIDI note-on -> add to held-note list -> [first note starts timer] -> onTimer fires -> playNote() -> build sequence from held keys + octave expansion -> select note by direction mode -> apply slider pack data (semitone offset, velocity, length) -> send artificial note-on + scheduled note-off -> increment step -> restart timer with shuffle

When all keys are released, the arpeggiator resets and stops the timer.

## Gap Answers

### event-processing-logic

**Question:** What is the core event processing logic in processHiseEvent()?

**Answer:** The Arpeggiator uses HardcodedScriptProcessor callbacks rather than a direct `processHiseEvent()` override. The flow is:

1. **`onNoteOn()`** (Arpeggiator.cpp:409-455): If not bypassed and channel passes filter, the incoming note is consumed via `Message.ignoreEvent(true)`. The note is added to `userHeldKeysArray` (insertion order) and `userHeldKeysArraySorted` (pitch-sorted). If the arp is not already playing, `playNote()` is called to start. In Chord mode, if the arp is already playing and within 20ms of the chord start, the new note is immediately played alongside existing chord notes.

2. **`onNoteOff()`** (Arpeggiator.cpp:457-479): The note is removed from held arrays (unless sustain hold is active, in which case it's kept and added to `sustainHoldKeys`). If no keys remain held, `reset(false, true)` stops the arp.

3. **`onTimer()`** (Arpeggiator.cpp:602-612): If keys are held, calls `playNote()`. This is the tempo-synced clock that drives the sequence.

4. **`playNote()`** (Arpeggiator.cpp:615-764): The core method that:
   - Calculates the time interval from host BPM and Tempo parameter via `TempoSyncer::getTempoInMilliSeconds()`
   - Restarts the timer with shuffle factor applied
   - Builds `MidiSequenceArray` from held keys expanded across octave range
   - Selects the current note index based on direction mode
   - Reads per-step semitone offset, velocity, and note length from slider packs
   - Sends note-on via `Synth.addNoteOn()` and schedules note-off via `Synth.noteOffDelayedByEventId()`
   - Handles tie-note logic (extends note-off into next step if length = 100%)
   - Handles skip-step logic (no note if length = 0%)
   - Increments `curHeldNoteIdx` (note traversal) and `currentStep` (slider pack position) separately

Note management uses `Synth.addNoteOn()` which creates artificial MIDI events with unique event IDs, and `Synth.noteOffDelayedByEventId()` for sample-accurate note-off scheduling.

### slider-pack-usage

**Question:** How does the SliderPackProcessor interface feed into the arpeggiator?

**Answer:** There are three slider packs, registered as complex data objects at parent indices 0, 1, 2:

1. **SemiToneSliderPack** (index 0): Semitone offset per step. Range -24 to +24, default 0. Applied in `playNote()` line 680: `currentNote += (int8)semiToneSliderPack->getSliderValueAt(currentStep)`. Transposes the selected held note by the step's semitone value.

2. **VelocitySliderPack** (index 1): Velocity per step. Range 1 to 127, default 127. Read in `playNote()` line 681: `currentVelocity = (int)velocitySliderPack->getSliderValueAt(currentStep)`. Used as the velocity for `Synth.addNoteOn()`.

3. **LengthSliderPack** (index 2): Note length as percentage of the step duration. Range 0 to 100, default 75. Read in `playNote()` line 682: `currentNoteLengthInSamples = (int)(Engine.getSamplesForMilliSeconds(timeInterval * 1000.0) * lengthSliderPack->getSliderValueAt(currentStep) / 100.0)`. Special values: 0% = skip step (no note played), 100% = tie to next step (when EnableTieNotes is on).

All three slider packs resize to match `NumSteps` when that parameter changes (Arpeggiator.cpp:487-489).

### missing-parameters-12-15

**Question:** Parameter indices 12-15 are missing from the base data. What are these parameters?

**Answer:** These indices correspond to UI-only components created in `onInit()` that are not controllable parameters:

- Index 12: `packBg` - a `ScriptPanel` used as a visual background behind the slider packs
- Index 13: `semiToneSliderPack` - exposed as complex data (SliderPack) index 0, not a scalar parameter
- Index 14: `velocitySliderPack` - exposed as complex data (SliderPack) index 1
- Index 15: `lengthSliderPack` - exposed as complex data (SliderPack) index 2

These are correctly omitted from the parameter list. The slider packs are accessed through the `SliderPackProcessor` complex data interface, not through `getAttribute`/`setAttribute`. The metadata registration (Arpeggiator.cpp:106-131) jumps from `Par(11)` (EnableTieNotes) to `Par(16)` (InputChannel), confirming these indices are intentionally skipped.

### direction-modes

**Question:** How does each Direction mode change the note traversal algorithm?

**Answer:** The Direction parameter controls `arpDirMod` (increment direction) and `randomOrder` flag. The traversal operates on the expanded `MidiSequenceArray` (held keys + octave copies). In `changeDirection()` (Arpeggiator.cpp:856-883) and `playNote()`:

1. **Up (1):** `arpDirMod = 1`. Traverses notes from lowest to highest, wrapping around.

2. **Down (2):** `arpDirMod = -1`. Traverses from highest to lowest, wrapping around.

3. **Up-Down (3):** Starts going up. When `curHeldNoteIdx` reaches the top of the array, `arpDirMod` flips to -1. When it reaches the bottom, flips back to +1. Requires more than 1 note to oscillate.

4. **Down-Up (4):** Starts going down (`arpDirMod = -1`, `curHeldNoteIdx` starts at end of array). Same boundary-reversal logic as Up-Down but starting in the opposite direction.

5. **Random (5):** `randomOrder = true`. Each step picks a random index via `r.nextInt(MidiSequenceArraySorted.size())`. Avoids repeating the same note if more than 2 notes are available (re-rolls until different).

6. **Chord (6):** `arpDirMod = 1`. In `sendNoteOn()` (Arpeggiator.cpp:781-814), instead of sending one note, iterates over ALL notes in `MidiSequenceArraySorted` and sends a note-on for each, applying the current step's semitone offset to all. All notes share the same velocity and note length. New notes arriving within 20ms of the chord start are immediately played alongside (Arpeggiator.cpp:435-444).

Note: When `SortKeys` is enabled, the traversal uses `MidiSequenceArraySorted` (pitch-sorted) instead of `MidiSequenceArray` (insertion order). This affects Up, Down, Up-Down, and Down-Up modes.

### stride-behavior

**Question:** How does Stride interact with Direction?

**Answer:** Stride controls the **step index increment** through the slider packs, NOT the held-note traversal. In `playNote()` line 760: `currentStep = incAndWrapValueFromZeroToMax((int)stepSkipSlider->getValue(), currentStep, numStepSlider->getValue())`.

- Stride=1 (default): Steps through slider pack positions sequentially (0, 1, 2, 3, ...)
- Stride=2: Skips every other slider pack step (0, 2, 4, 6, ...)
- Stride=-1: Goes backwards through slider pack steps (0, 3, 2, 1, 0, ...)
- Stride=0: Stays on the same slider pack step forever

The held-note traversal (`curHeldNoteIdx`) is controlled independently by `arpDirMod` (set by Direction). So Stride and Direction are orthogonal: Direction controls which note from the held-key list is played, Stride controls which slider pack step provides the semitone offset, velocity, and length.

The wrapping uses modular arithmetic: `(((value + increment) % m) + m) % m` which correctly handles negative values.

### step-reset-behavior

**Question:** How does StepReset work?

**Answer:** StepReset controls a master step counter (`curMasterStep`) that triggers a full sequence reset. In `playNote()` lines 668-677:

- **StepReset = 0 (default):** Feature is disabled. `curMasterStep` is kept at 0 and never increments.
- **StepReset > 0:** `curMasterStep` increments each step. When `curMasterStep >= stepResetAmount`, `reset(false, false)` is called, which resets `currentStep` to 0, `curMasterStep` to 0, and resets the direction to its starting state (e.g., Up-Down resets to Up). It does NOT send all-notes-off or stop the timer, so playback continues seamlessly from the reset point.

So StepReset=8 means the sequence resets every 8 steps regardless of NumSteps.

### octave-range-traversal

**Question:** How does OctaveRange extend the sequence?

**Answer:** In `playNote()` lines 624-635, the `MidiSequenceArray` is built by iterating over octave copies:

```
octaveAmount = abs(octaveRaw) + 1
for i in 0..octaveAmount:
    for each held key:
        add key + (octaveSign * i * 12) to MidiSequenceArray
```

- **OctaveRange = 0 (default):** `octaveAmount = 1`, only the original notes. No transposition.
- **OctaveRange = 2:** `octaveAmount = 3`, adds original notes, then +12 semitones, then +24 semitones. The sequence is: [C3, E3, G3, C4, E4, G4, C5, E5, G5] (appended, not interleaved).
- **OctaveRange = -1:** `octaveAmount = 2`, `octaveSign = -1`. Adds original notes, then -12 semitones. Sequence: [C3, E3, G3, C2, E2, G2].

The octave copies are appended after the base note cycle. The total sequence length is `heldNotes * (abs(octaveRange) + 1)`. Direction modes traverse this expanded array, so Up mode goes through all base notes, then all +1 octave notes, etc.

### shuffle-implementation

**Question:** How is Shuffle implemented?

**Answer:** In `start()` (Arpeggiator.cpp:967-977):

```
shuffleAmount = 0.5 * shuffleSlider->getValue()
shuffleFactor = shuffleNextNote ? (1.0 - shuffleAmount) : (1.0 + shuffleAmount)
shuffleNextNote = !shuffleNextNote
Synth.startTimer(timeInterval * shuffleFactor)
```

This is standard swing timing. The `shuffleNextNote` flag alternates every step:
- Even steps (downbeats): timer = `timeInterval * (1 + shuffle * 0.5)` - longer
- Odd steps (upbeats): timer = `timeInterval * (1 - shuffle * 0.5)` - shorter

At Shuffle = 0: no swing (both factors = 1.0). At Shuffle = 1.0: maximum swing (factors alternate between 1.5x and 0.5x, a 3:1 ratio). At Shuffle = 0.66: triplet feel (factors ~1.33x and ~0.67x, a 2:1 ratio).

The shuffle resets to false (even step) when the arp stops.

### tie-notes-behavior

**Question:** How does EnableTieNotes work?

**Answer:** Tie behavior is checked via `curr_step_is_tied()` (Arpeggiator.h:278-281): returns true when `enableTieNotes->getValue()` is true AND the current step's length slider pack value equals exactly 100.0 (maximum). Two code paths handle ties:

**Multi-note sequence (> 1 note in MidiSequenceArray)** (lines 687-712):
- A note-on is always sent for the current step
- If the current step is tied AND the next step won't be skipped, `minNoteLenSamples` (~12ms at 44100 Hz) is added to the note length, causing a brief overlap with the next step's note. This engages the synth's legato handling.
- The note-off is always scheduled; there is no true note sustain across steps

**Single-note sequence (exactly 1 note)** (lines 714-747):
- Uses a "hold note" approach. If `last_step_was_tied` is true, no new note-on is sent (the previous note continues).
- If the current step is tied, `last_step_was_tied` is set true and no note-off is scheduled.
- If the current step is NOT tied, `stopCurrentNote()` schedules the note-off.
- If the next step will be skipped, tie state is cleared and the note-off is scheduled.

So for single-note sequences, ties create true sustained notes. For multi-note sequences, ties create brief overlaps for legato effect.

### hold-sustain-interaction

**Question:** How does the Hold parameter interact with the sustain pedal?

**Answer:** The `Hold` parameter (index 20) is a toggle button (`sustainHold`). It is NOT automatically linked to CC64 (sustain pedal). It's a UI toggle that can be manually clicked or MIDI-learned to any CC via HISE's MIDI learn system.

When Hold is toggled via `onControl()` (Arpeggiator.cpp:525-550):

- **Activating Hold** (`sustainHoldActive = true`): Subsequent note-off events in `remUserHeldKey()` (line 910-921) do NOT remove notes from the held arrays. Instead, the released note is added to `sustainHoldKeys` for tracking. The arpeggiator continues playing as if all keys are still held.

- **Deactivating Hold** (`sustainHoldActive = false`): All notes in `sustainHoldKeys` are removed from `userHeldKeysArray` and `userHeldKeysArraySorted`. If no real keys remain held, the arp resets and stops.

- **Re-playing a sustained note**: If a note that's being sustained is played again (`addUserHeldKey`, line 902-903), it's removed from `sustainHoldKeys` first, so it becomes a "real" held note again.

### tempo-sync-values

**Question:** What are the tempo sync note value mappings?

**Answer:** The Tempo parameter uses `TempoSyncer::Tempo` enum values. Without `HISE_USE_EXTENDED_TEMPO_VALUES` (standard build):

| Index | Enum | Note Value |
|-------|------|-----------|
| 0 | Whole | 1/1 |
| 1 | HalfDuet | 1/2D (dotted) |
| 2 | Half | 1/2 |
| 3 | HalfTriplet | 1/2T |
| 4 | QuarterDuet | 1/4D |
| 5 | Quarter | 1/4 |
| 6 | QuarterTriplet | 1/4T |
| 7 | EighthDuet | 1/8D |
| 8 | Eighth | 1/8 |
| 9 | EighthTriplet | 1/8T |
| 10 | SixteenthDuet | 1/16D |
| 11 | Sixteenth | 1/16 |
| 12 | SixteenthTriplet | 1/16T |
| 13 | ThirtyTwoDuet | 1/32D |
| 14 | ThirtyTwo | 1/32 |
| 15 | ThirtyTwoTriplet | 1/32T |
| 16 | SixtyForthDuet | 1/64D |
| 17 | SixtyForth | 1/64 |
| 18 | SixtyForthTriplet | 1/64T |

Default = 11 (Sixteenth / 1/16). With `HISE_USE_EXTENDED_TEMPO_VALUES`, 5 additional entries are prepended (EightBar through TwoBars), shifting all indices up by 5.

The actual timing is computed via `TempoSyncer::getTempoInMilliSeconds(BPM, tempo)` and clamped to a minimum of 40ms (`minTimerTime = 0.04`).

### mpe-channel-handling

**Question:** How do MPEStartChannel and MPEEndChannel affect behavior?

**Answer:** MPE mode is activated globally (not by these parameters) via the `MidiControllerAutomationHandler::MPEData::Listener` interface. When MPE mode is active:

**Input filtering** (`shouldFilterMessage()`, Arpeggiator.h:217-230): Channel 1 (MPE master channel) always passes. Other channels are accepted only if they fall within the `[mpeStart, mpeEnd]` range. This filters which MPE zones the arpeggiator responds to.

**MPE data capture** (`onController()`, Arpeggiator.cpp:570-591): Per-channel MPE gesture data is stored: stroke velocity, pressure (aftertouch), slide (CC74), glide (pitch bend), and lift velocity.

**Output** (`sendNoteOnInternal()`, Arpeggiator.cpp:820-849): In MPE mode, output notes use the original channel from the held note (not `midiChannel`). After sending the note-on, the stored MPE values (pressure, slide, glide) for that channel are sent as additional MIDI events with matching timestamps.

**Channel validation** (`onControl()`, Arpeggiator.cpp:551-561): If either MPEStartChannel or MPEEndChannel is set to 1 ("Inactive"), both are reset to defaults (2 and 16).

### current-step-read-only

**Question:** Is CurrentStep truly read-only?

**Answer:** No, CurrentStep is **read-write**. In `onControl()` (Arpeggiator.cpp:563-568), setting CurrentStep externally updates both `currentStep` and `curMasterStep`:

```cpp
currentStep = jlimit<int>(0, velocitySliderPack->getNumSliders()-1, (int)value);
curMasterStep = currentStep;
```

This allows external sequence control via scripting or MIDI learn. The value is updated per-step in `playNote()` line 757: `currentStepSlider->setValue(currentStep + 1)` (note: 1-indexed for display). The slider UI is configured as disabled (`enabled = false`) and non-saving (`saveInPreset = false`), suggesting it's intended primarily as a display, but the underlying parameter is fully writable.

## Processing Chain Detail

1. **Channel filter** (negligible): Check incoming MIDI channel against InputChannel filter or MPE zone range. Reject non-matching events.

2. **Note capture** (negligible): Add/remove notes from `userHeldKeysArray` (insertion order) and `userHeldKeysArraySorted` (pitch order). Incoming note events are consumed via `Message.ignoreEvent(true)`.

3. **Tempo calculation** (negligible, per-step): `calcTimeInterval()` reads host BPM, converts the Tempo parameter to milliseconds via `TempoSyncer::getTempoInMilliSeconds()`, clamps to minimum 40ms.

4. **Timer start with shuffle** (negligible): `start()` alternates the timer interval by shuffle factor. Even steps are longer, odd steps are shorter.

5. **Sequence building** (negligible-low, per-step): Builds `MidiSequenceArray` from held keys expanded by OctaveRange. For N held keys and octave range R, produces `N * (|R| + 1)` entries.

6. **Note index selection** (negligible): Selects `curHeldNoteIdx` based on Direction mode (linear traversal, boundary reversal, or random).

7. **Step reset check** (negligible): If StepReset > 0 and master step count reached, reset step counters and direction.

8. **Slider pack read** (negligible): Read semitone offset, velocity, and note length for `currentStep` from the three slider packs.

9. **Note generation** (negligible): `Synth.addNoteOn()` creates artificial note-on event. `Synth.noteOffDelayedByEventId()` schedules note-off. In Chord mode, one note-on per held key. In MPE mode, additional CC/pitch-bend events are sent.

10. **Step increment** (negligible): Advance `currentStep` by Stride amount (wrapping), advance `curHeldNoteIdx` by `arpDirMod`.

## Conditional Behavior

- **Bypass = On**: All callbacks return early. No notes captured or generated. On bypass deactivation (`bypassStateChanged`), held keys are cleared and the arp resets with all-notes-off.
- **Direction = Chord**: All held notes play simultaneously each step instead of one note per step. Notes arriving within 20ms of chord start are added live.
- **Direction = Random**: Note selection is randomized instead of sequential. Avoids immediate repeats when > 2 notes available.
- **Direction = Up-Down / Down-Up**: Direction reverses at array boundaries.
- **SortKeys = On**: Note traversal uses pitch-sorted array instead of insertion-order array.
- **EnableTieNotes = On + Length = 100%**: Note extends into next step. Multi-note: brief overlap for legato. Single-note: true sustain (no note-off).
- **Length = 0%**: Step is skipped entirely (no note played). Checked via `curr_step_is_skip()`.
- **Hold = On**: Note-off events don't remove notes from held array. Arp continues after all keys released.
- **StepReset = 0**: Master step counter disabled. StepReset > 0: sequence resets after N master steps.
- **Stride = 0**: Slider pack step never changes (same semitone/velocity/length every step).
- **MPE mode active**: Channel filtering uses MPE zone range. Output notes carry per-channel MPE gesture data.
- **OutputChannel = "Use input channel"**: Output notes use the channel of the held key. Otherwise forced to selected channel.
- **Single note held**: Uses "hold note" logic with true tie sustain instead of overlap-based legato.

## Interface Usage

### SliderPackProcessor

Three slider packs are registered as complex data objects:

- **Index 0 (SemiToneSliderPack):** Read per step in `playNote()` to transpose the selected note by -24 to +24 semitones.
- **Index 1 (VelocitySliderPack):** Read per step in `playNote()` to set the output velocity (1-127).
- **Index 2 (LengthSliderPack):** Read per step in `playNote()` to calculate note duration as percentage of step interval. Also used by `curr_step_is_tied()` (value == 100) and `curr_step_is_skip()` (value == 0) for tie/skip logic.

All three packs resize when NumSteps changes. They are accessed via `getSliderValueAt(currentStep)` for display-updating reads, and via the direct `getSliderValueWithoutDisplay()` static helper for tie/skip checks (avoids triggering UI updates on the audio thread).

## Vestigial / Notable

- `applySliderPackData(NoteWithChannel& c)` (Arpeggiator.cpp:851-854) is declared and defined but the method body is empty. It appears to be a leftover from an earlier design where slider pack data was applied differently. Not a parameter issue.

## CPU Assessment

- **Overall baseline:** negligible. The arpeggiator processes discrete MIDI events on a timer; there is no per-sample audio processing.
- **Per-step cost:** negligible. Each timer tick does array operations (sequence building, index selection), slider pack reads, and MIDI event generation. All O(N) where N is held keys * octave range, bounded at 256.
- **No scaling factors:** CPU cost is constant regardless of tempo or number of steps. Sequence array size scales with held keys and octave range but is trivially small.

## Notes

- The Arpeggiator is a `HardcodedScriptProcessor`, meaning it was originally developed as a HISEScript and later converted to C++. It uses scripting API calls internally (`Synth.addNoteOn`, `Engine.getSampleRate`, `Content.addButton`, etc.) rather than direct C++ DSP APIs.
- The code is credited as "based on a script by Elan Hickler" (header comment).
- The `curHeldNoteIdx` (which note from the held-key list) and `currentStep` (which slider pack step) advance independently. This means the note sequence and the slider pack pattern can have different lengths and cycle at different rates, creating polyrhythmic patterns.
- In Chord mode, `MidiSequenceArraySorted` is used (not `MidiSequenceArray`), so chord notes are always pitch-sorted regardless of the SortKeys setting.
- The minimum timer time of 40ms (`minTimerTime`) limits the maximum arpeggiator speed to approximately 25 steps per second regardless of host tempo.
