Creates a snapshot of a partial parameter at a specific time point, returning the value for each harmonic across all channels. Returns a nested array: the outer array has one entry per channel, and each inner array contains the parameter value for each harmonic at the specified time.

The time value is interpreted according to the current `timedomain` setting (seconds by default). The file must have been previously analysed.

> [!Warning:512-harmonic truncation limit] The internal buffer is fixed at 512 harmonics per channel. Sounds with more than 512 tracked partials will have their higher harmonics silently truncated.
