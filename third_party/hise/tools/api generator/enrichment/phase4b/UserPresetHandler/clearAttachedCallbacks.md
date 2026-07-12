UserPresetHandler::clearAttachedCallbacks() -> undefined

Thread safety: SAFE
Removes all automation callbacks attached via attachAutomationCallback.
Deregisters from the dispatch system and releases callback holders.
Also called automatically when the UserPresetHandler is destroyed.
Pair with:
  attachAutomationCallback -- the callbacks being cleared
Source:
  ScriptExpansion.cpp  clearAttachedCallbacks()
    -> clears internal AttachedCallback list
