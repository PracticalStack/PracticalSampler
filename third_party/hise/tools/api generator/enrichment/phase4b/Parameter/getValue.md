Parameter::getValue() -> Double

Thread safety: SAFE
Returns the current parameter value. If dynamicParameter is active (node connected),
returns dynamicParameter->getDisplayValue(). Otherwise falls back to the Value
property in the parameter's ValueTree.

Source:
  NodeBase.cpp:1063  Parameter::getValue()
    -> dynamicParameter->getDisplayValue() if dynamicParameter != nullptr
    -> fallback: data[PropertyIds::Value]
