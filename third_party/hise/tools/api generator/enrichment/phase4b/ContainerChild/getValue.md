ContainerChild::getValue() -> NotUndefined

Thread safety: WARNING -- String involvement: constructs component ID string for Values tree lookup.
Returns this component's current value from the Values tree. If no value has
been set, returns the defaultValue property from the component's data.
Anti-patterns:
  - [BUG] Does not call isValidOrThrow() unlike sibling get() method. On an
    invalid reference, returns stale data without warning instead of throwing
    a script error.
Pair with:
  setValue -- writes the value that getValue() reads
  changed -- triggers the control callback after setValue()
Source:
  ScriptingApiContent.cpp  ChildReference::getValue()
    -> data->getValueTree(Values).getProperty(id, defaultValue)
    -> no isValidOrThrow() check (bug)
