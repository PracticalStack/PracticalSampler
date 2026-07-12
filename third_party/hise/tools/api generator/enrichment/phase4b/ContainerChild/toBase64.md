ContainerChild::toBase64(Integer includeValues) -> String

Thread safety: UNSAFE
Serializes this component's data properties and children into a Base64-encoded,
zstd-compressed string. When includeValues is true, runtime values of this
component and all descendants are also included. The resulting string can be
passed to fromBase64() to restore the state.
Dispatch/mechanics:
  Creates ValueTree named after component id
    -> deep copy of componentData as child
    -> if includeValues: collects all descendant values into "Values" child
    -> ZDefaultCompressor compress -> base64 encode
Pair with:
  fromBase64 -- restores the state produced by toBase64
  addStateToUserPreset -- automatic preset integration using the same mechanism
Source:
  ScriptingApiContent.cpp  ChildReference::toBase64()
    -> componentData deep copy
    -> recursive value collection from Values tree
    -> ZDefaultCompressor -> base64 string
