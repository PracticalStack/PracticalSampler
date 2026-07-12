# Date -- Class Analysis

## Brief
System time access and ISO-8601 date string conversion utilities.

## Purpose
Date is a namespace-style utility class providing wall-clock time access and bidirectional conversion between millisecond timestamps (Unix epoch) and ISO-8601 formatted date strings. It was factored out of the Engine class to reduce that class's method count. All methods delegate to JUCE's `juce::Time` class and use the system's local timezone for string representations.

## obtainedVia
Global namespace -- accessed directly as `Date.methodName()`.

## minimalObjectToken


## Constants
None.

## Dynamic Constants
None.

## Common Mistakes
None.

## codeExample
```javascript:date-round-trip-conversion
// Title: Millisecond round-trip through ISO-8601 is lossless
var ms = Date.getSystemTimeMs();
var dateStr = Date.millisecondsToISO8601(ms, true);
var backToMs = Date.ISO8601ToMilliseconds(dateStr);

// Round-trip preserves the exact millisecond value
Console.print(backToMs == ms); // true

// The divider parameter only affects formatting, not data
var withDividers = Date.getSystemTimeISO8601(true);
var withoutDividers = Date.getSystemTimeISO8601(false);
Console.print(withDividers);    // e.g. 2026-03-09T14:30:00+0100
Console.print(withoutDividers); // e.g. 20260309T143000+0100
```
```json:testMetadata:date-round-trip-conversion
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "ms > 0", "value": true},
    {"type": "REPL", "expression": "backToMs == ms", "value": true},
    {"type": "REPL", "expression": "withDividers.indexOf(\"T\") != -1", "value": true},
    {"type": "REPL", "expression": "withoutDividers.indexOf(\"-\") == -1", "value": true}
  ]
}
```

## Alternatives
None.

## Related Preprocessors
None.

## Diagnostic Ideas
Reviewed: Yes
Count: 0
Rationale: All four methods are stateless, standalone calls with no inter-method timeline dependencies or state preconditions. The ISO8601ToMilliseconds silent-return-0 issue is a runtime validation gap (tracked as a bug), not a parse-time checkable pattern.
