ScriptComboBox::getValue() -> var

Thread safety: SAFE
Returns the current value as a 1-based integer index. Value 1 = first item,
value 0 = nothing selected (placeholder text shown).
Pair with:
  setValue -- set the selected item by index
  getItemText -- get display text of current selection
Anti-patterns:
  - Do NOT assume value is a String -- the stored value must be numeric. An assertion
    fires in debug builds if a String is stored.
Source:
  ScriptingApiContent.h  ScriptComponent::getValue()
    -> returns numeric value field directly (no override in ScriptComboBox)
