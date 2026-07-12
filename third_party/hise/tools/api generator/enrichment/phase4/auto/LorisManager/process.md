Applies a predefined processing command to the analysed partial list. The data parameter is a JSON object or array whose structure depends on the command. The file must have been previously analysed. Processing modifies the cached partial list in place - use `process(file, "reset", {})` to revert to the original analysis state.

| Command | Data format | Effect |
|---------|-------------|--------|
| `"reset"` | `{}` | Reverts partials to the original analysis state |
| `"shiftTime"` | `{"offset": number}` | Shifts all partial times by a constant offset (in the current time domain) |
| `"shiftPitch"` | `{"offset": number}` or `[[time, value], ...]` | Shifts pitch by a constant (cents) or an envelope |
| `"scaleFrequency"` | `[[time, value], ...]` | Scales frequency by an envelope curve |
| `"dilate"` | `[[inputTimes], [targetTimes]]` | Time-stretches partials using input/target time pairs |
| `"applyFilter"` | `[[freq, value], ...]` | Applies a gain envelope in the frequency domain |
