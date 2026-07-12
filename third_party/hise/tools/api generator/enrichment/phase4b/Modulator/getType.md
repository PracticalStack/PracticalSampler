Modulator::getType() -> String

Thread safety: WARNING -- String return involves atomic ref-count operations.
Returns the C++ type name of the modulator (e.g., "LFO", "AHDSR", "Velocity",
"Constant"). This is the module type identifier, not the user-assigned name.

Pair with:
  getId -- returns the user-assigned name, while getType returns the module type

Source:
  ScriptingApiObjects.cpp  getType() -> mod->getType().toString()
