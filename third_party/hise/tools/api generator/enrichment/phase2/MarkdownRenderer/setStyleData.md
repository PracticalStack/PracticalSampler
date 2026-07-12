## setStyleData

**Examples:**

```javascript:get-modify-set-pattern
// Title: Customize style with the get-modify-set pattern
// Context: The safe way to change individual style properties without
// resetting others. Get the defaults, modify what you need, set it back.

const var md = Content.createMarkdownRenderer();

// Get current style (preserves all defaults)
const var style = md.getStyleData();

// Modify only what you need
style.headlineColour = 0xFF333333;
style.textColour = 0xFF444444;
style.Font = "Roboto";
style.FontSize = 13.0;

// Table styling - set to transparent for borderless tables
style.tableLineColour = 0;
style.tableHeaderBgColour = 0;
style.tableBgColour = 0;

md.setStyleData(style);
```

```json:testMetadata:get-modify-set-pattern
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "md.getStyleData().FontSize", "value": 13.0},
    {"type": "REPL", "expression": "md.getStyleData().tableLineColour", "value": 0}
  ]
}
```

```javascript:shared-style-config
// Title: Define a reusable style configuration object
// Context: When multiple renderers share the same visual theme,
// define the style as a standalone JSON object and pass it
// to each renderer's setStyleData().

const var DARK_THEME_STYLE = {
    "Font": "Roboto",
    "BoldFont": "Roboto Bold",
    "FontSize": 14.0,
    "bgColour": 0,
    "codeBgColour": 0,
    "linkBgColour": 0,
    "textColour": 0x88FFFFFF,
    "codeColour": 0xFFAAAAEE,
    "linkColour": 0xFF6666EE,
    "headlineColour": 0xFF5F9EA0,
    "tableHeaderBgColour": 0,
    "tableLineColour": 0,
    "tableBgColour": 0,
    "UseSpecialBoldFont": false
};

// Apply the same theme to multiple renderers
const var dialogMd = Content.createMarkdownRenderer();
dialogMd.setStyleData(DARK_THEME_STYLE);

const var errorMd = Content.createMarkdownRenderer();
errorMd.setStyleData(DARK_THEME_STYLE);

// The style object can also be used for consistent font/colour
// choices in other drawing code alongside the markdown
```

```json:testMetadata:shared-style-config
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "dialogMd.getStyleData().FontSize", "value": 14.0},
    {"type": "REPL", "expression": "errorMd.getStyleData().FontSize", "value": 14.0}
  ]
}
```

**Pitfalls:**
- When passing a standalone JSON object (not one obtained from `getStyleData()`), every style property that is not included in the object will be reset to its default value. To avoid surprises, always define the complete set of properties in shared configuration objects, or use the get-modify-set pattern for incremental changes.
