Date::ISO8601ToMilliseconds(String iso8601) -> Integer

Thread safety: WARNING -- String parameter involvement, atomic ref-count operations.
Parses an ISO-8601 formatted date string and returns the corresponding time as
milliseconds since the Unix epoch. Accepts strings with or without divider characters.

Anti-patterns:
  - Do NOT rely on a return value of 0 to detect parse failure -- invalid strings
    silently return 0 (the Unix epoch), indistinguishable from a genuine epoch timestamp.

Pair with:
  millisecondsToISO8601 -- inverse operation (milliseconds to ISO-8601 string)
  getSystemTimeISO8601 -- to get a parseable string from the current time

Source:
  ScriptingApi.cpp:~3767  Date::ISO8601ToMilliseconds()
    -> juce::Time::fromISO8601(iso8601).toMilliseconds()
