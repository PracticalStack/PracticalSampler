Engine::getBufferSize() -> Integer

Thread safety: SAFE -- returns cached int (largestBlockSize)
Returns the current maximum processing block size in samples. May differ from the
host buffer size if setMaximumBlockSize() has been called.
Source:
  ScriptingApi.h  Engine::getBufferSize() inline
    -> returns Processor::largestBlockSize
