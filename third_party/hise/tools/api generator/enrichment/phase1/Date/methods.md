# Date -- Method Analysis

## getSystemTimeISO8601

**Signature:** `String getSystemTimeISO8601(Integer includeDividerCharacters)`
**Return Type:** `String`
**Call Scope:** warning
**Call Scope Note:** String return value involves atomic ref-count operations.
**Minimal Example:** `var now = Date.getSystemTimeISO8601(true);`

**Description:**
Returns the current system time as an ISO-8601 formatted string using the local timezone. When `includeDividerCharacters` is true, the output uses dashes, colons, and the `T` separator (e.g., `2026-03-09T14:30:00+0100`). When false, dividers are omitted (e.g., `20260309T143000+0100`). Delegates to `juce::Time::getCurrentTime().toISO8601()`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| includeDividerCharacters | Integer | no | When true, includes dashes, colons, and T separator in the output. When false, omits divider characters. | Boolean (0 or 1) |

**Cross References:**
- `$API.Date.getSystemTimeMs$`
- `$API.Date.millisecondsToISO8601$`
- `$API.Date.ISO8601ToMilliseconds$`

## getSystemTimeMs

**Signature:** `Integer getSystemTimeMs()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var ms = Date.getSystemTimeMs();`

**Description:**
Returns the current system time as milliseconds since the Unix epoch (January 1, 1970 00:00:00 UTC). The returned value is a 64-bit integer. This is the wall-clock time from the operating system, not the DAW transport position. Delegates to `juce::Time::getCurrentTime().toMilliseconds()`.

**Parameters:**
None.

**Cross References:**
- `$API.Date.getSystemTimeISO8601$`
- `$API.Date.ISO8601ToMilliseconds$`
- `$API.Date.millisecondsToISO8601$`

## ISO8601ToMilliseconds

**Signature:** `Integer ISO8601ToMilliseconds(String iso8601)`
**Return Type:** `Integer`
**Call Scope:** warning
**Call Scope Note:** String parameter involvement, atomic ref-count operations.
**Minimal Example:** `var ms = Date.ISO8601ToMilliseconds("2026-03-09T14:30:00");`

**Description:**
Parses an ISO-8601 formatted date string and returns the corresponding time as milliseconds since the Unix epoch. The string can include or omit divider characters (dashes, colons, T separator). Delegates to `juce::Time::fromISO8601().toMilliseconds()`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| iso8601 | String | no | An ISO-8601 formatted date string to parse. | Valid ISO-8601 format |

**Pitfalls:**
- [BUG] If an invalid or unparseable string is passed, the method silently returns 0 (the Unix epoch, January 1, 1970 00:00:00 UTC) instead of throwing an error. There is no way to distinguish between a genuinely parsed epoch timestamp and a parse failure.

**Cross References:**
- `$API.Date.millisecondsToISO8601$`
- `$API.Date.getSystemTimeISO8601$`
- `$API.Date.getSystemTimeMs$`

## millisecondsToISO8601

**Signature:** `String millisecondsToISO8601(Integer miliseconds, Integer includeDividerCharacters)`
**Return Type:** `String`
**Call Scope:** warning
**Call Scope Note:** String return value involves atomic ref-count operations.
**Minimal Example:** `var dateStr = Date.millisecondsToISO8601(Date.getSystemTimeMs(), true);`

**Description:**
Converts a millisecond timestamp (since the Unix epoch) to an ISO-8601 formatted date string using the local timezone. When `includeDividerCharacters` is true, the output uses dashes, colons, and the `T` separator. When false, dividers are omitted. Delegates to `juce::Time(miliseconds).toISO8601()`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| miliseconds | Integer | no | Milliseconds since the Unix epoch (January 1, 1970 00:00:00 UTC). | 64-bit integer |
| includeDividerCharacters | Integer | no | When true, includes dashes, colons, and T separator in the output. When false, omits divider characters. | Boolean (0 or 1) |

**Cross References:**
- `$API.Date.ISO8601ToMilliseconds$`
- `$API.Date.getSystemTimeISO8601$`
- `$API.Date.getSystemTimeMs$`
