# String -- Method Analysis

## capitalize

**Signature:** `String capitalize()`
**Return Type:** `String`
**Call Scope:** unsafe
**Call Scope Note:** Allocates new string content.
**Minimal Example:** `var result = {obj}.capitalize();`

**Description:**
Converts the first letter of each word to uppercase (title case). Splits the string on spaces, uppercases the first character of each token, and rejoins with spaces.

**Parameters:**

(No parameters.)

**Cross References:**
- `$API.String.toUpperCase$`

---

## charAt

**Signature:** `String charAt(int position)`
**Return Type:** `String`
**Call Scope:** unsafe
**Call Scope Note:** Allocates new string content via substring extraction.
**Minimal Example:** `var ch = {obj}.charAt(0);`

**Description:**
Returns the character at the specified position as a single-character string. Returns an empty string if the position is out of bounds.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| position | Integer | no | Zero-based character index | -- |

**Cross References:**
- `$API.String.charCodeAt$`

---

## charCodeAt

**Signature:** `int charCodeAt(int position)`
**Return Type:** `Integer`
**Call Scope:** warning
**Call Scope Note:** String involvement, atomic ref-count operations.
**Minimal Example:** `var code = {obj}.charCodeAt(0);`

**Description:**
Returns the Unicode code point (integer) of the character at the specified position. Returns 0 if the position is out of bounds.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| position | Integer | no | Zero-based character index | -- |

**Cross References:**
- `$API.String.charAt$`

---

## concat

**Signature:** `String concat(String arg1, ...)`
**Return Type:** `String`
**Call Scope:** unsafe
**Call Scope Note:** Allocates new string content.
**Minimal Example:** `var result = {obj}.concat(" world", "!");`

**Description:**
Appends one or more string arguments to the string and returns the combined result. Accepts a variable number of arguments -- all are converted to strings and concatenated in order.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| arg1 | String | no | First value to append | -- |
| ... | String | no | Additional values to append (variadic) | -- |

---

## contains

**Signature:** `bool contains(String searchString)`
**Return Type:** `Integer`
**Call Scope:** warning
**Call Scope Note:** String involvement, atomic ref-count operations.
**Minimal Example:** `var found = {obj}.contains("hello");`

**Description:**
Returns true if the string contains the specified substring. Case-sensitive.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| searchString | String | no | Substring to search for | -- |

**Cross References:**
- `$API.String.includes$`

---

## decrypt

**Signature:** `String decrypt(String key)`
**Return Type:** `String`
**Call Scope:** unsafe
**Call Scope Note:** BlowFish decryption, base64 decoding, and string construction.
**Minimal Example:** `var plain = {obj}.decrypt("myKey");`

**Description:**
Decrypts the string using BlowFish with the provided key. Expects the string to be in base64-encoded encrypted format as produced by `encrypt`. Returns the decrypted plaintext string.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| key | String | no | BlowFish decryption key (must match the key used for encryption) | Max 72 bytes (silently clamped) |

**Pitfalls:**
- Key length is silently clamped to 72 bytes. Keys longer than 72 characters are truncated without warning, so encrypt and decrypt still work but only use the first 72 bytes.

**Cross References:**
- `$API.String.encrypt$`

---

## encrypt

**Signature:** `String encrypt(String key)`
**Return Type:** `String`
**Call Scope:** unsafe
**Call Scope Note:** BlowFish encryption, memory allocation, and base64 encoding.
**Minimal Example:** `var encrypted = {obj}.encrypt("myKey");`

**Description:**
Encrypts the string using BlowFish with the provided key and returns a base64-encoded result. Use `decrypt` with the same key to recover the original string.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| key | String | no | BlowFish encryption key | Max 72 bytes (silently clamped) |

**Pitfalls:**
- Key length is silently clamped to 72 bytes. Keys longer than 72 characters are truncated without warning.

**Cross References:**
- `$API.String.decrypt$`

**Example:**
```javascript:encrypt-decrypt-roundtrip
// Title: Encrypt and decrypt a string with BlowFish
var secret = "Hello World";
var key = "mySecretKey";
var encrypted = secret.encrypt(key);
var decrypted = encrypted.decrypt(key);
Console.print(decrypted); // Hello World
```
```json:testMetadata:encrypt-decrypt-roundtrip
{
  "testable": true,
  "verifyScript": [
    {"type": "log-output", "values": ["Hello World"]}
  ]
}
```

---

## endsWith

**Signature:** `bool endsWith(String suffix)`
**Return Type:** `Integer`
**Call Scope:** warning
**Call Scope Note:** String involvement, atomic ref-count operations.
**Minimal Example:** `var result = {obj}.endsWith(".wav");`

**Description:**
Returns true if the string ends with the specified suffix. Case-sensitive.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| suffix | String | no | Suffix to test for | -- |

**Cross References:**
- `$API.String.startsWith$`

---

## getIntValue

**Signature:** `int getIntValue()`
**Return Type:** `Integer`
**Call Scope:** warning
**Call Scope Note:** String involvement, atomic ref-count operations.
**Minimal Example:** `var num = {obj}.getIntValue();`

