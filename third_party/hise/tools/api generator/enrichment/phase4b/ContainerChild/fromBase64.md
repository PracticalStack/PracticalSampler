ContainerChild::fromBase64(String b64) -> undefined

Thread safety: UNSAFE
Restores this component's properties, children, and optionally values from a
Base64-encoded string produced by toBase64(). Restoration is deferred via
SafeAsyncCall -- the component tree is not updated immediately upon return.
Dispatch/mechanics:
  ZDefaultCompressor decompress(base64)
    -> extract "Component" child -> copy properties/children into componentData
    -> extract "Values" child -> copy properties into Values tree
    -> deferred via SafeAsyncCall
Pair with:
  toBase64 -- produces the string that fromBase64 consumes
Anti-patterns:
  - Do NOT read component state immediately after fromBase64() -- restoration is
    deferred, so reads may return stale data until the async call completes.
Source:
  ScriptingApiContent.cpp  ChildReference::fromBase64()
    -> ZDefaultCompressor decompress
    -> SafeAsyncCall deferred property/child restoration
    -> uses undo manager if useUndoManager=true
