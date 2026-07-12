Sets the MIDI controller value on the current event. Only works on Controller events with a range of 0-127.

> [!Warning:Only works on Controller events] Unlike `Message.getControllerValue()` which works on Controller, PitchBend, and Aftertouch events, this setter only works on Controller events. To modify aftertouch pressure, use `Message.setMonophonicAfterTouchPressure()` or `Message.setPolyAfterTouchNoteNumberAndPressureValue()`. There is no direct setter for pitch bend values.
