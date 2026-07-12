File::isChildOf(ScriptObject otherFile, Integer checkSubdirectories) -> Integer

Thread safety: SAFE -- no I/O, compares in-memory path strings only.
Checks whether this file is a child of the given directory. When
checkSubdirectories is true, checks the entire ancestor chain (any depth).
When false, only checks the immediate parent.

Dispatch/mechanics:
  if checkSubdirectories: f.isAChildOf(sf->f)
  else: f.getParentDirectory() == sf->f

Anti-patterns:
  - Passing a non-File object as otherFile silently returns false instead of
    reporting an error. This can mask bugs where a string is passed instead of
    a File object.

Source:
  ScriptingApiObjects.cpp  ScriptFile::isChildOf()
    -> dynamic_cast<ScriptFile*>(otherFile.getObject())
    -> checkSubdirectories ? f.isAChildOf(sf->f) : f.getParentDirectory() == sf->f
