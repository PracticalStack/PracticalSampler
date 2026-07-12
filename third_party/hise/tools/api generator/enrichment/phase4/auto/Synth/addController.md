Adds a controller event to the MIDI buffer with an explicit channel and sample-accurate timestamp. The `number` parameter accepts standard CC numbers (0-127), or the special values 128 (pitch bend, value range 0-16383) and 129 (aftertouch). Use `sendController` when you do not need to specify a channel or timestamp.

> [!Warning:Creates artificial events, not real ones] Events created by `addController` are flagged as artificial, unlike those from `sendController`. If downstream logic filters on the artificial flag, the two methods produce different results for identical CC values.
