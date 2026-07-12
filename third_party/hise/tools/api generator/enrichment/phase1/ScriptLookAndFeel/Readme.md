# ScriptLookAndFeel -- Class Analysis

## Brief
Customizable rendering override for UI components via script paint functions or CSS stylesheets.

## Purpose
ScriptLookAndFeel overrides the default rendering of standard HISE UI components (buttons, sliders, comboboxes, preset browsers, keyboards, envelopes, etc.) using either registered JavaScript paint functions or CSS stylesheets. It acts as a service layer between the component system and the rendering pipeline. A global instance affects all components; local instances can be assigned per-component for scoped styling. The two rendering modes (script functions and CSS) can be combined on local LAFs, with per-function fallback from script to CSS.

## Details

### Rendering Modes

ScriptLookAndFeel operates in three rendering configurations:

| Mode | Activation | LAF Type Created |
|------|-----------|-----------------|
| Script functions only | `registerFunction()` called, no CSS | `LocalLaf` (local) or `Laf` (global) |
| CSS only | `setInlineStyleSheet()` or `setStyleSheet()` called, no `registerFunction()` | `CSSLaf` |
| Combined | Both CSS and `registerFunction()` used | `CombinedLaf` |

In Combined mode, each draw operation checks whether a script function is registered for that specific operation. If registered, the script function runs; otherwise, CSS handles it. This allows mixing rendering approaches per-component-type.

### Paint Function Protocol

All draw functions follow the same protocol: register with `registerFunction()`, receive `(g, obj)` in the callback, and return nothing. Five data-returning functions receive only `(obj)` and must return a value. See `registerFunction()` for the full list of 62 predefined function names and callback signatures.

### Error Halting Behavior

If any registered paint function throws a script error, `lastResult` is set to failed and ALL subsequent paint calls are silently skipped until the script is recompiled. This prevents cascading errors from flooding the console during rendering.

### Threading Model

Paint functions execute on the message thread with a non-blocking try-read-lock on the `LookAndFeelRenderLock`. If the lock is unavailable (e.g., during script compilation), rendering is silently skipped. The destructor acquires a write lock on the same lock before clearing script references.

### Global vs Local Scope

| Scope | Factory | Affects |
|-------|---------|---------|
| Global | `Engine.createGlobalScriptLookAndFeel()` | All components without a local LAF |
| Local | `Content.createLocalLookAndFeel()` | Only components where `setLocalLookAndFeel()` is called (propagates to children) |

`Engine.createGlobalScriptLookAndFeel()` returns the existing global LAF if one is already set, preventing duplicate globals.

### CSS Stylesheet Details

CSS can be applied via `setStyleSheet()` (external file) or `setInlineStyleSheet()` (inline string). Dynamic CSS variables are injected with `setStyleSheetProperty()`. See those methods for details on file loading, live editing, and type conversion. CSS is parsed by `simple_css::Parser` and stored as a `StyleSheet::Collection`.

### Image Loading

Images are loaded via `loadImage()`, checked with `isImageLoaded()`, and released with `unloadAllImages()`. See those methods for details on the image lifecycle.

### Font Override

`setGlobalFont()` overrides fonts for JUCE LookAndFeel methods only (alert windows, popup menus, combo boxes, text buttons). It does NOT affect fonts inside `registerFunction()` paint callbacks -- those use `g.setFont()` directly.

## obtainedVia
`Engine.createGlobalScriptLookAndFeel()` (global) or `Content.createLocalLookAndFeel()` (local)

## minimalObjectToken
laf

## Constants
None.

## Dynamic Constants
None.

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| `laf.registerFunction("drawKnob", cb)` | `laf.registerFunction("drawRotarySlider", cb)` | Function names must exactly match the predefined list (65 names). Invalid names are silently ignored -- the default rendering is used instead. |
| `laf.setStyleSheet("styles")` | `laf.setStyleSheet("styles.css")` | The file must have the `.css` extension or a script error is thrown. |

## codeExample
```javascript
// Global LAF with a custom rotary slider
const var laf = Engine.createGlobalScriptLookAndFeel();

laf.registerFunction("drawRotarySlider", function(g, obj)
{
    var a = obj.area;
    g.setColour(obj.bgColour);
    g.fillEllipse(a);
    g.setColour(obj.itemColour1);
    g.drawArc(a, -2.5, obj.valueNormalized * 5.0 - 2.5, 3.0);
});
```

## Alternatives
- **ScriptPanel** -- Use ScriptPanel for fully custom-drawn components with their own paint routines; use ScriptLookAndFeel to customize how standard built-in components render.
- **Graphics** -- ScriptLookAndFeel defines which paint functions are called for components; Graphics is the drawing context passed into those paint functions.

## Related Preprocessors
`USE_BACKEND` (CSS file loading path), `HISE_USE_SCRIPT_RECTANGLE_OBJECT` (area property format), `HISE_INCLUDE_PROFILING_TOOLKIT` (paint routine profiling).

## Diagnostic Ideas
Reviewed: Yes
Count: 1
- ScriptLookAndFeel.registerFunction -- value-check (logged)
