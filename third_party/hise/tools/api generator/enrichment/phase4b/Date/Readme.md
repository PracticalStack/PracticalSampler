Date (namespace)

System time access and ISO-8601 date string conversion utilities. Provides
wall-clock time as milliseconds (Unix epoch) and bidirectional conversion
between millisecond timestamps and ISO-8601 formatted strings using the
local timezone. Factored out of Engine to reduce that class's method count.

Common mistakes:
  - Passing an invalid string to ISO8601ToMilliseconds -- silently returns 0
    (Unix epoch) with no way to distinguish from a genuine epoch timestamp.

Example:
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

Methods (4):
  getSystemTimeISO8601    getSystemTimeMs
  ISO8601ToMilliseconds   millisecondsToISO8601
