Sets a single range property by ID. Use the Parameter constants for the first argument: `p.MinValue`, `p.MaxValue`, `p.StepSize`. For `SkewFactor`, pass the string literal `"SkewFactor"` directly (no constant shortcut exists).

> [!Warning:No undo support] Unlike `setRangeFromObject()`, changes made with `setRangeProperty()` are not recorded in the UndoManager.
