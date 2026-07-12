Draws the text of a `MarkdownRenderer` object to its previously specified area. The renderer must be created via `Content.createMarkdownRenderer()` and must have `setTextBounds()` called before this method. The MarkdownRenderer handles its own text layout and styling - the current colour and font set on the Graphics object do not affect the output.

> [!Warning:Call setTextBounds first] Calling this method without first calling `setTextBounds()` on the renderer triggers a script error.
