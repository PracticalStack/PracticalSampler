Server::getPendingDownloads() -> Array

Thread safety: UNSAFE -- iterates the pending downloads list and constructs a new Array with heap allocation.
Returns a snapshot array of all Download objects currently tracked by the server, including waiting, in-progress, paused, and finished downloads. Finished downloads remain in the list until cleanFinishedDownloads() is called. The array is not live-updated as download states change.
Pair with:
  downloadFile -- initiates downloads tracked in this list
  cleanFinishedDownloads -- removes finished downloads from the list
  getPendingCalls -- equivalent for GET/POST request queue
Source:
  ScriptingApi.cpp  Server::getPendingDownloads()
    -> iterates WebThread::pendingDownloads
    -> constructs Array of ScriptDownloadObject references
