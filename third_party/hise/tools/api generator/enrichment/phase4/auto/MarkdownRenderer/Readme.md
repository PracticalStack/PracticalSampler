<!-- Diagram triage:
  - (no diagrams in Phase 1 data)
-->

# MarkdownRenderer

MarkdownRenderer is a draw action object that parses markdown-formatted text and renders it inside a ScriptPanel paint routine or LAF function. Unlike the MarkdownPanel FloatingTile, it does not require a file directory and gives you full control over positioning, styling, and dynamic content updates.

Create one with `Content.createMarkdownRenderer()`, set the text and bounds, then draw it with `Graphics.drawMarkdownText()`:

```js
const var md = Content.createMarkdownRenderer();
md.setText("## Hello\nThis is **bold** text.");
md.setTextBounds([0, 0, 400, 9000]);
```

The renderer supports standard markdown syntax including headings (4 levels), bold, italic, inline code, fenced code blocks, bullet and numbered lists, tables, links, block quotes, and horizontal rules. Images require a custom image provider registered via `setImageProvider()`.

The typical workflow has three tiers of complexity:

1. **Basic styled text** - call `setText()`, `setTextBounds()`, and `g.drawMarkdownText()` to render formatted text with the default theme.
2. **Themed rendering** - add `getStyleData()` / `setStyleData()` to customise fonts, colours, and table appearance.
3. **Rich content with images** - add `setImageProvider()` with path or image entries for inline vector icons or pool images.

> [!Tip:Create once in onInit, reuse per repaint] Create the MarkdownRenderer once in `onInit` and store it as a `const var`. Call `setText()` to update content and `repaint()` on the host panel to trigger a redraw - do not create a new renderer each paint cycle.

## Common Mistakes

- **Start from getStyleData for changes**
  **Wrong:** Calling `setStyleData()` with only the properties you want to change.
  **Right:** Call `getStyleData()` first, modify the returned object, then pass it back to `setStyleData()`.
  *`setStyleData()` resets all unspecified properties to defaults. Passing a partial object loses your other customisations.*

- **Re-call setTextBounds after setText**
  **Wrong:** Calling `setText()` after `setTextBounds()` and relying on the previously returned height.
  **Right:** Call `setTextBounds()` again after each `setText()` to get the correct height for the new content.
  *The height returned by `setTextBounds()` reflects the text at the time of that call. New text may require a different layout height.*

- **Call setTextBounds before drawing**
  **Wrong:** Calling `g.drawMarkdownText(md)` without calling `md.setTextBounds()` first.
  **Right:** Always call `setTextBounds()` before drawing.
  *A script error is thrown if bounds have not been set.*
