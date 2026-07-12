Engine::matchesRegex(String stringToMatch, String regex) -> Integer

Thread safety: UNSAFE -- std::regex construction (heap allocation, expensive)
Tests if string contains a regex match (partial match, not full-string).
Invalid regex patterns emit debug error and return false.
Anti-patterns:
  - Uses std::regex_search, not std::regex_match -- partial match suffices.
    Anchor with ^ and $ for full-string matching.
Pair with:
  getRegexMatches -- extract actual matches
Source:
  ScriptingApi.cpp  Engine::matchesRegex()
    -> std::regex_search(str, regex)
