Creates a new child component from a JSON definition and appends it as the last child of this component. Returns a ContainerChild reference to the new child. Position can be specified as a `bounds` array `[x, y, w, h]` or as individual `x`, `y`, `width`, `height` properties (defaults: 0, 0, 128, 50).

> [!Warning:Children are always appended] There is no API to insert a child at a specific index. New children are always added at the end of the child list.
