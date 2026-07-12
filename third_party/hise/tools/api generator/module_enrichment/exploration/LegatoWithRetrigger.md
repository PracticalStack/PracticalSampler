# LegatoWithRetrigger - C++ Exploration

**Source:** `hi_scripting/scripting/HardcodedScriptProcessor.h` (lines 178-273)
**Base class:** `HardcodedScriptProcessor` (extends `ScriptBaseMidiProcessor`)
**Metadata:** `hi_scripting/scripting/HardcodedScriptProcessor.cpp` (lines 35-40)

## Signal Path

LegatoWithRetrigger is a pure event-transformation MIDI processor with zero parameters. It enforces monophonic behavior by intercepting noteOn/noteOff events and managing a single-depth retrigger stack. The base class `HardcodedScriptProcessor::processHiseEvent()` dispatches incoming `HiseEvent` objects to `onNoteOn()` and `onNoteOff()` callbacks.

MIDI in -> [make artificial] -> [kill previous if held] -> [store retrigger candidate] -> [track current note] -> MIDI out (monophonic)

On note release: MIDI in -> [ignore original noteOff] -> [kill current via event ID] -> [retrigger candidate if available] -> MIDI out

## Gap Answers

### legato-event-logic

**Question:** What is the full processHiseEvent() logic? When a new noteOn arrives while a note is already held, does it kill the old note or update pitch? What MIDI events are generated or suppressed?

**Answer:** The base class `processHiseEvent()` (HardcodedScriptProcessor.cpp:243-302) sets the Message object and dispatches by event type: NoteOn calls `onNoteOn()`, NoteOff calls `onNoteOff()`.

In `onNoteOn()` (line 204-222):
1. The incoming note is made artificial via `Message.makeArtificial()`, which assigns a new event ID and marks it as artificial. The new ID is captured.
2. If `lastNote != -1` (a note is already sounding), the module sends an artificial noteOff for the previous note via `Synth.noteOffByEventId(lastEventId)`, then stores the previous note number and channel as the retrigger candidate (`possibleRetriggerNote`, `possibleRetriggerChannel`).
3. The module updates its tracking state: `lastEventId` = new artificial ID, `lastNote` = incoming note number, `lastVelo` = incoming velocity, `lastChannel` = incoming channel.

The original noteOn event passes through (now with an artificial event ID). The previous note is killed. Only one note sounds at a time.

### retrigger-mechanism

**Question:** What does 'retrigger the previous note after a release' mean? How deep is the stack?

**Answer:** In `onNoteOff()` (line 224-258), when the released note matches `lastNote` (Block 3, line 242), the module checks if a retrigger candidate exists. If `possibleRetriggerNote != -1 && possibleRetriggerChannel != -1`, it sends a new artificial noteOn via `Synth.addNoteOn(possibleRetriggerChannel, possibleRetriggerNote, lastVelo, 0)`, captures the new event ID, updates `lastNote`/`lastChannel` to the retriggered note, and clears the retrigger candidate.

**Stack depth is exactly 1.** Only one retrigger candidate is stored. If notes A, B, C are played in legato sequence: when C arrives, B becomes the retrigger candidate and A is forgotten. Releasing C retriggers B. Releasing B then produces silence (A was never stored).

### note-tracking-state

**Question:** What internal state does the module maintain? What data is stored per note?

**Answer:** Six member variables (lines 266-271):

| Variable | Type | Init | Purpose |
|----------|------|------|---------|
| `lastNote` | int | -1 | Note number of currently sounding note (-1 = none) |
| `lastEventId` | int | -1 | Artificial event ID of current note (for noteOff pairing) |
| `lastChannel` | int | (uninit) | MIDI channel of current note |
| `lastVelo` | int | 0 | Velocity of the most recently played note |
| `possibleRetriggerNote` | int | -1 | Note number of retrigger candidate (-1 = none) |
| `possibleRetriggerChannel` | int | -1 | MIDI channel of retrigger candidate |

Notable: the retrigger candidate does NOT store its own velocity or event ID. Only note number and channel are preserved.

### velocity-handling

**Question:** When retriggering, what velocity is used? When performing legato, is the new note's velocity preserved?

**Answer:** The retrigger uses `lastVelo`, which is the velocity of the **most recently played** note, NOT the original velocity of the retriggered note. This is because `lastVelo` is overwritten on every `onNoteOn()` call (line 220: `lastVelo = Message.getVelocity()`), and the retrigger candidate does not store its own velocity.

