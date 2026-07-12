Sets the time signature and bar count of the current sequence from a JSON object. See `File.loadMidiMetadata` for the object format. Only `Nominator`, `Denominator`, and `NumBars` are required; `LoopStart` and `LoopEnd` are optional (normalised 0.0 to 1.0). Returns true if the values are valid.

> [!Warning:Tempo property is read-only] The `Tempo` property returned by `getTimeSignature()` is read-only. Including it in the object passed to this method has no effect - tempo is always derived from the host or master clock.
