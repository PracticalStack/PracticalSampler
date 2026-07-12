Loads path geometry from external data, replacing the current path contents. Accepts three input formats:

- **Base64 string** - compact binary encoding, typically used for icon libraries stored in script files
- **Byte array** - array of numeric values (0-255) representing binary path data, commonly exported from SVG conversion tools
- **Path object** - copies geometry directly from another Path, effectively cloning it

The format is detected automatically. For human-readable serialisation, use `fromString` instead.

`Content.createPath(data)` passes its argument directly to `loadFromData`, so the two-step pattern of creating a path and loading data can be collapsed into a single call. This is the recommended approach for icon libraries where each path is initialised from a constant base64 string.
