ContainerChild::get(String id) -> NotUndefined

Thread safety: UNSAFE
Returns the value of the specified property. Falls back to the default value if
the property has not been explicitly set. Throws a script error if the property
name is not in the valid property list.
Pair with:
  set -- writes the property that get() reads
Anti-patterns:
  - Do NOT use dot-read syntax (cc.text) when you need default fallback -- dot-read
    returns the raw ValueTree property (possibly undefined). Use get() instead.
Source:
  ScriptingApiContent.cpp  ChildReference::get()
    -> dyncomp::dcid::Helpers::isValidProperty(id) validation
    -> componentData.getProperty(id, defaultValue) with default fallback
