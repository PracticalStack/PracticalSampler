String::match(String regex) -> Array

Thread safety: UNSAFE -- std::regex compilation, array allocation, string construction.
Matches the string against a regular expression (std::regex syntax) and returns
an array of all matches including capture groups. Returns undefined if the regex
is invalid. Internal safety limit of 100000 iterations.

Anti-patterns:
  - Invalid regex silently returns undefined instead of throwing. Check
    isDefined(result) to distinguish "no matches" (empty array) from
    "invalid regex" (undefined).

Source:
  JavascriptEngineObjects.cpp:771-999  StringClass::match()
    -> std::regex(pattern) -> std::sregex_iterator
    -> catches std::regex_error, returns undefined on error
