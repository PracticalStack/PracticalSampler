Modulator::getId() -> String

Thread safety: WARNING -- String return involves atomic ref-count operations.
Returns the user-assigned ID (name) of the modulator in the HISE module tree.
Returns empty string if the reference is invalid.

Source:
  ScriptingApiObjects.cpp  getId() -> mod->getId()
