# String -- C++ Source Exploration

## Resources Consulted
- `enrichment/resources/survey/class_survey_data.json` (String entry)
- `enrichment/resources/survey/class_survey.md` (no prerequisites)
- No prerequisite Readmes needed

## Class Declaration

**File:** `HISE/hi_scripting/scripting/engine/JavascriptEngineObjects.cpp` lines 771-999

```cpp
struct HiseJavascriptEngine::RootObject::StringClass : public DynamicObject
```

StringClass is a nested struct inside `HiseJavascriptEngine::RootObject`. It inherits from `juce::DynamicObject`, which provides the property/method storage mechanism. It is NOT a `ScriptingObject` or `ApiClass` -- it is a built-in JavaScript engine type, registered as a native object prototype rather than an API class.

**Forward declaration:** `HiseJavascriptEngine.h` line 788: `struct StringClass;`

## Registration Mechanism

**File:** `JavascriptEngineAdditionalMethods.cpp` line 165

```cpp
registerNativeObject(RootObject::StringClass::getClassName(), new RootObject::StringClass());
```

This is called in the `HiseJavascriptEngine` constructor alongside ObjectClass, ArrayClass, JSONClass, and IntegerClass. These are the five built-in engine types.

The `registerNativeObject` method (HiseJavascriptEngine.cpp line 130) stores the object as a property on the root object, making its methods available as a prototype for all string values.

## Method Resolution

**File:** `JavascriptEngineAdditionalMethods.cpp` lines 474-476

```cpp
if (targetObject.isString())
    if (var* m = findRootClassProperty(StringClass::getClassName(), functionName))
        return *m;
```

When a method is called on a string value, the engine checks if the target is a string, then looks up the method name in the StringClass prototype. This is in `Scope::findFunctionCall()`.

The lookup chain is: string methods -> object methods (fallback). Array methods are only checked for array targets. Object methods are always checked as a final fallback.

## Constructor -- Method Registration

All methods are registered via `setMethod()` in the constructor (lines 775-808). There are NO `addConstant()` calls and NO `ADD_TYPED_API_METHOD_N` or `ADD_API_METHOD_N` macros -- this class uses the simpler `DynamicObject::setMethod()` pattern since it is a built-in engine type, not a scripting API class.

### Registered Methods (30 total)

| Method Name | C++ Function | Notes |
|-------------|-------------|-------|
| substring | substring | |
| indexOf | indexOf | |
| charAt | charAt | |
| charCodeAt | charCodeAt | |
| fromCharCode | fromCharCode | Static-like, creates char from int |
| replace | replace | Replaces ALL occurrences (not just first) |
| split | split | |
| splitCamelCase | splitCamelCase | |
| lastIndexOf | lastIndexOf | |
| toLowerCase | toLowerCase | |
| toUpperCase | toUpperCase | |
| capitalize | capitalize | Start case (each word) |
| parseAsJSON | parseAsJSON | Uses juce::JSON::parse |
| trim | trim | |
| concat | concat | |
| encrypt | encrypt | Blowfish encryption |
| decrypt | decrypt | Blowfish decryption |
| contains | contains | |
| startsWith | startsWith | JS-compatible |
| endsWith | endsWith | JS-compatible |
| includes | contains (fn) | Alias -- same C++ function as `contains` |
| slice | substring (fn) | Alias -- same C++ function as `substring` |
| replaceAll | replace (fn) | Alias -- same C++ function as `replace` |
| match | match | Regex via std::regex |
| getTrailingIntValue | getTrailingIntValue | |
| getIntValue | getIntValue | |
| hash | hash | |
| fromFirstOccurrenceOf | fromFirstOccurrenceOf | NOT in base JSON |
| fromLastOccurrenceOf | fromLastOccurrenceOf | NOT in base JSON |
| upToFirstOccurrenceOf | upToFirstOccurrenceOf | NOT in base JSON |
| upToLastOccurrenceOf | upToLastOccurrenceOf | NOT in base JSON |

### Methods in C++ but NOT in base JSON (4)

These four methods are registered in the constructor but missing from the base JSON. They have Doxygen stubs (lines 1084-1094) using `/*` instead of `/**` (single-star comments), which may explain why the doc generator skipped them:

