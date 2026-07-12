<!-- Diagram triage:
  - No diagrams specified in Phase 1 data
-->

# ScriptLookAndFeel

ScriptLookAndFeel overrides the default rendering of standard HISE UI components using either registered JavaScript paint functions or CSS stylesheets. It supports three rendering configurations:

1. **Script functions only** - register custom paint callbacks via `registerFunction()`
2. **CSS only** - apply stylesheets via `setStyleSheet()` or `setInlineStyleSheet()`
3. **Combined** - use both on the same LAF object; each draw operation checks for a registered script function first, falling back to CSS

A ScriptLookAndFeel operates at one of two scope levels:

- **Global** (`Engine.createGlobalScriptLookAndFeel()`) - affects all components without a local LAF. Use for system-level elements like popup menus, alert windows, and number tags.
- **Local** (`Content.createLocalLookAndFeel()`) - affects only components where `setLocalLookAndFeel()` is called. Propagates to child components automatically.

```javascript
const var laf = Content.createLocalLookAndFeel();
myKnob.setLocalLookAndFeel(laf);
```

Images for use in paint callbacks are loaded with `loadImage()` and drawn via `g.drawImage()` inside registered functions. `setGlobalFont()` sets the font for built-in rendering (popup menus, alert windows) but does not affect fonts inside paint callbacks - use `g.setFont()` there instead.

To get started with custom rendering, right-click a component in the Interface Designer and choose **Create LocalLookAndFeel for selection**. This generates the boilerplate LAF assignment code.

> If any registered paint function throws a script error, all subsequent paint calls are silently skipped until the script is recompiled. This prevents cascading errors during rendering but means a single broken function disables all custom rendering across the entire LAF.

## Common Mistakes

- **Wrong:** `laf.registerFunction("drawKnob", cb)`
  **Right:** `laf.registerFunction("drawRotarySlider", cb)`
  *Function names must exactly match the predefined list. Invalid names are silently ignored - the default rendering is used with no error message.*

- **Wrong:** Using CSS-only for rotary knobs
  **Right:** Use scripted `registerFunction("drawRotarySlider", ...)`
  *Rotary sliders require `Path.addArc()` with angles derived from `obj.valueNormalized`. This is procedural arc geometry that CSS cannot express.*

- **Wrong:** Registering all draw functions on a single global LAF
  **Right:** Use multiple local LAFs assigned to specific components
  *A monolithic global LAF prevents different visual styles for different UI sections. Local LAFs provide per-component scoping and enable per-section theming.*

- **Wrong:** Calling `Content.createLocalLookAndFeel()` inside a callback
  **Right:** Create LAF objects at init time with `const var laf = Content.createLocalLookAndFeel()`
  *LAF objects should be created once at script initialisation and stored in `const var`. Creating them inside callbacks wastes resources and loses registered functions.*

- **Wrong:** Using `laf.setGlobalFont()` and expecting it to affect `registerFunction` paint callbacks
  **Right:** Use `g.setFont()` inside each paint callback
  *`setGlobalFont()` only affects built-in rendering (alert windows, popup menus, text buttons). Paint callbacks control their own fonts via the Graphics context.*
