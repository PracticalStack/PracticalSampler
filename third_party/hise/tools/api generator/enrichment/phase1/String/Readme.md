# String -- Class Analysis

## Brief
Built-in string type with search, split, transform, regex, encryption, and parsing methods.

## Purpose
String is a built-in JavaScript engine type that wraps juce::String, providing string manipulation methods directly on string values. Unlike scripting API classes, it is registered as a native object prototype in the HiseJavascriptEngine constructor, so its methods are automatically available on any string literal or variable. All methods are pure functions that return new values without modifying the original string. The class provides a mix of JavaScript-compatible methods (indexOf, split, trim) and HISE-specific additions (encrypt/decrypt, splitCamelCase, hash, getTrailingIntValue).

## obtainedVia
Built-in type -- any string literal or variable (e.g. `"hello"` or `var s = "text"`)

## minimalObjectToken
s

## Constants
None.

## Dynamic Constants
None.

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| `"abc".split("::")` expecting split on `::` | `"abc".split(":")` using single char | `split()` only uses the first character of the separator string. Multi-character separators are silently truncated. |
| `"abc".replace("a", "x")` expecting only first replaced | Use `replace()` knowing all are replaced | Unlike JavaScript, HISE `replace()` replaces ALL occurrences. `replace` and `replaceAll` are identical. |

## codeExample
```javascript
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
```

## Alternatives
None.

## Related Preprocessors
None.

## Diagnostic Ideas
Reviewed: Yes
Count: 0
Rationale: String methods are pure functions with no preconditions, timeline dependencies, or silent failure modes that would benefit from parse-time diagnostics.
