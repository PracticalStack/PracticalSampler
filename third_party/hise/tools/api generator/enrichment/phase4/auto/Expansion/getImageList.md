Returns an array of pool reference strings for all image files in this expansion. Each string can be used with `ScriptImage.set("fileName", ref)` or other image-loading APIs.

> [!Warning:First call triggers filesystem scan] Unlike `getSampleMapList` and `getMidiFileList`, this method scans the expansion folder for image files before returning results. The first call may be noticeably slower than subsequent calls.
