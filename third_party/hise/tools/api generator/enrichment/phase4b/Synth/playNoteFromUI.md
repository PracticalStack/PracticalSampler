Synth::playNoteFromUI(Integer channel, Integer noteNumber, Integer velocity) -> undefined

Thread safety: UNSAFE -- calls CustomKeyboardState::injectMessage() which involves internal locking on the keyboard state's critical section.
Injects a MIDI note-on through the virtual keyboard input pipeline, as if pressing a key on the
on-screen keyboard. Routes through CustomKeyboardState, NOT the MIDI processor buffer.
No event ID is returned -- use noteOffFromUI to release.

Pair with:
  noteOffFromUI -- the corresponding note-off method for UI-driven note control

Anti-patterns:
  - Do NOT use noteOffByEventId to stop notes from playNoteFromUI -- no event ID is returned.
    Use noteOffFromUI with the same channel and note number.
  - Events are NOT marked artificial -- they update keyboard state (isKeyDown, getNumPressedKeys).

Source:
  ScriptingApi.cpp  Synth::playNoteFromUI()
    -> CustomKeyboardState::injectMessage(MidiMessage::noteOn(channel, noteNumber, (uint8)velocity))
