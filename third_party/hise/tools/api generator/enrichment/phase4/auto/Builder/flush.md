Finalises all pending module tree changes by notifying the UI and patch browser. Call this once after all `create()`, `clearChildren()`, or `clear()` operations are complete. Calling it multiple times is harmless - subsequent calls do nothing if the tree is already flushed.

The Builder's destructor logs a console warning if `flush()` was never called after modifications, helping catch the common mistake of forgetting this step.