**Description:**
Parses the string as a 64-bit integer from the beginning. Returns 0 if the string does not start with a numeric character. Uses juce::String::getLargeIntValue() internally, so it can handle values beyond the 32-bit integer range.

**Parameters:**

(No parameters.)

**Cross References:**
- `$API.String.getTrailingIntValue$`

---

## getTrailingIntValue

**Signature:** `int getTrailingIntValue()`
**Return Type:** `Integer`
**Call Scope:** warning
**Call Scope Note:** String involvement, atomic ref-count operations.
**Minimal Example:** `var num = {obj}.getTrailingIntValue();`

**Description:**
Extracts and returns the integer at the end of the string. Returns 0 if the string does not end with digits. Useful for extracting numeric suffixes from component names (e.g., "Knob12" returns 12).

**Parameters:**

(No parameters.)

**Cross References:**
- `$API.String.getIntValue$`

---

## hash

**Signature:** `int hash()`
**Return Type:** `Integer`
**Call Scope:** warning
**Call Scope Note:** String involvement, atomic ref-count operations.
**Minimal Example:** `var h = {obj}.hash();`

**Description:**
Returns a 64-bit hash code for the string. Uses juce::String::hashCode64() internally. Two identical strings always produce the same hash value.

**Parameters:**

(No parameters.)

---

## includes

**Signature:** `bool includes(String searchString)`
**Return Type:** `Integer`
**Call Scope:** warning
**Call Scope Note:** String involvement, atomic ref-count operations.
**Minimal Example:** `var found = {obj}.includes("hello");`

**Description:**
Returns true if the string contains the specified substring. Alias for `contains` -- both call the same C++ implementation. Provided for JavaScript API familiarity.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| searchString | String | no | Substring to search for | -- |

**Cross References:**
- `$API.String.contains$`

---

## indexOf

**Signature:** `int indexOf(String searchString)`
**Return Type:** `Integer`
**Call Scope:** warning
**Call Scope Note:** String involvement, atomic ref-count operations. O(n) search.
**Minimal Example:** `var pos = {obj}.indexOf("needle");`

**Description:**
Returns the zero-based index of the first occurrence of the search string, or -1 if not found. Case-sensitive.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| searchString | String | no | Substring to search for | -- |

**Cross References:**
- `$API.String.lastIndexOf$`

---

## lastIndexOf

**Signature:** `int lastIndexOf(String searchString)`
**Return Type:** `Integer`
**Call Scope:** warning
**Call Scope Note:** String involvement, atomic ref-count operations. O(n) search.
**Minimal Example:** `var pos = {obj}.lastIndexOf("/");`

**Description:**
Returns the zero-based index of the last occurrence of the search string, or -1 if not found. Case-sensitive.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| searchString | String | no | Substring to search for | -- |

**Cross References:**
- `$API.String.indexOf$`

---

## match

**Signature:** `var match(String regex)`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** std::regex compilation, array allocation, and string construction.
**Minimal Example:** `var matches = {obj}.match("[0-9]+");`

**Description:**
Matches the string against a regular expression (std::regex syntax) and returns an array of all matches, including capture groups from each match. Returns undefined if the regex pattern is invalid. Has an internal safety limit of 100000 iterations to prevent runaway matches.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| regex | String | no | Regular expression pattern (std::regex syntax) | -- |

**Pitfalls:**
- An invalid regex pattern silently returns undefined instead of throwing an error. Check `isDefined(result)` after calling match to distinguish "no matches" (empty array) from "invalid regex" (undefined).

**Example:**
```javascript:regex-matching
// Title: Extract numeric values with regex
var text = "Width: 100px, Height: 200px";
var numbers = text.match("[0-9]+");

Console.print(numbers.length); // 2
Console.print(numbers[0]);     // 100
Console.print(numbers[1]);     // 200
```
```json:testMetadata:regex-matching
{
  "testable": true,
  "verifyScript": [
    {"type": "log-output", "values": ["2", "100", "200"]}
  ]
}
```

---

## replace

**Signature:** `String replace(String search, String replacement)`
**Return Type:** `String`
**Call Scope:** unsafe
**Call Scope Note:** Allocates new string content.
**Minimal Example:** `var result = {obj}.replace("old", "new");`

**Description:**
Replaces all occurrences of the search string with the replacement string and returns the result. Unlike JavaScript's `String.replace()` which only replaces the first match, HISEScript's `replace` replaces ALL occurrences.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| search | String | no | Substring to find | -- |
| replacement | String | no | Replacement string | -- |

**Pitfalls:**
- Replaces ALL occurrences, not just the first. This differs from standard JavaScript behavior where `replace` only replaces the first match.

**Cross References:**
- `$API.String.replaceAll$`

---

## replaceAll

**Signature:** `String replaceAll(String search, String replacement)`
**Return Type:** `String`
**Call Scope:** unsafe
**Call Scope Note:** Allocates new string content.
**Minimal Example:** `var result = {obj}.replaceAll("old", "new");`

