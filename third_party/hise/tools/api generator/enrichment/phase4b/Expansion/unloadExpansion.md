Expansion::unloadExpansion() -> undefined

Thread safety: UNSAFE -- delegates to ExpansionHandler which triggers expansion removal, listener notifications, and potential voice killing
Removes this expansion from the active expansion list. After calling, the expansion
will not appear in ExpansionHandler.getExpansionList() until restart or re-initialisation.
The current Expansion reference becomes invalid.

Required setup:
  const var e = Engine.createExpansionHandler().getExpansionList()[0];

Dispatch/mechanics:
  exp->getMainController()->getExpansionHandler().unloadExpansion(exp)
    -> removes from expansion list, notifies listeners
    -> WeakReference exp becomes null

Anti-patterns:
  - Do NOT use this Expansion reference after calling unloadExpansion() -- subsequent
    method calls will either throw "Expansion was deleted" or crash, depending on
    whether the specific method checks object validity.

Source:
  ScriptExpansion.cpp  ScriptExpansionReference::unloadExpansion()
    -> ExpansionHandler::unloadExpansion(exp)
    -> WeakReference nullified after removal
