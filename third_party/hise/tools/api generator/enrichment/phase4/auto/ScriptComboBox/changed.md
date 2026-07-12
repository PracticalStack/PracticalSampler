Triggers the control callback manually with the current value. Use this in cascading selector patterns where rebuilding one combo box's items should propagate through the dependent callback chain. Always call `setValue()` before `changed()` to ensure the value is within the new item range.

> [!Warning:Ignored during onInit] Cannot be called during `onInit` - the call is silently ignored. If your callback throws an error, script execution after the `changed()` call is aborted.
