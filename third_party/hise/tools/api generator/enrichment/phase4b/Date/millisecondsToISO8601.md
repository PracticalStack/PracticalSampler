Date::millisecondsToISO8601(Integer miliseconds, Integer includeDividerCharacters) -> String

Thread safety: WARNING -- String return value involves atomic ref-count operations.
Converts a millisecond timestamp (Unix epoch) to an ISO-8601 formatted date string
using the local timezone. With dividers: dashes, colons, and T separator. Without: compact form.

Pair with:
  ISO8601ToMilliseconds -- inverse operation (ISO-8601 string to milliseconds)
  getSystemTimeMs -- to get the current time as the input millisecond value

Source:
  ScriptingApi.cpp:~3761  Date::millisecondsToISO8601()
    -> Time(miliseconds).toISO8601(includeDividerCharacters)
