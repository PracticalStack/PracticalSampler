Resynthesises audio from the analysed partial list, returning an array of Buffer objects with one buffer per audio channel. Each buffer contains the full resynthesised waveform at the original sample rate. The file must have been previously analysed.

If the partial list has been modified via `process()` or `processCustom()`, the resynthesis reflects those modifications. Call `process(file, "reset", {})` before resynthesising to get the original unmodified audio.
