# ScriptFloatingTile

ScriptFloatingTile embeds pre-built HISE widgets - preset browser, virtual keyboard, meters, envelope graphs, filter displays, and more - into the scripted interface. Create one with `Content.addFloatingTile(name, x, y)` and configure it with `setContentData()`.

Unlike ScriptPanel, which requires custom paint routines, ScriptFloatingTile gives you access to complex built-in UI panels with minimal scripting. The trade-off is less control: you configure the panel through a JSON object rather than drawing it yourself.

The key method is `setContentData()`, which accepts a JSON object with a `"Type"` property that selects the panel type and optional configuration for colours, fonts, and panel-specific settings. Only frontend-available panel types can be used - backend-only panels like Console or ScriptEditor are not registered.

Common content types include `"PresetBrowser"`, `"Keyboard"`, `"PerformanceLabel"`, `"CustomSettings"`, `"AHDSRGraph"`, `"FilterDisplay"`, `"AudioAnalyser"`, and `"MarkdownPanel"`. See the `setContentData()` method documentation for the full list and JSON configuration format.

> [!Warning:Property changes trigger full reload] All visual property changes (colours, font, content type, data) trigger a complete content reload where the panel is destroyed and recreated. Avoid changing these properties at high frequency.

ScriptFloatingTile deactivates several inherited properties that are irrelevant for display-only widgets, including `saveInPreset` (defaults to false), `macroControl`, `min`, `max`, `text`, and `tooltip`.
