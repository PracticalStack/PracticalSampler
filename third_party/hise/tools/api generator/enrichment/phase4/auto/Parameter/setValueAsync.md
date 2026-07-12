> **Deprecated.** Use `setValue()` after calling `setUseExternalConnection(true)` for the same behaviour (immediate DSP update, no undo).

Applies the value directly to the node without storing it. The value is applied to all voices in polyphonic networks. Does not support undo.

> [!Warning:Silently ignored before node initialisation] If the node has not been fully connected in the network, the call is silently ignored without error.
