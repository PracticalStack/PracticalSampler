Returns an array of pool reference strings for all audio files in this expansion. Each string can be passed directly to pool-based APIs such as `AudioSampleProcessor.setFile()`.

> [!Warning:First call triggers filesystem scan] Unlike `getSampleMapList` and `getMidiFileList`, this method scans the expansion folder for audio files before returning results. The first call may be noticeably slower than subsequent calls.
