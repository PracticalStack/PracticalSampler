# MarkdownRenderer -- Project Context

## Project Context

### Real-World Use Cases
- **Modal dialog content**: A plugin that needs confirmation dialogs or informational popups creates a MarkdownRenderer once, stores it alongside the dialog panel, and calls `setText()` each time the dialog content changes. The renderer is drawn inside the panel's paint routine, producing styled text with headings and bold formatting without needing a separate label component. This is the most common use case - it turns a ScriptPanel into a rich-text display.
- **Rich tooltip system**: A plugin with contextual help creates a MarkdownRenderer paired with a tooltip panel. When the user hovers a control, the tooltip text is set via `setText()`, the panel auto-sizes to the content height returned by `setTextBounds()`, and the renderer draws inside the panel's paint routine. Image providers supply inline icons (mute, solo, etc.) referenced in the markdown text via `![](/icon_name)` syntax.
- **Error/status display**: A plugin's activation or error handling system uses a MarkdownRenderer to display formatted error messages. The renderer's text is updated reactively through a broadcaster listener, and it renders inside a dedicated status panel. A shared style configuration object ensures visual consistency across the UI.

### Complexity Tiers
1. **Basic styled text** (most common): `setText()` + `setTextBounds()` + `g.drawMarkdownText()` inside a ScriptPanel paint routine. No style customization - uses defaults. Good for simple multi-line text that needs bold/italic/headings.
2. **Themed rendering**: Add `getStyleData()` / `setStyleData()` to customize fonts, colours, and table appearance. Use this when the renderer must match a custom UI theme rather than the default dark-on-light style.
3. **Rich content with images**: Add `setImageProvider()` with path entries to embed vector icons inline in markdown text. Combine with dynamic height calculation to auto-size containers. This tier is used for documentation-style tooltips or help panels.

### Practical Defaults
- Use `getStyleData()` to get the current style, modify the returned object, then pass it back to `setStyleData()`. This preserves defaults for properties you do not change, since `setStyleData()` resets unspecified properties.
- Set `tableLineColour` to `0` (transparent) when rendering markdown that does not use visible table borders - the default table colours add unwanted gridlines.
- A `FontSize` of 13.0-14.0 is a good default for tooltip or secondary text. The default 18.0 works well for dialog body text.
- Always store the MarkdownRenderer as a `const var` at namespace or file scope. Creating it once and calling `setText()` to update is much cheaper than creating a new renderer each paint cycle.

### Integration Patterns
- `MarkdownRenderer.setText()` -> `ScriptPanel.repaint()` - After changing the markdown text, call `repaint()` on the host panel to trigger a redraw that will call `g.drawMarkdownText()`.
- `MarkdownRenderer.setTextBounds()` return value -> `ScriptPanel.set("height", ...)` - Use the returned height to auto-size the panel to fit the content, adding margins as needed.
- `Broadcaster.addListener()` -> `MarkdownRenderer.setText()` - Drive markdown content reactively through a broadcaster. In the listener callback, update the text and repaint the host panel.
- `Content.createPath()` -> `setImageProvider()` - Path objects created via Content or loaded from data are passed as the `Data` property in image provider entries for inline vector icons.

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| Creating a new MarkdownRenderer inside `setPaintRoutine()` | Create once in `onInit`, call `setText()` to update, draw in paint routine | The renderer is a persistent object, not a per-frame operation. Creating inside paint allocates on every repaint. |
| Calling `setStyleData()` with only the properties you want to change | Call `getStyleData()` first, modify the returned object, then `setStyleData()` | `setStyleData()` resets all unspecified properties to defaults. Passing a partial object loses your other customizations. |
| Calling `setText()` after `setTextBounds()` and using the old height | Call `setTextBounds()` again after each `setText()` to get the correct height | The height returned by `setTextBounds()` reflects the text at the time of that call. New text may require different height. |