- `fromFirstOccurrenceOf(String subString)` -- Returns section starting after first occurrence
- `fromLastOccurrenceOf(String subString)` -- Returns section starting after last occurrence
- `upToFirstOccurrenceOf(String subString)` -- Returns section up to first occurrence
- `upToLastOccurrenceOf(String subString)` -- Returns section up to last occurrence

All four use `false, false` for the `includeSubString` and `ignoreCase` parameters of the underlying juce::String methods.

### Methods in base JSON but NOT registered as unique C++ functions

- `includes` -- alias for `contains` (same C++ function)
- `slice` -- alias for `substring` (same C++ function)
- `replaceAll` -- alias for `replace` (same C++ function)

These are in the base JSON as separate entries, which is correct since they are distinct API surface methods even though they map to the same implementation.

### Method NOT in base JSON: `fromCharCode`

`fromCharCode` is registered in C++ but not in the base JSON. It converts an integer to a single-character string via `String::charToString(getInt(a, 0))`.

### Method NOT in base JSON: `parseAsJSON`

`parseAsJSON` is registered in C++ but not in the base JSON. It parses the string as JSON using `JSON::parse(a.thisObject.toString())`.

## Method Implementation Details

### Simple Wrappers (one-liners delegating to juce::String)

Most methods are trivial one-line wrappers around juce::String methods:

```cpp
static var contains(Args a)      { return a.thisObject.toString().contains(getString(a, 0)); }
static var startsWith(Args a)    { return a.thisObject.toString().startsWith(getString(a, 0)); }
static var endsWith(Args a)      { return a.thisObject.toString().endsWith(getString(a, 0)); }
static var indexOf(Args a)       { return a.thisObject.toString().indexOf(getString(a, 0)); }
static var lastIndexOf(Args a)   { return a.thisObject.toString().lastIndexOf(getString(a, 0)); }
static var charCodeAt(Args a)    { return (int)a.thisObject.toString()[getInt(a, 0)]; }
static var charAt(Args a)        { int p = getInt(a, 0); return a.thisObject.toString().substring(p, p + 1); }
static var toUpperCase(Args a)   { return a.thisObject.toString().toUpperCase(); }
static var toLowerCase(Args a)   { return a.thisObject.toString().toLowerCase(); }
static var trim(Args a)          { return a.thisObject.toString().trim(); }
static var replace(Args a)       { return a.thisObject.toString().replace(getString(a, 0), getString(a, 1)); }
static var getTrailingIntValue(Args a) { return a.thisObject.toString().getTrailingIntValue(); }
static var getIntValue(Args a)   { return a.thisObject.toString().getLargeIntValue(); }
static var hash(Args a)          { return a.thisObject.toString().hashCode64(); }
static var parseAsJSON(Args a)   { return JSON::parse(a.thisObject.toString()); }
```

### substring (also used by slice)

```cpp
static var substring(Args a) {
    return a.thisObject.toString().substring(getInt(a, 0),
        a.numArguments > 1 ? getInt(a, 1) : 0x7fffffff);
}
```

If only one argument is provided, uses `0x7fffffff` as the end index (effectively "to end of string"). This means `slice` and `substring` both support 1 or 2 arguments.

### split

```cpp
static var split(Args a) {
    const String str(a.thisObject.toString());
    const String sep(getString(a, 0));
    StringArray strings;

    if (sep.isNotEmpty())
        strings.addTokens(str, sep.substring(0, 1), "");
    else
        for (String::CharPointerType pos = str.getCharPointer(); !pos.isEmpty(); ++pos)
            strings.add(String::charToString(*pos));

    var array;
    for (int i = 0; i < strings.size(); ++i)
        array.append(strings[i]);
    return array;
}
```

Key behavior: Only the FIRST CHARACTER of the separator string is used (`sep.substring(0, 1)`). This is because `addTokens` treats each character in the separator as a separate delimiter. Empty separator splits into individual characters (JS-compatible behavior).

### splitCamelCase

Splits at uppercase characters and digit boundaries. Strips whitespace first. "MyValue123Test" would become ["y", "V", "alue", "123", "T", "est"]. Wait -- re-reading more carefully:

The algorithm flushes the current token when it encounters an uppercase char or digit. Uppercase chars are accumulated together (consecutive uppercase forms one token). Digits are accumulated together. Lowercase chars are appended to the current token.

