# Content -- Project Context

## Project Context

### Real-World Use Cases
- **Full-featured plugin interface**: The primary use case. Every HiseScript-based plugin calls `Content.makeFrontInterface()` as its first line and uses `Content.addKnob/addButton/addPanel/...` to construct the entire UI. Content is the root factory that every other UI pattern depends on.
- **LAF-heavy commercial plugin**: Plugins with 20-70+ distinct visual styles create many `Content.createLocalLookAndFeel()` objects, organized in dedicated LAF files (e.g., `SliderLAF.js`, `ButtonLAF.js`). Each LAF handles one visual style and is assigned to specific components via `setLocalLookAndFeel()`.
- **Icon/path library**: Plugins that need custom icons create a namespace with dozens of `Content.createPath()` calls, each loaded from base64 data. This produces a reusable icon dictionary that LAF draw functions and paint routines reference by name.
- **Zoom-capable interface**: Commercial plugins implement drag-to-zoom by combining `Content.getScreenBounds()` to determine the maximum safe zoom level with `Settings.setZoomLevel()` to apply it.

### Complexity Tiers
1. **Basic interface** (universal): `makeFrontInterface`, `addKnob/addButton/addPanel`, `getComponent`. Every plugin needs these. All component references should be cached as `const var` at init time.
2. **Styled interface** (common): Adds `createLocalLookAndFeel`, `createPath`, `setValuePopupData`. Most commercial plugins reach this tier for visual polish.
3. **Interactive interface** (intermediate): Adds `showModalTextInput`, `setKeyPressCallback`, `createMarkdownRenderer`, `getAllComponents` for batch operations. Plugins with preset naming, keyboard navigation, or rich tooltips use these.
4. **Advanced rendering** (specialized): Adds `createShader`, `createSVG`, `createScreenshot`, `setUseHighResolutionForPanels`. Only plugins with GPU shaders, SVG icons, or screenshot automation reach this tier.

### Practical Defaults
- Always call `Content.makeFrontInterface(width, height)` as the very first line of your interface script's `onInit`. Everything else depends on it.
- Use `Content.setUseHighResolutionForPanels(true)` immediately after `makeFrontInterface` if any ScriptPanel uses custom paint routines. Without this, panels look blurry on Retina/HiDPI displays.
- Cache every component reference as `const var` at init time. Never call `Content.getComponent()` repeatedly inside callbacks or timer functions - it performs a linear search each time.
- Use `Content.getAllComponents("Pattern.*")` to batch-retrieve components that share a naming convention, then iterate to assign a shared LAF or callback.
- Store `Content.createPath()` results in `const var` at namespace scope. Paths are immutable once loaded, so they should be created once and reused across paint calls.

### Integration Patterns
- `Content.createLocalLookAndFeel()` -> `ScriptLookAndFeel.registerFunction()` -> `ScriptComponent.setLocalLookAndFeel()` - The standard LAF pipeline. Create, register draw functions, then assign to components.
- `Content.createPath()` -> `Path.loadFromData()` -> `Graphics.fillPath()` - The icon pipeline. Create path, load from base64 data, then draw in paint routines or LAF callbacks.
- `Content.createMarkdownRenderer()` -> `MarkdownRenderer.setStyleData()` -> `MarkdownRenderer.setTextBounds()` -> `Graphics.drawMarkdownText()` - The rich text pipeline for tooltips, modals, and help systems.
- `Content.getScreenBounds(false)` -> `Settings.setZoomLevel()` - The zoom pipeline. Query available screen height, compute maximum safe zoom, then apply.
- `Content.setKeyPressCallback()` -> `Broadcaster.sendSyncMessage()` - Keyboard shortcuts that trigger broadcaster-driven actions. Register shortcuts in a dedicated file, each callback dispatching through a broadcaster.

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| `Content.getComponent("Knob1")` inside a timer callback | `const var knob1 = Content.getComponent("Knob1");` at init, use `knob1` in callback | `getComponent` performs a linear search through all components. Calling it repeatedly in callbacks wastes CPU. Cache the reference once at init time. |
| Creating paths inside paint routines | `const var icon = Content.createPath();` at init scope | `createPath` allocates a new object. Creating paths inside paint routines causes allocation on every repaint. Create once, reuse everywhere. |
| Scattering LAF objects across many files | Organize LAFs in dedicated files by component type (e.g., `SliderLAF.js`, `ButtonLAF.js`) | As the number of LAF objects grows (20+), keeping them organized by visual component type makes maintenance practical. |
| Using `Content.addKnob()` in `onControl` | All `addXXX()` calls must be in `onInit` | Content enforces a strict lifecycle: component creation is only allowed during `onInit`. Calling `addXXX()` after initialization throws a script error. |