Example: Play A at velocity 100, then B at velocity 50 (legato). Release B - A is retriggered at velocity 50 (B's velocity), not 100.

For legato transitions, the new note's velocity is preserved as-is (the original noteOn passes through with its artificial event ID).

### channel-handling

**Question:** Does the module operate on a specific MIDI channel or all channels?

**Answer:** The module processes all MIDI channels. It stores the channel of the current note (`lastChannel`) and the retrigger candidate (`possibleRetriggerChannel`). The retrigger preserves the original channel of the candidate note via `Synth.addNoteOn(possibleRetriggerChannel, ...)`.

Channel is used in matching: `onNoteOff()` Block 1 (line 226) checks both note number AND channel to identify the currently sounding note. Block 2 (line 236) checks both to identify the retrigger candidate. However, Block 3 (line 242) only checks note number - see Vestigial/Notable section for this inconsistency.

### event-id-management

**Question:** How does the module handle HISE artificial event IDs?

**Answer:** Every incoming noteOn is made artificial via `Message.makeArtificial()` (line 206), which assigns a new unique event ID. This ID is stored in `lastEventId` for later noteOff pairing.

NoteOffs for the currently sounding note are managed exclusively through `Synth.noteOffByEventId(lastEventId)` - the original noteOff is ignored via `Message.ignoreEvent(true)` (line 228). This ensures correct pairing regardless of note number collisions.

Retriggered notes get new event IDs from `Synth.addNoteOn()` (line 246), which returns the event ID of the newly created artificial noteOn. This ID becomes the new `lastEventId`.

The pattern is: every note transition produces a properly paired artificial noteOff (for the departing note) followed by either the original noteOn (for legato) or a new artificial noteOn (for retrigger).

### edge-case-all-released

**Question:** What happens when the last note in the stack is released?

**Answer:** When the released note matches `lastNote` and no retrigger candidate exists (`possibleRetriggerNote == -1`), Block 3's else branch (line 254-256) simply sets `lastNote = -1`. The artificial noteOff from Block 1 (`Synth.noteOffByEventId(lastEventId)`) stops the note. No retrigger occurs. The module returns to its initial idle state.

Additionally, if the retrigger candidate itself is released while a different note is still sounding (e.g., play A then B legato, then release A), Block 2 (line 236-240) clears the retrigger candidate. B continues sounding but there is nothing to retrigger when B is later released.

### description-accuracy

**Question:** Is the description "useful for lead lines and expressive legato phrasing" accurate? Is monophonic enforced?

**Answer:** The description is accurate. Monophonic behavior IS enforced - when a new note arrives while one is held, the previous note is always killed via `Synth.noteOffByEventId()`. Only one note sounds at any time.

The characterization as "useful for lead lines" is apt - the single-depth retrigger stack matches classic monosynth behavior where releasing a legato note returns to the previous pitch. However, it's worth noting the 1-deep limitation: unlike some monosynth implementations that maintain a full note priority stack, this module only remembers one previous note.

## Processing Chain Detail

1. **Event dispatch** (negligible): Base class `processHiseEvent()` routes HiseEvent to `onNoteOn()` or `onNoteOff()` based on event type
2. **Artificial ID assignment** (negligible): `Message.makeArtificial()` on every incoming noteOn
3. **Previous note kill** (negligible): `Synth.noteOffByEventId()` if a note is already held
4. **Retrigger candidate storage** (negligible): Store previous note number and channel
5. **State update** (negligible): Update lastNote, lastEventId, lastVelo, lastChannel
6. **NoteOff handling** (negligible): Ignore original noteOff, send artificial noteOff by event ID, conditionally retrigger

All stages are pure event manipulation with no DSP computation.

## Conditional Behavior

- **No note held** (`lastNote == -1`): NoteOn passes through with artificial ID, no kill/retrigger. Simple initialization.
- **Note held** (`lastNote != -1`): NoteOn kills previous note, stores it as retrigger candidate, new note becomes active.
- **NoteOff matches active note with retrigger available**: Active note killed, retrigger candidate re-sent as new noteOn.
- **NoteOff matches active note, no retrigger**: Active note killed, module goes idle.
- **NoteOff matches retrigger candidate** (not the active note): Retrigger candidate cleared. Active note continues.
- **NoteOff matches neither**: Event passes through unmodified (no-op for this module).

## Vestigial / Notable

**Channel check inconsistency in onNoteOff():** Block 1 (line 226) checks both note number and channel (`Message.getNoteNumber() == lastNote && Message.getChannel() == lastChannel`), but Block 3 (line 242) only checks note number (`number == lastNote`). In multi-channel scenarios, a noteOff for the correct note number on a different channel would skip Block 1 (no event suppression, no artificial noteOff sent) but enter Block 3 (attempt retrigger or clear state). This could cause state corruption in MPE or multi-channel configurations, though single-channel usage is unaffected.

**Retrigger velocity substitution:** The retrigger always uses `lastVelo` (the most recently played note's velocity), not the original velocity of the retriggered note. This is a deliberate design choice but may surprise users expecting the original dynamics to be preserved.

**`lastChannel` not initialized in `onInit()`:** The `lastChannel` member is not set in `onInit()` (lines 195-202), though it is only read after being set by the first `onNoteOn()` call, so this has no practical impact.

## CPU Assessment

- **Overall baseline:** negligible
- All processing is pure event manipulation (integer comparisons, event ID management)
- No per-sample computation, no buffers, no DSP
- Cost is per-event, not per-block, and each event involves at most a few integer comparisons and 1-2 artificial event operations

## Notes

- This is a `HardcodedScriptProcessor` - a C++ class that uses the same scripting API objects (`Message`, `Synth`) as HISEScript but compiled as native code. The `onNoteOn()`/`onNoteOff()` callbacks mirror the scripting callbacks exactly.
- The module has no `createEditor()` override, so it uses the default HardcodedScriptProcessor editor (no custom UI).
- The single-depth retrigger stack is a common simplification. Full note-priority stacks (low/high/last note priority with arbitrary depth) would require a more complex data structure. This module implements "last note priority with 1-deep retrigger" specifically.
- Compare with other parameterless MIDI processors like TransposerMidiProcessor - LegatoWithRetrigger is similarly minimal but with more complex state management.
