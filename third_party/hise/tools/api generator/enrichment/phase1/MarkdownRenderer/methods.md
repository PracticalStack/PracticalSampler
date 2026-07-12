# MarkdownRenderer -- Methods

## getStyleData

**Signature:** `JSON getStyleData()`
**Return Type:** `JSON`
**Call Scope:** unsafe
**Call Scope Note:** Acquires a CriticalSection lock on the internal renderer, then constructs a DynamicObject with string and int64 properties (heap allocation).
**Minimal Example:** `var style = {obj}.getStyleData();`

**Description:**
Returns the current style configuration as a JSON object. The returned object contains all configurable style properties including font names, font size, and colour values. Colours are returned as ARGB int64 values (not hex strings). The object can be modified and passed back to `setStyleData()` to apply changes incrementally.

**Parameters:**

(No parameters.)

**Callback Properties:**

| Property | Type | Description |
|----------|------|-------------|
| Font | String | Font family name (`"default"` for global font) |
| BoldFont | String | Bold font family name (`"default"` for global bold font) |
| FontSize | Double | Base font size in pixels (default: 18.0) |
| UseSpecialBoldFont | Integer | Whether to use a separate bold typeface (0 or 1) |
| bgColour | Integer | Background colour as ARGB int64 |
| textColour | Integer | Text colour as ARGB int64 |
| headlineColour | Integer | Headline text colour as ARGB int64 |
| codeColour | Integer | Code text colour as ARGB int64 |
| codeBgColour | Integer | Code block background colour as ARGB int64 |
| linkColour | Integer | Link text colour as ARGB int64 |
| linkBgColour | Integer | Link highlight background colour as ARGB int64 |
| tableBgColour | Integer | Table cell background colour as ARGB int64 |
| tableHeaderBgColour | Integer | Table header background colour as ARGB int64 |
| tableLineColour | Integer | Table border colour as ARGB int64 |

**Cross References:**
- `$API.MarkdownRenderer.setStyleData$`

**Example:**
```javascript:get-modify-style
// Title: Modify existing style incrementally
const var md = Content.createMarkdownRenderer();

var style = md.getStyleData();
style.FontSize = 24.0;
style.textColour = 0xFFCCCCCC;
md.setStyleData(style);
```

```json:testMetadata:get-modify-style
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "md.getStyleData().FontSize", "value": 24.0}
  ]
}
```

## setImageProvider

**Signature:** `undefined setImageProvider(Array data)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Creates new image provider objects (heap allocation), acquires CriticalSection lock, clears all existing resolvers, and sets the new provider.
**Minimal Example:** `{obj}.setImageProvider(imageProviderData);`

**Description:**
Creates a custom image provider from a JSON array of image entries that resolves `![alt](url)` markdown image links to actual images. Each entry in the array must have a `URL` property matching the markdown image link syntax. Entries are classified as either Path entries (vector paths rendered to bitmap) or Image entries (loaded from the HISE image pool). The provider is registered with the highest priority (EmbeddedPath), so it takes precedence over all other image resolvers. Calling this method replaces all previously registered image and link resolvers on the internal renderer.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| data | Array | no | JSON array of image provider entries | Each element must be an object with a `URL` property |

**Callback Properties:**

Path entry properties:

| Property | Type | Description |
|----------|------|-------------|
| URL | String | The markdown image URL to match (e.g. `"icon_url"`) |
| Type | String | Must be `"Path"` for vector path entries |
| Data | Array | Path data (base64 string or array of path points) |
| Colour | Integer | Fill colour for the path as ARGB int64 (default: 0xFF888888) |

Image entry properties:

| Property | Type | Description |
|----------|------|-------------|
| URL | String | The markdown image URL to match |
| Reference | String | Pool reference string (e.g. `"{PROJECT_FOLDER}image.png"`) |

**Pitfalls:**
- Calling `setImageProvider()` clears ALL existing resolvers on the internal markdown renderer (both image providers and link resolvers), not just the previously set scripted provider. After this call, only the newly provided entries will resolve images.
- [BUG] If the `data` parameter is not an array, no entries are created and the method silently succeeds after clearing all existing resolvers. This leaves the renderer with no image resolution capability.
- URL matching compares the sanitized URL without anchors. The `URL` property in each entry should match the URL portion of the markdown image syntax, not the full `![alt](url)` markdown.

**Cross References:**
- `$API.MarkdownRenderer.setText$`

**Example:**
```javascript:image-provider-setup
// Title: Register path and image providers for markdown
const var md = Content.createMarkdownRenderer();

const var providers = [
    {
        "URL": "icon_check",
        "Type": "Path",
        "Data": Content.createPath(),
        "Colour": 0xFF00FF00
    }
];

md.setImageProvider(providers);
md.setText("Here is an icon: ![check](icon_check)");
md.setTextBounds([0, 0, 400, 300]);
```

```json:testMetadata:image-provider-setup
{
  "testable": false,
  "skipReason": "Path data requires a valid path object and visual verification of rendered image"
}
```

## setStyleData

**Signature:** `undefined setStyleData(JSON styleData)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Constructs a StyleData object from the JSON (font lookups, string operations), then acquires CriticalSection lock to apply it to the internal renderer.
**Minimal Example:** `{obj}.setStyleData({"FontSize": 24.0, "textColour": 0xFFCCCCCC});`

