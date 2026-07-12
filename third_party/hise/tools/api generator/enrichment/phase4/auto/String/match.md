Matches the string against a regular expression and returns an array of all matches, including capture groups. Uses std::regex syntax. Returns an empty array when there are no matches.

> [!Warning:Invalid regex returns undefined, not an error] An invalid regex pattern silently returns `undefined` instead of throwing an error. Check `isDefined(result)` after calling `match` to distinguish "no matches" (empty array) from "invalid regex" (undefined).
