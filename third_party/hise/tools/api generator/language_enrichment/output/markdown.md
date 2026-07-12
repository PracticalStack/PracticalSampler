---
title: Markdown
description: "HISE's markdown dialect for in-app text rendering and forum posts"

guidance:
  summary: >
    Reference for the markdown syntax accepted by HISE's built-in renderer.
    Three consumers: the MarkdownRenderer scripting API class (for displaying
    formatted text inside plugin UIs), the TextBox component within
    ScriptDynamicContainer, and the HISE forum. Covers supported syntax
    (headings, text formatting, lists, links, images, tables, code blocks),
    HISE-specific extensions (fixed-width table columns), and a categorised
    differences-from-standard table.
  concepts:
    - markdown
    - MarkdownRenderer
    - ScriptDynamicContainer
    - TextBox
    - forum formatting
  prerequisites: []
  complexity: beginner
---

HISE includes a lightweight markdown renderer for displaying formatted text inside your project. It supports a subset of standard Markdown — enough for rich text display, but stripped of features that don't apply to an embedded audio plugin context (no inline HTML, no nested blockquotes, no reference-style links).

In practice, markdown appears in three contexts:

- **Plugin UI text** — the $API.MarkdownRenderer$ scripting API class renders markdown strings as formatted text inside ScriptPanel components.
- **Dynamic container content** — the TextBox component within $API.ScriptDynamicContainer$ displays markdown for in-app documentation or help text.
- **Forum posts** — the [HISE forum](https://forum.hise.audio) uses the same markdown dialect for post formatting.

> [!Warning:Documentation pipeline deprecated] The markdown-based documentation system within HISE is deprecated. This reference covers the syntax accepted by the MarkdownRenderer, the ScriptDynamicContainer TextBox, and the forum — not a documentation authoring workflow.

**See also:** [Usage in HISE](#usage-in-hise) -- how markdown integrates with the MarkdownRenderer API and other consumers


## The Language

HISE's markdown follows standard [Markdown](https://www.markdownguide.org) conventions with a few omissions and one extension.

### Text Formatting

| Syntax | Result |
| --- | --- |
| `**bold**` | **bold** |
| `` `code` `` | Monospaced inline code |
| Two trailing spaces | Line break |

> [!Tip:No italic or strikethrough] HISE's renderer supports bold and code spans only. If you need emphasis beyond bold, use code formatting or structural separation instead.

### Headings

```
# Heading 1
## Heading 2
### Heading 3
```

Headings up to level 2 appear in the table of contents and are linkable via anchor syntax (`#heading-urlified`).

### Paragraphs & Lists

```
> This is a blockquote

- Bullet point
- Another point

1. Numbered item
2. Another item
```

> [!Warning:Single-level blockquotes only] Nested blockquotes (`> > text`) are not supported. The renderer treats all `>` prefixes as a single blockquote level.

### Links

```
[Display text](link-target)
```

Only inline link syntax is supported — reference-style links (`[text][ref]` with a separate `[ref]: url` definition) are not parsed.

### Images

```
![Alt text](image-url)
```

### Tables

Standard markdown table syntax with an optional HISE extension for fixed column widths:

```
| Column A | Column B |
| ---:120px | --- |
| cell 1 | cell 2 |
| cell 3 | cell 4 |
```

The `:120px` suffix after `---` sets a fixed pixel width for that column. This is useful for icon or shortcut columns.

> [!Tip:Use the Table generator] The HISE markdown table parser is stricter than most parsers. Use the built-in Table generator in the editor to avoid formatting issues.

### Code Blocks

Fenced code blocks with language hints are supported:

````
```javascript
Console.print("Hello");
```
````


## Usage in HISE

Markdown content reaches the renderer through three paths: the MarkdownRenderer scripting API, the ScriptDynamicContainer TextBox component, and the HISE forum.

### Render text in plugin UIs

The $API.MarkdownRenderer$ class renders a markdown string into a ScriptPanel's graphics context. Create a renderer, set its text, and paint it in the panel's paint routine:

```javascript
const var md = Content.createMarkdownRenderer();
md.setText("## Hello\nThis is **bold** text.");

Panel1.setPaintRoutine(function(g)
{
    md.draw(g, this.getLocalBounds(0));
});
```

The renderer parses the markdown once on `setText` and caches the layout — calling `draw` repeatedly is cheap.

> [!Tip:Update text dynamically] Call `setText` again with new markdown content to update what the panel displays. The renderer re-parses and re-caches automatically.

**See also:** $API.MarkdownRenderer$ -- full class reference

### Display text in dynamic containers

The TextBox component type within a $API.ScriptDynamicContainer$ accepts markdown content for displaying help text, descriptions, or formatted content alongside other dynamic UI elements.

**See also:** $API.ScriptDynamicContainer$ -- dynamic container reference

### Format forum posts

The [HISE forum](https://forum.hise.audio) uses the same markdown renderer. All syntax documented on this page works in forum posts — including fenced code blocks with language hints for syntax highlighting.


## Differences from Standard Markdown

HISE's markdown deviations fall into three categories.

### Feature Omissions

Features from the Markdown specification that don't apply to an embedded text renderer in an audio plugin UI. There's no HTML rendering engine behind the scenes, so HTML passthrough and features that depend on it aren't available.

| Feature | Standard Markdown | HISE Markdown |
| --- | --- | --- |
| Italic (`*text*`) | Supported | Not supported |
| Strikethrough (`~~text~~`) | Supported | Not supported |
| Custom HTML | Supported | Not supported — no inline HTML |
| Nested blockquotes | Supported | Not supported — single level only |
| Reference-style links | Supported | Not supported — use inline links |

### Deliberate Design Decisions

Intentional differences that improve reliability in HISE's rendering context.

| Feature | Standard Markdown | HISE Markdown | Rationale |
| --- | --- | --- | --- |
| Table parsing | Lenient | Strict | Prevents ambiguous layouts — use the Table generator |
| Blockquote rendering | Generic quotes | Styled comment boxes | Visual consistency with HISE's UI style |
| Heading anchors | Varies by parser | Auto-generated for h1 and h2 only | Keeps the TOC manageable in a panel context |

### HISE-Specific Extensions

| Feature | Standard Markdown | HISE Markdown |
| --- | --- | --- |
| Table column widths (`---:120px`) | Not available | Fixed pixel widths for individual columns |


**See also:** $API.MarkdownRenderer$ -- scripting API for rendering markdown in plugin UIs, $API.ScriptDynamicContainer$ -- dynamic container with TextBox markdown support
