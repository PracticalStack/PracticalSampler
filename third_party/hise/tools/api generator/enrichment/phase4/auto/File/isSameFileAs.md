Compares path strings to determine whether this file and the given File object reference the same location. The comparison is case-sensitive on Linux and case-insensitive on Windows and macOS.

> [!Warning:Does not resolve symlinks] Does not resolve symlinks or HISE link file redirects. Two File objects pointing to the same physical file via different paths (e.g. one through a symlink) compare as different.
