Sets the markdown text to be parsed and rendered. The text is parsed immediately into internal layout elements, replacing any previously set content. Use `\n` (the literal two-character escape sequence) to insert newlines within a HISEScript string.

Call `setTextBounds()` again after changing the text to get an updated layout height, then `repaint()` on the host panel to trigger a redraw.
