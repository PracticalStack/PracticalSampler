Engine::getRegexMatches(String stringToMatch, String regex) -> Array

Thread safety: UNSAFE -- std::regex construction and iteration (heap allocations)
Returns array of all regex matches including capture groups. Safety limit of 100k iterations.
Anti-patterns:
  - Invalid regex returns undefined, not empty array -- check isDefined()
  - Each match appends ALL sub-matches (full + capture groups), not just full match
Source:
  ScriptingApi.cpp  Engine::getRegexMatches()
    -> std::regex_search in loop -> collects sub_matches
