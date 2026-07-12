# ScriptLookAndFeel -- Project Context

## Project Context

### Real-World Use Cases
- **Complete plugin UI theming**: A plugin that needs a branded, custom appearance registers draw functions for every control type (knobs, buttons, combo boxes, sliders) on one or more LAF objects. The global LAF handles system-level components (popup menus, alert windows, number tags) while local LAFs handle per-component styling. This is the dominant use case -- every commercial plugin with custom visuals uses this pattern.
- **Per-section visual identity**: A plugin with distinct UI modes or sections (e.g., different pages with different aesthetics) creates separate local LAF objects for each section, each with its own colour palette, font, and drawing style. A single global LAF provides shared elements (popup menus, dialogs) while local LAFs give each section a unique look. This allows a single plugin to contain multiple visual themes.
- **CSS + scripted combined rendering**: A plugin that wants CSS for state-driven components (buttons with hover/checked states, combo boxes, popup menus, keyboard keys) and scripted LAF for data-driven components (rotary knobs with arc paths, filter response curves, envelope visualizers) uses both `setStyleSheet()` and `registerFunction()` on the same LAF object. HISE creates a CombinedLaf that dispatches per-function: if a script function is registered, it runs; otherwise CSS renders.

### Complexity Tiers
1. **Minimal** (most common entry point): 1 global LAF with 1-3 functions -- typically `drawPopupMenuBackground`, `drawPopupMenuItem`, or `drawPresetBrowserListItem`. Customize only the components that look out of place with default rendering.
2. **Centralized namespace**: 10-22 LAF objects organized in a `namespace LookAndFeel` in a single file, exported for use by feature files via `LookAndFeel.fxButton`, `LookAndFeel.selectors`, etc. A `register()` helper function attaches shared popup menu draw functions to any LAF.
3. **Per-section theming**: 5-10 LAF objects where each represents a distinct visual identity (e.g., vintage knobs vs. LED displays vs. sci-fi arcs). One global LAF provides shared elements, multiple local LAFs provide section-specific rendering.
4. **Modular directory**: 70+ LAF objects organized in a dedicated `LookAndFeels/` directory with files by component type (`ButtonLAF.js`, `SliderLAF.js`, `ComboBoxLAF.js`). Each file contains a namespace with multiple named LAFs. Use this when the project has 20+ distinct LAF objects.

### Practical Defaults
- Use `Content.createLocalLookAndFeel()` with `setLocalLookAndFeel()` as the default. Reserve `Engine.createGlobalScriptLookAndFeel()` for popup menus, alert windows, number tags, and other system-level components that cannot have a local LAF attached.
- `drawToggleButton` is the most commonly registered function because buttons have the most visual variation (icon buttons, enable toggles, page selectors, solo/mute, radio groups).
- Always use scripted LAF (not CSS) for rotary sliders. Arc geometry requires `Path.addArc()` with angles computed from `obj.valueNormalized` -- this is fundamentally procedural and cannot be replicated in CSS.
- For CSS + scripted combined LAFs, use CSS for discrete-state components (buttons, combo boxes, popup menus) and scripted functions for continuous-data rendering (knobs, filter graphs, envelope displays).
- When a project has 10+ LAF objects, organize them in a centralized namespace. When it exceeds 20+, move to a dedicated directory with files by component type.
- Pre-compute alpha constants for common opacity levels (`const var WHITE_55 = Colours.withAlpha(Colours.white, 0.55);`) and reference them in draw functions to avoid repeated computation during rendering.

### Integration Patterns
- `Content.createLocalLookAndFeel()` -> `component.setLocalLookAndFeel(laf)` -- the standard assignment chain. The LAF propagates to child components automatically.
- `laf.registerFunction()` + `laf.setStyleSheet()` on the same object -- creates a CombinedLaf where each draw operation checks for a registered script function first, falling back to CSS.
- `Engine.loadFontAs()` -> `g.setFont("alias", size)` inside paint callbacks -- custom fonts must be loaded before use in LAF draw functions.
- `laf.loadImage()` -> `g.drawImage("alias", area, xOffset, yOffset)` inside paint callbacks -- filmstrip or background images loaded once, drawn per-frame.
- A reusable `register()` helper function that attaches popup menu draw functions (`drawPopupMenuBackground`, `drawPopupMenuItem`, `getIdealPopupMenuItemSize`) to any LAF object -- avoids duplicating popup menu rendering code across multiple LAFs.
- `component.setStyleSheetClass(".className")` + `component.setStyleSheetProperty("varName", value, "type")` -- CSS class and variable injection from HiseScript to drive CSS rendering dynamically.

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| Using CSS-only for rotary knobs | Use scripted `registerFunction("drawRotarySlider", ...)` | Rotary sliders require `Path.addArc()` with angles derived from `obj.valueNormalized`. This is procedural arc geometry that CSS cannot express. No shipping plugin achieves CSS-only rotary knobs. |
| Registering all draw functions on a single global LAF | Use multiple local LAFs assigned to specific components | A monolithic global LAF makes it impossible to have different visual styles for different UI sections. Local LAFs provide per-component scoping and enable per-section theming. |
| Calling `Content.createLocalLookAndFeel()` inside a callback | Create LAF objects at init time with `const var laf = Content.createLocalLookAndFeel()` | LAF objects should be created once at script initialization and stored in `const var`. Creating them inside callbacks wastes resources and loses registered functions. |
| Using `laf.setGlobalFont()` and expecting it to affect `registerFunction` paint callbacks | Use `g.setFont()` inside each paint callback | `setGlobalFont()` only affects JUCE LookAndFeel methods (alert windows, popup menus, text buttons). Paint callbacks control their own fonts via the Graphics context. |
