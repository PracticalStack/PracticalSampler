Download::getFullURL() -> String

Thread safety: WARNING -- string involvement, atomic ref-count operations.
Returns the full URL string of this download as passed to Server.downloadFile().
The URL is returned without POST data. Immutable for the lifetime of the Download object.

Source:
  ScriptingApiObjects.cpp:~1330  ScriptDownloadObject::getFullURL()
    -> downloadURL.toString(false)
