Builder::flush() -> undefined

Thread safety: UNSAFE -- calls MessageManager::callAsync which involves heap
  allocation for the lambda capture.
Finalizes all pending module tree changes by notifying the UI and patch browser.
Does nothing if already flushed. Must be called after create() or clear()
operations -- the destructor warns if forgotten.

Dispatch/mechanics:
  MessageManager::callAsync -> setIsRebuilding(false)
    -> sendRebuildMessage(true) on synth chain
    -> RebuildModuleList event to processor change handler

Pair with:
  create -- flush after all modules are created
  clear -- flush after clearing the tree

Source:
  ScriptingApiObjects.cpp  ScriptBuilder::flush()
    -> MessageManager::callAsync lambda
    -> sendRebuildMessage(true) + RebuildModuleList event
