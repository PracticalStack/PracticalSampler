Returns true if the current preset load originates from a DAW state restore or initial state load, rather than the user selecting a preset from the browser. This distinction enables "parameter lock" features where locked values survive user-initiated preset changes but are overwritten on DAW recall.

> [!Warning:Only valid inside preset callbacks] Only meaningful inside `setPreCallback` or `setPostCallback`. Outside those callbacks, the flag retains its value from the most recent load and may produce incorrect conditional logic.
