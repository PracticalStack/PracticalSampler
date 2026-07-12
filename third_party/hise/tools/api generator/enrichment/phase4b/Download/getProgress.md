Download::getProgress() -> Double

Thread safety: UNSAFE -- reads from a DynamicObject via getProperty() which involves string-based property lookup.
Returns the download progress as a ratio from 0.0 to 1.0. Calculated as
numDownloaded / numTotal from the data object. Returns 0.0 when total size is unknown.

Dispatch/mechanics:
  Reads data["numDownloaded"] and data["numTotal"] via getProperty()
  Adds existingBytesBeforeResuming to both numerator and denominator
  Returns 0.0 if denominator is 0

Anti-patterns:
  - [BUG] During resumed downloads, progress values are double-counted. data.numDownloaded
    and data.numTotal already include existingBytesBeforeResuming (added in progress()
    callback), but getProgress() adds it again. The ratio may still appear approximately
    correct since both sides are inflated equally. For accurate progress during resumes,
    use data.numDownloaded / data.numTotal directly.

Source:
  ScriptingApiObjects.cpp:~1300  ScriptDownloadObject::getProgress()
    -> reads data->getProperty("numDownloaded") + existingBytesBeforeResuming
    -> reads data->getProperty("numTotal") + existingBytesBeforeResuming
    -> returns numerator / denominator (or 0.0 if denominator == 0)
