Content::createLocalLookAndFeel() -> ScriptObject

Thread safety: UNSAFE -- heap-allocates a ScriptedLookAndFeel object and registers debug info listeners in backend builds.
Creates a local Look and Feel object for customizing individual UI component appearance.
Only applies to components explicitly assigned via component.setLocalLookAndFeel(laf).
Register drawing functions on the returned object with laf.registerFunction().

Required setup:
  const var laf = Content.createLocalLookAndFeel();
  laf.registerFunction("drawToggleButton", function(g, obj) { ... });
  myButton.setLocalLookAndFeel(laf);

Dispatch/mechanics:
  new ScriptedLookAndFeel(getScriptProcessor(), false)
  false = local (not global)
  Backend: registers LafRegistry debug info listener

Pair with:
  ScriptLookAndFeel.registerFunction -- register draw callbacks on the LAF
  ScriptComponent.setLocalLookAndFeel -- assign the LAF to a component
  Engine.createGlobalScriptLookAndFeel -- for global (all-component) styling

Anti-patterns:
  - Do NOT create LAF objects inside paint routines or callbacks -- allocates on
    every call. Create once at init scope and reuse.

Source:
  ScriptingApiContent.cpp:8467  Content::createLocalLookAndFeel()
    -> new ScriptedLookAndFeel(getScriptProcessor(), false)
    -> Backend: LafRegistry debug info listener registration
