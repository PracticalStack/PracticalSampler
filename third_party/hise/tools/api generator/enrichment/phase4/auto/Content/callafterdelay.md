Schedules a function to execute once after the specified delay in milliseconds. The callback runs on the UI thread and takes no arguments. An optional third argument sets the `this` context for the callback.

> [!Warning:Not sample-accurate timing] The timing is not sample-accurate. Do not use this for musical or DSP-critical scheduling - use a timer object or the transport handler instead.