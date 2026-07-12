Sample::getId(int id) -> String

Thread safety: WARNING -- returns String from Identifier::toString(), atomic ref-count on StringHolder.
Returns the string identifier name for a property index. For example,
Sample.Root (2) returns "Root". Reverse mapping of bracket-access string
resolution. Useful for constructing setFromJSON objects programmatically.
Note: No Wrapper struct entry or ADD_API_METHOD registration found -- may not
be callable in all builds.
Pair with:
  setFromJSON -- use returned strings as property keys in JSON objects
  get/set -- integer-index counterparts for direct property access
Anti-patterns:
  - Do NOT pass indices outside 1-23 -- no bounds checking on the sampleIds
    array, out-of-range access reads arbitrary memory
Source:
  ScriptingApiObjects.cpp  getId()
    -> sampleIds[id].toString()
