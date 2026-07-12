ScriptWebView::setLocalLookAndFeel(ScriptObject lafObject) -> undefined

Thread safety: UNSAFE
Attaches a scripted look and feel object to this component and all its children.
Pass false to clear. ScriptWebView renders content through the embedded browser,
so LAF primarily affects the component wrapper frame.
Anti-patterns:
  - Propagates to ALL child components automatically
  - If the LAF uses CSS, automatically initializes the class selector
Pair with:
  setStyleSheetClass -- set CSS class selectors
  setStyleSheetProperty -- set CSS variables
Source:
  ScriptingApiContent.cpp  ScriptComponent::setLocalLookAndFeel() (base class)
