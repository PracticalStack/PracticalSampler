# String -- Project Context

## Project Context

### Real-World Use Cases
- **File path and metadata parsing**: Plugins that load samples, presets, or IR files parse path strings with `split("/")` to extract folder hierarchy, file names, and categories. Combined with `substring` and `replace`, this forms the backbone of content browser UIs.
- **Component ID decomposition**: Plugins with numbered UI controls (e.g., "FilterAttack1", "MixerVolume3") use `getTrailingIntValue` and `splitCamelCase` to extract the parameter type and channel index from a single component ID string, enabling one callback to handle all instances.
- **Display name formatting**: Raw identifiers from file names or internal IDs are transformed into user-facing labels through `replace("_", " ")` chains followed by `capitalize()` or `toUpperCase()`.
- **Search filtering**: Preset browsers and sound browsers implement case-insensitive search by converting both the search term and target to `toLowerCase()` and then using `contains()`.
- **State change detection**: `hash()` is used to compute a fingerprint of composite state (e.g., concatenated control values) so that expensive operations like loading audio files are skipped when the state has not changed.

### Complexity Tiers
1. **Basic text operations** (most common): `contains`, `indexOf`, `split`, `replace`, `toLowerCase`, `toUpperCase`, `trim`. Used in nearly every plugin for path parsing, search, and display formatting.
2. **HISE-specific parsing** (common in structured UIs): `getTrailingIntValue`, `splitCamelCase`, `capitalize`, `getIntValue`. Used when component naming conventions encode structured information (type + index).
3. **Advanced** (specialized use cases): `hash` for state fingerprinting, `match` for regex extraction, `encrypt`/`decrypt` for data obfuscation.

### Practical Defaults
- Use `split("/")` to parse hierarchical paths like sample map IDs. Remember that `split` only uses the first character of the separator.
- Use `toLowerCase()` on both sides of a `contains()` check for case-insensitive search. There is no built-in case-insensitive search.
- Name UI components with a camelCase convention like "FilterAttack1" to leverage `splitCamelCase()` and `getTrailingIntValue()` for structured dispatch in callbacks.
- When using `substring` to get "the rest of the string", pass a large number as the end index (e.g., `substring(5, 10000)`) since HISE clamps it to the string length.
- Use `replace` knowing it replaces ALL occurrences. There is no "replace first only" variant in HiseScript.

### Integration Patterns
- `String.split()` -> `parseInt()` -- Splitting a path or version string and parsing numeric tokens is the most common multi-step string pattern.
- `String.getTrailingIntValue()` -> array index lookup -- Extract the trailing number from a component ID to index into a parallel array of processors or data objects.
- `String.splitCamelCase()` -> object property lookup -- Decompose a camelCase component ID into tokens, then use the tokens as keys into a structured object.
- `String.toLowerCase()` -> `String.contains()` -- Case-insensitive search pattern used in preset browsers and sound browsers.
- `String.replace()` chain -> `String.capitalize()` -- Transform raw file names into display-ready labels.
- `String.hash()` -> equality check -- Compute a fingerprint of concatenated state to skip redundant expensive operations.

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| `path.split("::")` expecting split on `::` | `path.split(":")` knowing only first char is used | `split()` silently uses only the first character. Multi-character separators do not work. |
| `name.substring(5)` omitting end index | `name.substring(5, 10000)` with explicit large end | HiseScript `substring` requires two arguments. Omitting the end index may not behave as expected. Pass a large number to mean "rest of string". |
| Repeated `name.toLowerCase()` in a loop | `local lower = name.toLowerCase();` cached before loop | Each call allocates a new string. Cache the result when checking the same string repeatedly. |