So "MyValue" -> flush at 'M' (uppercase run: "M"), then 'y' is lowercase appended, flush at 'V' (token "y"), uppercase run "V", then "alue" lowercase. Result: ["M", "y", "V", "alue"]. Hmm, that seems odd. Let me re-read...

Actually looking more carefully: the initial state has an empty currentToken. When 'M' is encountered (uppercase), flush (nothing), then accumulate uppercase chars: "M". Next char 'y' is lowercase, so the while loop for uppercase exits. Back to main while loop, 'y' is not digit, not uppercase, so `currentToken << *current++` appends 'y' to "M" making "My". Then 'V' is uppercase: flush "My", accumulate "V". Then 'a','l','u','e' are lowercase, appended to "V" making "Value". End: flush "Value". Result: ["My", "Value"]. That matches the doc description.

### capitalize

Splits by space, uppercases first letter of each word, rejoins with spaces. Standard title/start case.

### match

Uses `std::regex` for regex matching. Catches `std::regex_error` and returns `undefined` on error. Has a safety limit of 100000 iterations to prevent runaway matches. Returns an array of all matches (including capture groups from each match).

### concat

Appends all arguments to the string. Supports variable number of arguments via `a.numArguments`.

### encrypt / decrypt

Uses juce `BlowFish` encryption:
- Key length is clamped to 0-72 bytes via `jlimit(0, 72, key.length())`
- Encrypt: writes string to MemoryOutputStream, encrypts the MemoryBlock, returns base64 encoding
- Decrypt: decodes base64, decrypts MemoryBlock, converts back to string

### fromFirstOccurrenceOf / fromLastOccurrenceOf / upToFirstOccurrenceOf / upToLastOccurrenceOf

All four pass `false, false` to the underlying juce::String methods:
- First `false`: `includeSubStringInResult` -- the delimiter substring is NOT included in the result
- Second `false`: `ignoreCase` -- matching is case-sensitive

## Doxygen Stub Class

**File:** `JavascriptEngineObjects.cpp` lines 1004-1095

```cpp
/** Doxy functions for String operations. */
class DoxygenStringFunctions
```

This is a dummy class used solely for documentation generation. It provides Doxygen-commented method stubs that the API doc generator extracts. Note:
- The four `*OccurrenceOf` methods use single-star comments `/*` instead of `/**`, which may intentionally exclude them from documentation
- `fromCharCode` and `parseAsJSON` are not present in the Doxygen stubs at all

## Survey Data

From `class_survey_data.json`:
- **domain:** scripting
- **role:** utility
- **baseClass:** juce::String (built-in JavaScript engine type)
- **creates:** Array (via split, splitCamelCase, match)
- **createdBy:** [] (built-in type, no factory)
- **seeAlso:** [] (no related classes listed)
- **fanIn:** 0.35 (many classes return/accept strings)
- **fanOut:** 0.04 (minimal outgoing dependencies)
- **callbackDensity:** 0.0 (no callbacks)
- **statefulness:** 0.15 (immutable -- methods return new strings)
- **threadingExposure:** 0.0 (no threading concerns)

## Threading / Lifecycle

No threading constraints. String methods are pure functions that operate on immutable string values. They can be called from any thread, any callback. No onInit-only restrictions.

## Preprocessor Guards

None. The StringClass has no conditional compilation.

## Key Behavioral Notes

1. **replace() replaces ALL occurrences** -- unlike JavaScript's `String.replace()` which only replaces the first match. The comment in the constructor confirms: "alias for replace (HISE replace already replaces all)". This means `replace` and `replaceAll` are truly identical.

2. **split() only uses first character of separator** -- `addTokens(str, sep.substring(0, 1), "")` means multi-character separators are truncated. This differs from JavaScript behavior.

3. **getIntValue() uses getLargeIntValue()** -- returns int64, not int. Can handle large numbers.

4. **hash() uses hashCode64()** -- returns a 64-bit hash, not 32-bit.

5. **Immutable pattern** -- All methods return new values. The original string is never modified (accessed via `a.thisObject.toString()` which creates a copy).

6. **No length property** -- Unlike JavaScript, there is no `.length` property on strings. String length must be obtained differently (not through this class).
