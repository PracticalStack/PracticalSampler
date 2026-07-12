Requests resumption of a stopped download. Returns `true` if the resume request was accepted, `false` if the download is currently running, already finished, or was aborted. When resuming, the existing partial file is detected and only the remaining bytes are downloaded using HTTP Range headers.

[See: Download State Machine](#diagram-download-lifecycle)

> [!Warning:Returns false silently on failure] Returns `false` silently when the download cannot be resumed. Check `getStatusText()` to determine why - a finished or aborted download cannot be resumed.