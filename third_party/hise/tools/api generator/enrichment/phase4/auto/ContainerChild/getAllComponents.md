Recursively searches all descendants and returns an array of ContainerChild references whose `id` matches the given pattern. Returns an empty array if no matches are found.

> [!Warning:Uses wildcard matching, not regex] Despite the parameter name `regex`, this method uses `*` and `?` glob syntax, not regular expressions.
