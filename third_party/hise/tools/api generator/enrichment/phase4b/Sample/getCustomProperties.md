Sample::getCustomProperties() -> JSON

Thread safety: UNSAFE -- allocates a DynamicObject on first call (heap allocation). Subsequent calls return the cached object.
Returns a mutable JSON object for attaching arbitrary key-value metadata to this
sample. Lazily created on first access. This data is transient -- NOT persisted
when the sample map is saved or exported.
Pair with:
  get/set -- for persistent sample properties; use getCustomProperties for transient analysis data
Anti-patterns:
  - Do NOT rely on custom properties surviving a sample map save/load -- they
    are transient scripting-side data only
Source:
  ScriptingApiObjects.cpp  getCustomProperties()
    -> if customObject.isObject(): return cached
    -> else: customObject = new DynamicObject(); return customObject
