Updates the parameter's range from a JSON object. You only need to specify the properties you want to change - missing properties receive defaults:

| Property | Default |
|----------|---------|
| `MinValue` | 0.0 |
| `MaxValue` | 1.0 |
| `SkewFactor` | 1.0 |
| `StepSize` | 0.0 (continuous) |
| `Inverted` | false |

This method supports undo via the parent node's UndoManager, unlike `setRangeProperty()` which does not.
