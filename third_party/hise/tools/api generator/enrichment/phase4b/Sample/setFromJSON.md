Sample::setFromJSON(var object) -> undefined

Thread safety: UNSAFE -- iterates properties calling setSampleProperty, same ValueTree operations as set().
Sets multiple sample properties from a JSON object. Keys must use SampleIds
identifier names ("Root", "HiKey", "LoVel"), not integer indices. Cascading
range adjustments apply per property in iteration order.
Pair with:
  get -- read back property values after batch set
  getRange -- query valid bounds before setting interdependent properties
Anti-patterns:
  - [BUG] Non-object input (arrays, strings, numbers) is silently ignored --
    no error reported and no properties changed
  - Iteration order matters for interdependent properties due to cascading
    range adjustments
Source:
  ScriptingApiObjects.cpp  setFromJSON()
    -> iterates DynamicObject properties
    -> calls setSampleProperty(prop.name, prop.value) for each
