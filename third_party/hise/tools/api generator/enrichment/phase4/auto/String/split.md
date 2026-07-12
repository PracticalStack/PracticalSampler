Splits the string by a separator character and returns an array of substrings. Pass an empty string to split into individual characters. The most common use is parsing hierarchical paths like sample map IDs with `split("/")`.

> [!Warning:Only the first character of the separator is used] Multi-character separators are silently truncated. `"a::b".split("::")` splits on `":"`, producing `["a", "", "b"]` with an empty token. Always use a single-character delimiter.
