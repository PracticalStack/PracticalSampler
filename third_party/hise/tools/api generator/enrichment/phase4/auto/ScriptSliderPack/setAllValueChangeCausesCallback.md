Enables or disables callback triggering for non-undo write helpers such as `setAllValues` and `setSliderAtIndex`.

This is useful when loading or transforming large datasets and you only want one explicit refresh at the end.

> [!Warning:Undo writes still emit notifications] This toggle does not silence undo bulk writes - `setAllValuesWithUndo` still emits change notification.
