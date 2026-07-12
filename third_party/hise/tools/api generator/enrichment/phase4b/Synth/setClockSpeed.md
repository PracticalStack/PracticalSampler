Synth::setClockSpeed(Integer clockSpeed) -> undefined

Thread safety: SAFE -- writes a single enum value to the owner synth's clockSpeed member, no allocations.
Sets the internal clock speed for MIDI clock generation. Valid values: 0 (inactive), 1 (bar),
2 (half), 4 (quarter), 8 (eighth), 16 (sixteenth), 32 (thirty-second).

Anti-patterns:
  - Error message says "Use 1,2,4,8,16 or 32" but omits 0 (Inactive) which is valid.

Source:
  ScriptingApi.cpp  Synth::setClockSpeed()
    -> switch(clockSpeed): maps script values to ModulatorSynth::ClockSpeed enum
    -> owner->setClockSpeed(enumValue)
