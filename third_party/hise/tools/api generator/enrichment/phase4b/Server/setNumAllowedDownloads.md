Server::setNumAllowedDownloads(Integer maxNumberOfParallelDownloads) -> undefined

Thread safety: SAFE -- sets a plain integer field on the WebThread. No allocations, no locks.
Sets the maximum number of downloads that can run in parallel. Default is 1 (sequential). The WebThread manages the queue and starts waiting downloads up to this limit. When a download completes, the next waiting download starts automatically. If the limit is reduced below the number of active downloads, excess downloads are paused until slots free up. Persists across script recompilations.
Pair with:
  downloadFile -- initiates downloads managed by this limit
  getPendingDownloads -- inspect the download queue
Source:
  ScriptingApi.cpp  Server::setNumAllowedDownloads()
    -> sets WebThread::numMaxDownloads integer field
    -> WebThread::run() starts/stops downloads based on this limit each iteration
