Checks whether a connection between the given source and target can be established. Returns `true` if no connection between them currently exists, `false` if a connection already exists.

> [!Warning:False for unknown source IDs] Returns `false` for unrecognised source IDs without throwing an error, making it indistinguishable from "connection already exists". Verify source IDs against `getSourceList()` if you need to differentiate.
