# MarkdownRenderer -- Class Analysis

## Brief
Markdown text renderer drawn via Graphics.drawMarkdownText() inside paint callbacks.

## Purpose
MarkdownRenderer is a stateful object that parses markdown-formatted text into styled visual elements (headings, lists, tables, code blocks, images) and renders them as a draw action within a ScriptPanel paint routine or LAF function. It is created via `Content.createMarkdownRenderer()`, configured with text, style, bounds, and optional image providers, then passed to `Graphics.drawMarkdownText()` for rendering. The renderer supports customizable fonts, colours, and embedded images resolved from the HISE image pool or vector paths.

## Details

### Architecture

MarkdownRenderer is not a UI component -- it is a **draw action object** that accumulates rendering state and executes within the Graphics draw pipeline. Internally, it wraps a `DrawActions::MarkdownAction` containing the C++ `MarkdownRenderer` engine, a `CriticalSection` for thread safety, and a `Rectangle<float>` for the render area.

The workflow follows this pattern:
1. Create the renderer in `onInit`
2. Set text via `setText()`
3. Set bounds via `setTextBounds()` (returns the required height)
4. Optionally set style via `setStyleData()`
5. Optionally set image providers via `setImageProvider()`
6. In a paint callback, call `g.drawMarkdownText(renderer)`

### Supported Markdown Syntax

| Element | Syntax | Notes |
|---------|--------|-------|
| Headings | `# H1` through `#### H4` | 4 levels maximum |
| Bold | `**text**` | -- |
| Italic | `*text*` | -- |
| Inline code | `` `code` `` | -- |
| Code blocks | Triple backtick fenced | -- |
| Bullet lists | `- item` | -- |
| Numbered lists | `1. item` | -- |
| Tables | Pipe-separated `\|` | With header row |
| Links | `[text](url)` | -- |
| Images | `![alt](url)` | Requires image provider |
| Block quotes | `> text` | -- |
| Horizontal rules | `---`, `***`, `___` | -- |

### StyleData JSON Schema

The style data object controls all visual properties:

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `Font` | String | `"default"` | Font family name (loaded font or `"default"` for global font) |
| `BoldFont` | String | `"default"` | Bold font family name |
| `FontSize` | Number | `18.0` | Base font size in pixels |
| `UseSpecialBoldFont` | Boolean | `false` | Whether to use a separate bold typeface |
| `bgColour` | Integer | `0xFF333333` | Background colour (ARGB int64) |
| `textColour` | Integer | `0xFFFFFFFF` | Text colour |
| `headlineColour` | Integer | SIGNAL_COLOUR | Headline text colour |
| `codeColour` | Integer | `0xFFFFFFFF` | Code text colour |
| `codeBgColour` | Integer | `0x33888888` | Code block background |
| `linkColour` | Integer | `0xFFAAAAFF` | Link text colour |
| `linkBgColour` | Integer | `0x008888FF` | Link highlight background |
| `tableBgColour` | Integer | grey@0.2 alpha | Table cell background |
| `tableHeaderBgColour` | Integer | grey@0.2 alpha | Table header background |
| `tableLineColour` | Integer | grey@0.2 alpha | Table border colour |

Colours are represented as ARGB int64 values in the scripting API (not hex strings).

### Image Provider

The renderer supports embedded images via `setImageProvider()`, which accepts a JSON array of image entries mapping markdown `![alt](url)` links to either vector paths or pool images. See `setImageProvider()` for the full entry format and provider behavior.

### Thread Safety

All API methods are thread-safe. A CriticalSection protects access to the internal renderer, allowing configuration from the scripting thread while rendering occurs on the paint thread.

## obtainedVia
`Content.createMarkdownRenderer()`

## minimalObjectToken
md

## Constants
No constants defined.

## Dynamic Constants
No dynamic constants.

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| `g.drawMarkdownText(md)` without calling `md.setTextBounds()` first | Call `md.setTextBounds([x, y, w, h])` before `g.drawMarkdownText(md)` | Throws a script error: "You have to call setTextBounds() before using this method" |

## codeExample
```javascript
const var md = Content.createMarkdownRenderer();
md.setText("## Hello\nThis is **bold** and *italic* text.");
md.setTextBounds([0, 0, 400, 300]);
```

## Alternatives
Graphics (provides the drawMarkdownText rendering method), ScriptLabel (for simple plain text display without markdown formatting).

## Related Preprocessors
None.

## Diagnostic Ideas
Reviewed: Yes
Count: 0
Rationale: The only precondition (setTextBounds before drawMarkdownText) is already enforced at runtime with a script error, so no additional parse-time diagnostic is needed.
