Broadcaster::removeAllListeners() -> undefined

Thread safety: UNSAFE -- OwnedArray::clear() deallocates all targets, destructors release callbacks and stop timers
Removes all registered listener targets. Sources (attachedListeners) are not affected.
Use removeAllSources to also detach sources.
Dispatch/mechanics:
  Destroys all TargetBase objects (ScriptTarget, DelayedItem, ComponentPropertyItem, etc.).
  Destructors release WeakCallbackHolder ref-counts and stop timers.
Pair with:
  removeListener -- selective removal by metadata
  removeAllSources -- remove all sources
Source:
  ScriptBroadcaster.cpp  OwnedArray<TargetBase>::clear()
