## loadFontAs

**Examples:**

```javascript:load-multiple-font-weights
// Title: Loading multiple font weights at initialization
// Context: Plugins typically load several weights of the same font
// family for different UI elements (headings, labels, readouts).
// All font files must be placed in the Images folder.

Engine.loadFontAs("{PROJECT_FOLDER}Fonts/Roboto-Regular.ttf", "regular");
Engine.loadFontAs("{PROJECT_FOLDER}Fonts/Roboto-Medium.ttf", "medium");
Engine.loadFontAs("{PROJECT_FOLDER}Fonts/Roboto-Bold.ttf", "bold");

// Specialty font for LED-style numeric displays
Engine.loadFontAs("{PROJECT_FOLDER}Fonts/digital-7.ttf", "digital");

// Set the global default font used by all components
Engine.setGlobalFont("regular");
```
```json:testMetadata:load-multiple-font-weights
{
  "testable": false,
  "skipReason": "Requires font files in the project's Images folder. loadFontAs is a no-op in compiled plugins."
}
```

**Pitfalls:**
- Font files go in the Images folder, not a "Fonts" subfolder at the project root. The `{PROJECT_FOLDER}` prefix resolves to the Images directory. A subfolder within Images (like `Images/Fonts/`) is fine and commonly used.
- In compiled plugins, `loadFontAs` is a no-op because fonts are baked into the binary at export time. The calls are harmless but unnecessary in the frontend.
