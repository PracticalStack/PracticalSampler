Date::getSystemTimeMs() -> Integer

Thread safety: SAFE
Returns the current system time as milliseconds since the Unix epoch
(January 1, 1970 00:00:00 UTC). This is wall-clock time, not DAW transport position.

Pair with:
  millisecondsToISO8601 -- convert the returned value to a human-readable date string
  getSystemTimeISO8601 -- for a string representation directly

Source:
  ScriptingApi.cpp:~3755  Date::getSystemTimeMs()
    -> Time::getCurrentTime().toMilliseconds()
