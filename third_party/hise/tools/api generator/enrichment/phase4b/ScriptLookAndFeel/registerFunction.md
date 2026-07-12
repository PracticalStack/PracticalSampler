ScriptLookAndFeel::registerFunction(String functionName, Function paintFunction) -> undefined

Thread safety: UNSAFE
Registers a custom paint function that overrides the default rendering for a specific UI
component type. 62 predefined function names are supported. If no function is registered
for a given operation, default JUCE LookAndFeel rendering is used.
Callback signature: f(Graphics g, Object obj) for draw functions; f(Object obj) returning a value for data functions
Required setup:
  const var laf = Content.createLocalLookAndFeel();
  laf.registerFunction("drawToggleButton", function(g, obj)
  {
      // g = Graphics context, obj = component-specific properties
  });
  myButton.setLocalLookAndFeel(laf);

Dispatch/mechanics:
  Stores function in DynamicObject by functionName, sets hasScriptFunctions=true.
  At paint time: callWithGraphics() acquires ScopedTryReadLock on LookAndFeelRenderLock
    -> looks up function by name -> creates/reuses pooled GraphicsObject
    -> injects obj with component properties (area, colours, interaction state)
    -> calls script function -> flushes draw actions to Graphics context
  If any paint function throws, lastResult is set to failed and ALL subsequent
  paint calls are skipped until recompilation.

Pair with:
  setInlineStyleSheet/setStyleSheet -- can combine CSS and script functions on same LAF
  setGlobalFont -- for JUCE LookAndFeel font methods (not paint callback fonts)

Anti-patterns:
  - Do NOT use invalid function names -- silently accepted and stored but never invoked.
    No error or warning produced. The component uses default rendering with no indication
    the name was wrong.
  - Do NOT pass a non-function as the second argument -- silently does nothing.
  - Data-returning functions (getIdealPopupMenuItemSize, getThumbnailRenderOptions,
    getAlertWindowMarkdownStyleData, createPresetBrowserIcons, getModulatorDragData)
    receive only (obj) and must return a value -- do NOT use (g, obj) signature for these.

Source:
  ScriptingGraphics.cpp:2664  ScriptedLookAndFeel::registerFunction()
    -> stores in functions DynamicObject
  ScriptingGraphics.cpp:2757  callWithGraphics() -- paint dispatch
    -> ScopedTryReadLock on LookAndFeelRenderLock
    -> function lookup -> GraphicsObject pool -> script call -> flush draw actions
