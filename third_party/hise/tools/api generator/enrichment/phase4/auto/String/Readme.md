<!-- Diagram triage:
  - No diagrams specified in Phase 1 data
-->

# String

String is the built-in text type in HISEScript. Methods are available directly on any string literal or variable - there is no need to import or construct anything. You can call methods on literals (`"hello".toUpperCase()`) or on variables holding string values.

All String methods are pure functions that return new values without modifying the original string. The class provides three tiers of functionality:

1. **Standard text operations** - searching, splitting, replacing, case conversion, and trimming. These mirror familiar JavaScript methods with minor behavioural differences.
2. **HISE-specific parsing** - extracting trailing numbers from component IDs, splitting camelCase tokens, and converting strings to integers. These support the common pattern of encoding structured data in component names.
3. **Advanced utilities** - regex matching, hashing for state fingerprinting, and BlowFish encryption/decryption.

String methods that correspond to JavaScript built-ins (`indexOf`, `split`, `replace`) do not always behave identically to their JavaScript counterparts. In particular, `split` only uses the first character of the separator, and `replace` replaces all occurrences rather than just the first.

> All search and comparison methods (`contains`, `indexOf`, `startsWith`, `endsWith`) are case-sensitive. For case-insensitive matching, convert both strings to lowercase with `toLowerCase()` before comparing.

## Common Mistakes

- **Split uses only the first character of the separator**
  **Wrong:** `"a::b::c".split("::")` expecting split on `::`
  **Right:** `"a::b::c".split(":")` knowing only the first character is used
  *`split()` silently uses only the first character. Multi-character separators produce unexpected empty-string tokens instead of clean splits.*

- **Replace replaces all occurrences, not just the first**
  **Wrong:** `"a-b-c".replace("-", "_")` expecting only the first dash replaced
  **Right:** Use `replace()` knowing all occurrences are replaced
  *Unlike JavaScript's `replace`, HISEScript's version replaces every match. `replace` and `replaceAll` are identical.*

- **Substring requires two arguments**
  **Wrong:** `name.substring(5)` omitting the end index
  **Right:** `name.substring(5, 10000)` with an explicit large end index
  *HISEScript's `substring` does not reliably default the end index. Pass a large number to mean "rest of string" - HISE clamps it to the actual length.*

- **Cache toLowerCase results in loops**
  **Wrong:** Repeated `name.toLowerCase()` inside a loop body
  **Right:** `local lower = name.toLowerCase();` cached before the loop
  *Each call allocates a new string. Cache the result when checking the same string repeatedly.*