**Description:**
Sets the visual style configuration for the markdown renderer. Accepts a JSON object with style properties controlling fonts, font size, and colours. Properties not included in the object are reset to their default values (a fresh StyleData is constructed with defaults, then populated from the JSON). To modify individual properties without resetting others, call `getStyleData()` first, modify the returned object, and pass it back. Font names are resolved through the main controller's font registry -- use `"default"` to select the global HISE font. Colours must be provided as ARGB int64 values.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| styleData | JSON | no | Style configuration object | See getStyleData for the full property schema |

**Callback Properties:**

| Property | Type | Description |
|----------|------|-------------|
| Font | String | Font family name (`"default"` for global font) |
| BoldFont | String | Bold font family name (`"default"` for global bold font) |
| FontSize | Double | Base font size in pixels |
| UseSpecialBoldFont | Integer | Whether to use a separate bold typeface (0 or 1) |
| bgColour | Integer | Background colour as ARGB int64 |
| textColour | Integer | Text colour as ARGB int64 |
| headlineColour | Integer | Headline text colour as ARGB int64 |
| codeColour | Integer | Code text colour as ARGB int64 |
| codeBgColour | Integer | Code block background colour as ARGB int64 |
| linkColour | Integer | Link text colour as ARGB int64 |
| linkBgColour | Integer | Link highlight background colour as ARGB int64 |
| tableBgColour | Integer | Table cell background colour as ARGB int64 |
| tableHeaderBgColour | Integer | Table header background colour as ARGB int64 |
| tableLineColour | Integer | Table border colour as ARGB int64 |

**Pitfalls:**
- When `BoldFont` is set to `"default"`, the C++ code forces `useSpecialBoldFont = true` regardless of the `UseSpecialBoldFont` property in the JSON. If you explicitly pass `UseSpecialBoldFont: false` alongside `BoldFont: "default"`, the `false` is overridden to `true` silently.

**Cross References:**
- `$API.MarkdownRenderer.getStyleData$`

## setText

**Signature:** `undefined setText(String markdownText)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Acquires CriticalSection lock, then calls `setNewText()` which parses the markdown string into element objects (heap allocations for each parsed element, string construction).
**Minimal Example:** `{obj}.setText("## Heading\nSome **bold** text.");`

**Description:**
Sets the markdown text to be parsed and rendered. The text is immediately parsed into internal element objects (headings, paragraphs, code blocks, tables, lists, images, etc.) by the underlying MarkdownParser. Supports standard markdown syntax including headings (up to 4 levels), bold, italic, inline code, fenced code blocks, bullet lists, numbered lists, tables, links, images, block quotes, and horizontal rules. Calling this method again replaces the previously parsed content entirely. The text must be set before calling `setTextBounds()` for the height calculation to reflect the actual content.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| markdownText | String | no | Markdown-formatted text to parse and render | -- |

**Pitfalls:**
- Headline levels beyond 4 (`#####` or more) are clamped to level 4. No error is reported.

**Cross References:**
- `$API.MarkdownRenderer.setTextBounds$`
- `$API.MarkdownRenderer.setImageProvider$`
- `$API.Graphics.drawMarkdownText$`

## setTextBounds

**Signature:** `Double setTextBounds(Array area)`
**Return Type:** `Double`
**Call Scope:** unsafe
**Call Scope Note:** Acquires CriticalSection lock, then calls `getHeightForWidth()` which iterates all parsed elements to compute layout heights (may trigger font metric calculations and heap allocations for cached layout data).
**Minimal Example:** `var height = {obj}.setTextBounds([0, 0, 400, 300]);`

**Description:**
Sets the rendering area for the markdown content and returns the actual height required to display the full parsed text at the specified width. The area is stored internally and used by `Graphics.drawMarkdownText()` to position the rendered output. The returned height may be greater or less than the height provided in the area parameter -- use it to size a ScriptPanel appropriately or to detect overflow. The height calculation is cached internally: calling `setTextBounds()` with the same width repeatedly returns the cached result without re-computing layout. This method must be called before `Graphics.drawMarkdownText()` or a script error is thrown.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| area | Array | no | Rectangle as `[x, y, width, height]` | 4-element numeric array; reports a script error if the format is invalid |

**Pitfalls:**
- The returned height reflects the content set by the last `setText()` call. If `setText()` is called after `setTextBounds()`, the stored area dimensions remain but the returned height from the previous `setTextBounds()` call is stale. Call `setTextBounds()` again after `setText()` to get the correct height for the new content.

**Cross References:**
- `$API.MarkdownRenderer.setText$`
- `$API.Graphics.drawMarkdownText$`

**Example:**
```javascript:dynamic-height-panel
// Title: Size a ScriptPanel to fit markdown content
const var md = Content.createMarkdownRenderer();
md.setText("# Title\nParagraph one.\n\n- Item A\n- Item B\n- Item C");

var requiredHeight = md.setTextBounds([0, 0, 400, 1000]);

// Changing the text invalidates the previous height
md.setText("Short.");
var shortHeight = md.setTextBounds([0, 0, 400, 1000]);
```

```json:testMetadata:dynamic-height-panel
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "requiredHeight > 50", "value": true},
    {"type": "REPL", "expression": "shortHeight < requiredHeight", "value": true}
  ]
}
```

