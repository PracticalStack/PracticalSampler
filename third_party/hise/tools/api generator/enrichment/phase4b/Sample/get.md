Sample::get(int propertyIndex) -> var

Thread safety: WARNING -- string involvement when accessing FileName property (atomic ref-count on String copy). Integer properties are safe.
Returns the value of the specified sample property. FileName (index 1) returns
a String. All other properties are cast to int before returning.
Pair with:
  set -- write back modified property values
  getRange -- query valid bounds for the property
Anti-patterns:
  - Do NOT expect float returns for Volume or Pan -- all non-FileName properties
    are cast to int regardless of internal storage type
Source:
  ScriptingApiObjects.cpp  get()
    -> sampleIds[propertyIndex] -> sound->getSampleProperty(id)
    -> if FileName: return string; else: return (int)v
