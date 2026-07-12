Builder::clear() -> undefined

Thread safety: UNSAFE -- acquires MessageManagerLock, performs heap deallocation,
  suspends global dispatch. In backend IDE, blocks 500ms for UI coordination.
Removes all modules from the MainSynthChain except the calling script processor.
Suspends global dispatch during demolition to prevent notification storms.
Sets flushed=false, requiring a subsequent flush() call.

Dispatch/mechanics:
  SUSPEND_GLOBAL_DISPATCH -> iterate MainSynthChain internal chains + direct children
    -> skip self (thisAsP) -> sendDeleteMessage() + raw::Builder::remove() per processor
    -> GlobalRoutingManager::removeUnconnectedSlots() cleanup

Pair with:
  flush -- must call after clear() to finalize UI state
  create -- typically follows clear() to build a new tree

Anti-patterns:
  - Do NOT rely on clear() during project load -- silently skips execution on
    the sample loading thread (logs a console message but returns without action).
    Subsequent create() calls will add to the existing tree, not a clean slate.

Source:
  ScriptingApiObjects.cpp:10190  ScriptBuilder::clear()
    -> MainController::ScopedBadBabysitter (threading bypass)
    -> SUSPEND_GLOBAL_DISPATCH macro
    -> raw::Builder::remove<Processor>() per child
    -> GlobalRoutingManager::removeUnconnectedSlots()
