Broadcaster::removeSource(var metadata) -> Integer

Thread safety: UNSAFE -- OwnedArray::removeObject deallocates, destructor unregisters from upstream
Removes the first attached source matching the metadata ID. Returns true if found, false
otherwise. Source destructor automatically unregisters from upstream provider.
Dispatch/mechanics:
  Constructs Metadata from parameter, matches by hash.
  Removes first matching ListenerBase. Destructor unregisters from upstream.
Pair with:
  removeAllSources -- bulk removal
Anti-patterns:
  - Must pass the metadata used at attachment time, not the source type or config.
  - Auto-generated metadata (when optionalMetadata was omitted) may be hard to discover.
Source:
  ScriptBroadcaster.cpp  removeSource()
