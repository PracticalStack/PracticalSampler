ScriptLookAndFeel (object)
Obtain via: Engine.createGlobalScriptLookAndFeel() (global) or Content.createLocalLookAndFeel() (local)

Customizable rendering override for UI components via script paint functions or CSS
stylesheets. Overrides default rendering of standard HISE components (buttons, sliders,
comboboxes, preset browsers, keyboards, envelopes, etc.) using registered JavaScript
paint functions, CSS stylesheets, or both combined.

Complexity tiers:
  1. Minimal: registerFunction with 1-3 functions. Customize only components that look
     out of place with default rendering (popup menus, preset browser items).
  2. Centralized namespace: 10-22 LAF objects organized in a namespace, exported for
     use by feature files. A register() helper attaches shared popup menu draw functions.
  3. Per-section theming: + loadImage, setGlobalFont. 5-10 LAF objects where each
     represents a distinct visual identity. One global LAF for shared elements, multiple
     local LAFs for section-specific rendering.
  4. CSS + scripted combined: + setInlineStyleSheet/setStyleSheet, setStyleSheetProperty.
     CSS for discrete-state components (buttons, combo boxes), scripted functions for
     continuous-data rendering (knobs, filter graphs, envelope displays).

Practical defaults:
  - Use Content.createLocalLookAndFeel() as the default. Reserve
    Engine.createGlobalScriptLookAndFeel() for popup menus, alert windows, number tags,
    and other system-level components that cannot have a local LAF attached.
  - drawToggleButton is the most commonly registered function because buttons have the
    most visual variation (icon buttons, toggles, page selectors, solo/mute, radio groups).
  - Always use scripted LAF (not CSS) for rotary sliders. Arc geometry requires
    Path.addArc() with angles computed from obj.valueNormalized -- fundamentally
    procedural, cannot be replicated in CSS.
  - For CSS + scripted combined LAFs, use CSS for discrete-state components and scripted
    functions for continuous-data rendering.
  - Pre-compute alpha constants (const var WHITE_55 = Colours.withAlpha(Colours.white, 0.55))
    and reference them in draw functions to avoid repeated computation during rendering.

Common mistakes:
  - Using an invalid function name in registerFunction() -- silently ignored, default
    rendering used, no error or warning produced.
  - Using CSS-only for rotary knobs -- rotary sliders require Path.addArc() with angles
    from obj.valueNormalized. CSS cannot express procedural arc geometry.
  - Calling Content.createLocalLookAndFeel() inside a callback -- create LAF objects once
    at init time with const var. Creating inside callbacks wastes resources and loses
    registered functions.
  - Using laf.setGlobalFont() and expecting it to affect registerFunction paint callbacks
    -- setGlobalFont() only affects JUCE LookAndFeel methods (alert windows, popup menus,
    text buttons). Paint callbacks control their own fonts via g.setFont().
  - Omitting the .css extension in setStyleSheet() -- throws a script error.
  - Registering all draw functions on a single global LAF -- makes per-section theming
    impossible. Use multiple local LAFs assigned to specific components.

Example:
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

Methods (8):
  isImageLoaded          loadImage
  registerFunction       setGlobalFont
  setInlineStyleSheet    setStyleSheet
  setStyleSheetProperty  unloadAllImages
