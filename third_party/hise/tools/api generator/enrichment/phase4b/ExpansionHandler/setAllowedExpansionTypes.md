ExpansionHandler::setAllowedExpansionTypes(var typeList) -> undefined

Thread safety: UNSAFE -- modifies expansion handler state, may trigger reinitialization.
Restricts which expansion types can load. Expansions with disallowed types move to
the uninitialised list and become invisible to getExpansionList().
Required setup:
  const var eh = Engine.createExpansionHandler();
  eh.setAllowedExpansionTypes([eh.Intermediate, eh.Encrypted]);
Dispatch/mechanics:
  ExpansionHandler::setAllowedExpansions(typeArray)
    -> checkAllowedExpansions() called during discovery, reinit, and rebuild
    -> disallowed expansions get failed Result, moved to uninitialisedExpansions
Anti-patterns:
  - Do NOT pass a single integer instead of an array -- triggers script error.
    Always wrap in an array: [eh.Encrypted], not eh.Encrypted
Source:
  ScriptExpansion.cpp:1398  setAllowedExpansionTypes()
    -> ExpansionHandler::setAllowedExpansions()
    -> checkAllowedExpansions() (ExpansionHandler.cpp:677)
