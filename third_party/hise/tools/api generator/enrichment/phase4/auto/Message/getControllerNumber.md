Returns the controller number of the current event inside `onController`. For standard CCs this is the actual CC number (0-127). Pitch bend and aftertouch also route through `onController` and return virtual CC numbers: 128 (`Message.PITCH_BEND_CC`) for pitch bend, 129 (`Message.AFTERTOUC_CC`) for aftertouch. This lets you handle all three event types uniformly with a single switch or if-chain.

> [!Warning:CC range filter excludes pitch and aftertouch] Code that filters by CC range (e.g. `if (cc < 128)`) will implicitly exclude pitch wheel and aftertouch events.
