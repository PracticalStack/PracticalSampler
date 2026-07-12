Parameter::getId() -> String

Thread safety: WARNING -- String return involves atomic ref-count operations.
Returns the name of the parameter as defined in the node's parameter tree.

Source:
  NodeBase.cpp  Parameter::getId()
    -> data[PropertyIds::ID].toString()
