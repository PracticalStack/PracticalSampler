Broadcaster::removeAllSources() -> undefined

Thread safety: UNSAFE -- OwnedArray::clear() deallocates all sources, destructors unregister from upstream
Removes all attached event sources. Targets (items) are not affected.
Each source destructor unregisters from its upstream provider automatically.
Dispatch/mechanics:
  Destroys all ListenerBase objects. Each destructor unregisters from upstream providers
  (Processor::AttributeListener, LambdaBroadcaster, etc.).
Pair with:
  removeSource -- selective removal by metadata
  removeAllListeners -- remove all targets
Source:
  ScriptBroadcaster.cpp  OwnedArray<ListenerBase>::clear()
