Synth::noteOffFromUI(Integer channel, Integer noteNumber) -> undefined

Thread safety: UNSAFE -- calls CustomKeyboardState::injectMessage() which involves internal locking on the keyboard state's critical section.
Injects a MIDI note-off through the virtual keyboard input pipeline, as if releasing a key on the
on-screen keyboard. Routes through CustomKeyboardState, NOT the MIDI processor buffer.

Pair with:
  playNoteFromUI -- the corresponding note-on method for UI-driven note control

Anti-patterns:
  - Do NOT mix with playNote/addNoteOn -- noteOffFromUI matches by channel+note number
    through the standard MIDI pipeline. It cannot stop artificial events started with playNote.
  - Events are NOT marked artificial -- they update keyboard state (isKeyDown, getNumPressedKeys).

Source:
  ScriptingApi.cpp  Synth::noteOffFromUI()
    -> CustomKeyboardState::injectMessage(MidiMessage::noteOff(channel, noteNumber))
