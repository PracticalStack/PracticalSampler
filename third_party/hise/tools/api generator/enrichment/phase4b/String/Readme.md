String (object)
Obtain via: Built-in type -- any string literal or variable (e.g. "hello" or var s = "text")

Built-in string type wrapping juce::String with search, split, transform, regex,
encryption, and parsing methods. All methods are pure functions returning new values
without modifying the original string. Provides JavaScript-compatible methods
(indexOf, split, trim) alongside HISE-specific additions (encrypt/decrypt,
splitCamelCase, hash, getTrailingIntValue).

Complexity tiers:
  1. Basic text operations: contains, indexOf, split, replace, toLowerCase,
     toUpperCase, trim. Path parsing, search, display formatting.
  2. HISE-specific parsing: + getTrailingIntValue, splitCamelCase, capitalize,
     getIntValue. Structured UI component ID decomposition.
  3. Advanced: + hash for state fingerprinting, match for regex extraction,
     encrypt/decrypt for data obfuscation.

Practical defaults:
  - Use split("/") to parse hierarchical paths. Remember split only uses the
    first character of the separator.
  - Use toLowerCase() on both sides of a contains() check for case-insensitive
    search. There is no built-in case-insensitive search.
  - Name UI components with camelCase like "FilterAttack1" to leverage
    splitCamelCase() and getTrailingIntValue() for structured dispatch.
  - Use replace() knowing it replaces ALL occurrences. There is no "replace
    first only" variant in HiseScript.

Common mistakes:
  - Using multi-character separator in split() -- only the first character is
    used, rest silently ignored. "::".split("::") splits on ":" not "::".
  - Expecting replace() to replace only first match (JavaScript behavior) --
    HISE replace() replaces ALL occurrences. replace and replaceAll are identical.
  - Calling toLowerCase() repeatedly in a loop instead of caching -- each call
    allocates a new string.

Example:
  var s = "Hello World";

  // Search
  var idx = s.indexOf("World"); // 6
  var has = s.contains("Hello"); // true

  // Transform
  var upper = s.toUpperCase(); // "HELLO WORLD"
  var parts = s.split(" "); // ["Hello", "World"]

  // Parse trailing numbers
  var name = "Knob3";
  var num = name.getTrailingIntValue(); // 3

Methods (25):
  capitalize          charAt              charCodeAt
  concat              contains            decrypt
  encrypt             endsWith            getIntValue
  getTrailingIntValue hash                includes
  indexOf             lastIndexOf         match
  replace             replaceAll          slice
  split               splitCamelCase      startsWith
  substring           toLowerCase         toUpperCase
  trim
