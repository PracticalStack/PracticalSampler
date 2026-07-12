ScriptDynamicContainer::setLocalLookAndFeel(ScriptObject lafObject) -> undefined

Thread safety: UNSAFE
Attaches a scripted look and feel object to the container and propagates it to all
regular ScriptComponent children. Pass undefined to clear. Does NOT affect dyncomp
children created via setData() -- those use CSS styling via class/elementStyle.
Anti-patterns:
  - Do NOT expect LAF to propagate to dyncomp children -- only ScriptComponent
    children (parentComponent-based) receive the LAF.
Pair with:
  setStyleSheetClass -- CSS class for dyncomp children instead
Source:
  ScriptingApiContent.cpp  ScriptComponent::setLocalLookAndFeel()
