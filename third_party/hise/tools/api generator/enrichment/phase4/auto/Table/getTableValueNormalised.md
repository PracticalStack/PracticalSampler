Returns the interpolated output value of the table at a given normalised input position (0.0-1.0). The input is mapped to the internal lookup array using linear interpolation between adjacent entries. This is the primary method for querying the curve at runtime - for example, remapping MIDI velocity through a user-editable response curve.

> [!Warning:Normalise input to 0-1 range] Always normalise the input to 0.0-1.0 before calling. Raw MIDI values (0-127) exceed the table range, and any input above 1.0 returns the last table value, effectively flattening the response.