**Description:**
Replaces all occurrences of the search string with the replacement string. Alias for `replace` -- both call the same C++ implementation. Since HISEScript's `replace` already replaces all occurrences, `replaceAll` exists purely for JavaScript API familiarity.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| search | String | no | Substring to find | -- |
| replacement | String | no | Replacement string | -- |

**Cross References:**
- `$API.String.replace$`

---

## slice

**Signature:** `String slice(int start, int end)`
**Return Type:** `String`
**Call Scope:** unsafe
**Call Scope Note:** Allocates new string content via substring extraction.
**Minimal Example:** `var part = {obj}.slice(0, 5);`

**Description:**
Returns a section of the string from start index to end index (exclusive). If end is omitted, returns from start to the end of the string. Alias for `substring` -- both call the same C++ implementation.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| start | Integer | no | Start index (inclusive) | -- |
| end | Integer | no | End index (exclusive). Omit for end of string. | Optional |

**Cross References:**
- `$API.String.substring$`

---

## split

**Signature:** `Array split(String separator)`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** Array and string allocation.
**Minimal Example:** `var parts = {obj}.split(",");`

**Description:**
Splits the string by a separator and returns an array of substrings. If the separator is empty, splits into individual characters (JavaScript-compatible behavior).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| separator | String | no | Delimiter character | Only the first character is used |

**Pitfalls:**
- Only the first character of the separator string is used as the delimiter. Multi-character separators like "::" or ", " are silently truncated to their first character (":" or ","). This differs from JavaScript's split which supports full string separators.

**Cross References:**
- `$API.String.splitCamelCase$`

**Example:**
```javascript:split-behavior
// Title: Split uses only the first character of the separator
var csv = "one,two,three";
var parts = csv.split(",");
Console.print(parts[0]); // one
Console.print(parts[1]); // two
Console.print(parts[2]); // three

// Empty separator splits into individual characters
var chars = "abc".split("");
Console.print(chars.length); // 3
```
```json:testMetadata:split-behavior
{
  "testable": true,
  "verifyScript": [
    {"type": "log-output", "values": ["one", "two", "three", "3"]}
  ]
}
```

---

## splitCamelCase

**Signature:** `Array splitCamelCase()`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** Array and string allocation.
**Minimal Example:** `var parts = {obj}.splitCamelCase();`

**Description:**
Splits a camelCase or PascalCase string into an array of word tokens at uppercase letter and digit boundaries. Consecutive uppercase letters form a single token. Consecutive digits form a single token. Whitespace is stripped before splitting.

**Parameters:**

(No parameters.)

**Cross References:**
- `$API.String.split$`

**Example:**
```javascript:split-camel-case
// Title: Split camelCase identifiers into word tokens
var name = "myValueTest";
var parts = name.splitCamelCase();

Console.print(parts[0]); // my
Console.print(parts[1]); // Value
Console.print(parts[2]); // Test
```
```json:testMetadata:split-camel-case
{
  "testable": true,
  "verifyScript": [
    {"type": "log-output", "values": ["my", "Value", "Test"]}
  ]
}
```

---

## startsWith

**Signature:** `bool startsWith(String prefix)`
**Return Type:** `Integer`
**Call Scope:** warning
**Call Scope Note:** String involvement, atomic ref-count operations.
**Minimal Example:** `var result = {obj}.startsWith("http");`

**Description:**
Returns true if the string starts with the specified prefix. Case-sensitive.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| prefix | String | no | Prefix to test for | -- |

**Cross References:**
- `$API.String.endsWith$`

---

## substring

**Signature:** `String substring(int start, int end)`
**Return Type:** `String`
**Call Scope:** unsafe
**Call Scope Note:** Allocates new string content.
**Minimal Example:** `var part = {obj}.substring(0, 5);`

**Description:**
Returns a section of the string from start index to end index (exclusive). If end is omitted, returns from start to the end of the string.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| start | Integer | no | Start index (inclusive) | -- |
| end | Integer | no | End index (exclusive). Omit for end of string. | Optional |

**Cross References:**
- `$API.String.slice$`

---

## toLowerCase

**Signature:** `String toLowerCase()`
**Return Type:** `String`
**Call Scope:** unsafe
**Call Scope Note:** Allocates new string content.
**Minimal Example:** `var lower = {obj}.toLowerCase();`

**Description:**
Returns the string with all characters converted to lowercase.

**Parameters:**

(No parameters.)

**Cross References:**
- `$API.String.toUpperCase$`

---

## toUpperCase

**Signature:** `String toUpperCase()`
**Return Type:** `String`
**Call Scope:** unsafe
**Call Scope Note:** Allocates new string content.
**Minimal Example:** `var upper = {obj}.toUpperCase();`

**Description:**
Returns the string with all characters converted to uppercase.

**Parameters:**

(No parameters.)

**Cross References:**
- `$API.String.toLowerCase$`

---

## trim

**Signature:** `String trim()`
**Return Type:** `String`
**Call Scope:** unsafe
**Call Scope Note:** Allocates new string content.
**Minimal Example:** `var trimmed = {obj}.trim();`

**Description:**
Returns the string with leading and trailing whitespace removed.

**Parameters:**

(No parameters.)
