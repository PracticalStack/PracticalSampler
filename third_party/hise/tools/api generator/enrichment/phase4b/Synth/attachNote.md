Synth::attachNote(Integer originalNoteId, Integer artificialNoteId) -> Integer

Thread safety: SAFE -- delegates to MidiProcessorChain::attachNote which inserts into a pre-allocated buffer, no allocations, no locks.
Links an artificial note to a real note so that releasing the original automatically stops the
artificial one. Returns true if successful. Requires setFixNoteOnAfterNoteOff(true) first.

Required setup:
  Synth.setFixNoteOnAfterNoteOff(true); // in onInit

Pair with:
  setFixNoteOnAfterNoteOff -- must enable the attached note buffer first
  playNote / addNoteOn -- create the artificial note whose ID you pass as artificialNoteId

Anti-patterns:
  - Do NOT call without first calling setFixNoteOnAfterNoteOff(true) -- throws a script error
    about the attached note buffer not being enabled.
  - If parentMidiProcessor is null (non-MIDI processor), silently returns false with no error.

Source:
  ScriptingApi.cpp  Synth::attachNote()
    -> parentMidiProcessor->getOwnerSynth()->getMainController()
       ->getEventHandler().getMidiProcessorChain()->attachNote(original, artificial)
