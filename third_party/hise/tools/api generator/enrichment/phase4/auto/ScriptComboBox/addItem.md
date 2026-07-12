Appends a new item to the end of the combo box's item list and increments the maximum value to match. Use this to build item lists dynamically - for example, from file scan results or engine-provided arrays. Clear existing items with `set("items", "")` before populating to avoid stale entries left by the Interface Designer.

> [!Warning:Names truncated to 128 characters] Item names are silently truncated to 128 characters with no error or notification.
