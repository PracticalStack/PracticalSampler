Adds a target that triggers a refresh action on the specified UI components whenever the broadcaster fires, ignoring the broadcast arguments entirely. The `refreshType` string determines the action:

| Refresh Type | Action |
|---|---|
| `"repaint"` | Triggers the component's paint routine or LAF function |
| `"changed"` | Triggers the control callback as if the user interacted with it |
| `"updateValueFromProcessorConnection"` | Refreshes the displayed value from the connected processor parameter |
| `"loseFocus"` | Removes keyboard/mouse focus |
| `"resetValueToDefault"` | Resets the value to the component's default |
