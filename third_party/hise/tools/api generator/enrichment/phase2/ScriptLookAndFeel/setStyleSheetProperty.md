## setStyleSheetProperty

**Examples:**

```javascript:dynamic-css-icons-theme
// Title: Dynamic CSS variables for icons and theme colours
// Context: CSS variables injected from HiseScript enable dynamic theming
// and per-component icon assignment without rewriting the stylesheet.

const var laf = Content.createLocalLookAndFeel();
laf.setStyleSheet("main.css");

// Inject a Path as a CSS background-image variable
const var dropdownIcon = Content.createPath();
dropdownIcon.loadFromData("84.t0VXfyBQ...");  // base64 path data
laf.setStyleSheetProperty("dropDown", dropdownIcon, "path");

// Inject a colour variable for theming
laf.setStyleSheetProperty("accentColor", 0xFFFF4444, "color");

// Inject a pixel size
laf.setStyleSheetProperty("borderRadius", 5.0, "px");

// In the CSS file, reference these variables:
// select::after { background-image: var(--dropDown); }
// button:checked { background: var(--accentColor); }
// .panel { border-radius: var(--borderRadius); }
```

```json:testMetadata:dynamic-css-icons-theme
{
  "testable": false,
  "skipReason": "Requires external CSS file and visual rendering cannot be verified"
}
```

```javascript:per-component-css-variable
// Title: Per-component CSS variable injection via setStyleSheetClass
// Context: Components can also inject CSS variables at the component level
// using setStyleSheetProperty on the component itself, complementing the
// LAF-level variables set here. This example shows the LAF-level pattern
// for a page title injected as a CSS content property.

const var pageLaf = Content.createLocalLookAndFeel();
pageLaf.setStyleSheet("page.css");

// The page.css uses: .title { content: var(--title); }
// Set the variable on the LAF for a default value
pageLaf.setStyleSheetProperty("title", "Untitled", "");

const var titlePanel = Content.getComponent("TitleDisplay");
titlePanel.setLocalLookAndFeel(pageLaf);

// Override per-component using the component's own setStyleSheetProperty
titlePanel.setStyleSheetProperty("title", "Oscillator", "");
```

```json:testMetadata:per-component-css-variable
{
  "testable": false,
  "skipReason": "Requires external CSS file and pre-existing UI component"
}
```
