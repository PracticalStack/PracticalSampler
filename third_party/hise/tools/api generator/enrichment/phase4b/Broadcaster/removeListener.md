Broadcaster::removeListener(var idFromMetadata) -> Integer

Thread safety: UNSAFE -- OwnedArray::removeObject deallocates, metadata construction involves string hashing
Removes the first listener target matching the metadata ID. Returns true if found, false
otherwise. Matching is by metadata hash (hashCode64 of ID string), not object reference.
Dispatch/mechanics:
  Constructs Metadata from parameter, matches by hash (hashCode64 of ID string).
  Removes first matching TargetBase from items OwnedArray.
Pair with:
  addListener -- registration
  removeAllListeners -- bulk removal
Anti-patterns:
  - Must pass the metadata string/object, not the original object or function reference.
  - Returns false silently on no match -- no error or warning.
Source:
  ScriptBroadcaster.cpp  removeListener()
