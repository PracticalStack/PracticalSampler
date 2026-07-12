Date::getSystemTimeISO8601(Integer includeDividerCharacters) -> String

Thread safety: WARNING -- String return value involves atomic ref-count operations.
Returns the current system time as an ISO-8601 formatted string using the local
timezone. With dividers: "2026-03-09T14:30:00+0100". Without: "20260309T143000+0100".

Pair with:
  getSystemTimeMs -- for a numeric timestamp instead of a string
  ISO8601ToMilliseconds -- to parse the returned string back to milliseconds

Source:
  ScriptingApi.cpp:~3750  Date::getSystemTimeISO8601()
    -> Time::getCurrentTime().toISO8601(includeDividerCharacters)
