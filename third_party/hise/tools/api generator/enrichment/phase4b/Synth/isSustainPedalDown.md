Synth::isSustainPedalDown() -> Integer

Thread safety: SAFE -- reads a plain boolean member variable.
Returns true if the sustain pedal (MIDI CC #64) is currently pressed. State is updated
by the host script processor before onController fires. Not affected by script-generated
controller events (sendController/addController with CC 64).

Source:
  ScriptingApi.cpp  Synth::isSustainPedalDown()
    -> sustainState (set via setSustainPedal())
