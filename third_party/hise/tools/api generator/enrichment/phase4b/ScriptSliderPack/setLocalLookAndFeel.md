ScriptSliderPack::setLocalLookAndFeel(ScriptObject lafObject) -> undefined

Thread safety: UNSAFE
Attaches or clears a local scripted look-and-feel object for this slider pack.

Dispatch/mechanics:
  Wrapper prefers local LAF implementing SliderPack::LookAndFeelMethods.
  If missing, wrapper falls back to global LookAndFeel implementation.

Anti-patterns:
  - Do NOT forget child impact -- local LAF propagation also affects child components.

Source:
  ScriptComponentWrappers.cpp:2499  SliderPackWrapper LAF selection and fallback path
