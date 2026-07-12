File::toString(Integer formatType) -> String

Thread safety: SAFE -- no I/O, extracts path components from in-memory path string.
Returns a string representation according to the format constant.

Format constants (available as properties on any File object):
  FullPath (0)     -- full absolute path (e.g., C:/Users/data/file.txt)
  NoExtension (1)  -- filename without extension (e.g., file)
  Extension (2)    -- extension only (e.g., .txt)
  Filename (3)     -- filename with extension (e.g., file.txt)

Dispatch/mechanics:
  switch (formatType):
    FullPath -> f.getFullPathName()
    NoExtension -> f.getFileNameWithoutExtension()
    OnlyExtension -> f.getFileExtension()
    Filename -> f.getFileName()
  Reports "Illegal formatType argument N" for invalid values.

Pair with:
  toReferenceString -- for pool-relative path strings
  getRelativePathFrom -- for relative paths between files

Source:
  ScriptingApiObjects.cpp  ScriptFile::toString()
    -> switch on Format enum -> juce::File getter methods
