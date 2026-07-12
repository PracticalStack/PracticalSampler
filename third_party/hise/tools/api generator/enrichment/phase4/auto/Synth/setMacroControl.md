Sets one of the eight macro controllers to a new value. The `macroIndex` is 1-based (1 to 8) and `newValue` uses a 0-127 range, consistent with MIDI CC scaling. The value does not need to be an integer. This method only works when the parent synth is a `ModulatorSynthChain` - calling it from a script inside a non-chain synth produces an error.

> [!Warning:Avoid recursive macro control loops] Avoid calling this from a control callback that is itself connected to the same macro control, or you will create a recursive loop that freezes the system.
