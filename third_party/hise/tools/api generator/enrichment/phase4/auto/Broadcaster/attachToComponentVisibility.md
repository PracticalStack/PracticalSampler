Registers the broadcaster as a source that fires whenever the effective visibility of any of the specified components changes. Visibility is checked recursively through the parent hierarchy - a component is considered visible only when it and all of its ancestors are visible. Hiding a parent panel triggers visibility change events for all watched children.

> [!Warning:First argument is component ID string] The first broadcast argument is the component's string ID, not a component reference. This differs from `attachToComponentValue` which passes the component reference object.
