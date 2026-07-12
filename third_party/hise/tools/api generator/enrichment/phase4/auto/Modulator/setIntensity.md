Sets the modulation depth. The valid range depends on the parent chain's modulation mode - 0.0 to 1.0 for GainMode, -12.0 to 12.0 (semitones) for PitchMode, and -1.0 to 1.0 for PanMode, GlobalMode, OffsetMode, and CombinedMode. Values outside the valid range are clamped automatically.

> [!Warning:Pitch intensity is in semitones] For pitch-mode modulators, the default intensity of 1.0 gives only 1 semitone of range. Set `setIntensity(12.0)` for a full octave, or the desired semitone value, immediately after creating a pitch modulation connection.
