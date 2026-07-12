ScriptedViewport::setLocalLookAndFeel(ScriptObject lafObject) -> undefined

Thread safety: UNSAFE
Attaches a scripted look-and-feel object to this component and all children. Pass false to clear. For table mode: drawTableRowBackground, drawTableCell, drawTableHeaderBackground, drawTableHeaderColumn. For all modes: drawScrollbar.
Required setup:
  const var vp = Content.getComponent("ViewportId");
  const var laf = Content.createLocalLookAndFeel();
  laf.registerFunction("drawScrollbar", function(g, obj) { /* ... */ });
  vp.setLocalLookAndFeel(laf);
Dispatch/mechanics: Stores LAF reference, propagates to all child components. If LAF uses CSS (has stylesheet), automatically calls setStyleSheetClass({}) to initialize the class selector.
Pair with: setStyleSheetClass, setStyleSheetProperty, setStyleSheetPseudoState (CSS styling companions)
Source:
  ScriptingApiContent.cpp  ScriptComponent::setLocalLookAndFeel() -> propagateToChildren()
