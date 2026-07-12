Sets the MIDI controller number on the current event. Only works on Controller events (CC 0-127).

> [!Warning:Only works on Controller events] Unlike `Message.getControllerNumber()` which works on Controller, PitchBend, and Aftertouch events, this setter only works on Controller events. Calling it on a PitchBend or Aftertouch event produces a script error.
