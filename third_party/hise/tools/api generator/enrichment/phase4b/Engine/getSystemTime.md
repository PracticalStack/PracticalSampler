Engine::getSystemTime(bool includeDividerCharacters) -> String

Thread safety: WARNING -- string construction
Returns current system date/time as ISO-8601 string. With dividers: "2025-03-19T14:30:00+01:00".
Without: "20250319T143000+0100".
Source:
  ScriptingApi.cpp  Engine::getSystemTime()
    -> Time::getCurrentTime().toISO8601(includeDividerCharacters)
