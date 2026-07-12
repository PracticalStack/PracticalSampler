Server::cleanFinishedDownloads() -> undefined

Thread safety: SAFE -- sets an atomic boolean flag only. Actual removal happens asynchronously on the Server Thread.
Signals the Server Thread to remove all finished downloads from the pending downloads list. Removal does not happen immediately -- sets an atomic flag checked on the next Server Thread iteration (within 500ms). In-progress downloads are not affected.
Pair with:
  getPendingDownloads -- finished downloads remain in list until this is called
  downloadFile -- initiates downloads that eventually need cleanup
Source:
  ScriptingApi.cpp  Server::cleanFinishedDownloads()
    -> sets WebThread::cleanDownloads atomic flag to true
    -> WebThread::run() checks flag, removes finished ScriptDownloadObjects from pendingDownloads
