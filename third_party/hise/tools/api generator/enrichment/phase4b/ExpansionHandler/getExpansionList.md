ExpansionHandler::getExpansionList() -> Array

Thread safety: UNSAFE -- allocates ScriptExpansionReference wrappers for each expansion.
Returns an array of Expansion references for all successfully initialized expansion
packs, sorted alphabetically. Expansions that failed initialization are excluded.
Pair with:
  setAllowedExpansionTypes -- filter which types appear in the list
  refreshExpansions -- rescan for new expansions before listing
Source:
  ScriptExpansion.cpp  getExpansionList()
    -> iterates expansionList OwnedArray
    -> wraps each as ScriptExpansionReference
