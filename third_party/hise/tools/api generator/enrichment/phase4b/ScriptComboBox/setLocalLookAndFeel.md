ScriptComboBox::setLocalLookAndFeel(ScriptObject lafObject) -> undefined

Thread safety: UNSAFE
Attaches a scripted look and feel object to this component and all its children.
Pass undefined to clear. The LAF function for ScriptComboBox is drawComboBox.
Dispatch/mechanics:
  Sets localLookAndFeel on this component
    -> iterates all child ScriptComponent instances, sets their LAF too
    -> if LAF uses CSS, initializes setStyleSheetClass({}) and colour properties
Pair with:
  setStyleSheetClass -- CSS class selectors when using CSS-based LAF
Anti-patterns:
  - Styling only drawComboBox without also registering drawPopupMenuBackground and
    drawPopupMenuItem on the same LAF -- the popup menu renders with default styling
    while the closed box looks custom.
Source:
  ScriptingApiContent.h  ScriptComponent::setLocalLookAndFeel()
