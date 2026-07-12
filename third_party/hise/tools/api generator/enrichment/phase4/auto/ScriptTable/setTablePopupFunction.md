Sets a formatter callback for the point-drag popup text. The callback receives normalised `x` and `y` values and returns the string to display.

> [!Warning:Null or empty hides popup text] Returning `null` or an empty string hides popup text. Call `setTablePopupFunction(false)` to restore the default popup formatter.
