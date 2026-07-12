Returns an array of all regex matches found in the input string, including capture groups. Each match produces N+1 entries in the result array (full match plus each capture group).

> [!Warning:Invalid regex returns undefined] An invalid regex pattern returns `undefined` instead of an empty array. Check the return value with `isDefined()` if user-provided patterns are possible.