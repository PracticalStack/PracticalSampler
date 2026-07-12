Stores a value at the given index. Out-of-range indices (negative or >= 128) are silently ignored. Setting a slot to `-1` marks it as empty. You can also use bracket syntax: `list[60] = 100` is equivalent to `list.setValue(60, 100)`.

> [!Warning:Out-of-range writes silently ignored] Out-of-range index writes are silently ignored with no error message. If your indices come from user input or calculations, validate them before calling.
