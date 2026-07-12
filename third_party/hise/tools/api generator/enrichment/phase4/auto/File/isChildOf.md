Checks whether this file is a child of the given directory. When `checkSubdirectories` is `true`, checks the entire ancestor chain at any depth. When `false`, only checks the immediate parent.

> [!Warning:Non-File argument returns false silently] Passing a non-File object as `otherFile` silently returns `false` instead of reporting an error, which can mask bugs where a string path is passed by mistake.
