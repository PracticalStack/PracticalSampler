Sets the rendering area as an `[x, y, width, height]` array and returns the actual height required to display the current text at the given width. Use the returned height to resize the host panel so it fits the content exactly.

Since the height is auto-calculated from the text content, you can pass a large value (e.g. `9000`) for the height parameter - it serves only as an upper bound and does not affect the returned measurement.

> [!Warning:Required before drawing, re-call after setText] This method must be called before `Graphics.drawMarkdownText()` or a script error is thrown. Call it again after each `setText()` to get an up-to-date height for the new content.
