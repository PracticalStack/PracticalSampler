Server::downloadFile(String subURL, JSON parameters, ScriptObject targetFile, Function callback) -> ScriptObject

Thread safety: UNSAFE -- allocates a ScriptDownloadObject, acquires a lock on the download queue, and modifies reference-counted arrays.
Initiates an async file download from subURL appended to the base URL. Returns a Download object for progress monitoring, pausing, resuming, or aborting. Downloads are deduplicated by URL -- calling twice with the same URL reuses the existing download and updates its callback. If the target file already exists with content, download resumes via HTTP Range headers. If subURL contains query parameters and the parameters object is empty, the query string is automatically parsed into the parameters object.
Callback signature: f() -- callback receives 0 arguments; `this` is the Download object
Required setup:
  Server.setBaseURL("https://files.example.com");
  const var targetFile = FileSystem.getFolder(FileSystem.Downloads).getChildFile("patch.zip");
Dispatch/mechanics:
  Validates targetFile is a ScriptFile (not string, not directory) -> script error if invalid
    -> getWithParameters(subURL, parameters) builds URL
    -> new ScriptDownloadObject(url, header, file, callback)
    -> GlobalServer::addDownload(download) with deduplication check (operator== on URL)
    -> WebThread manages download queue up to numMaxDownloads concurrent downloads
Pair with:
  setBaseURL -- must be called first
  setNumAllowedDownloads -- configure parallel download limit (default: 1)
  cleanFinishedDownloads -- remove completed downloads from tracking
  getPendingDownloads -- list all tracked downloads
Anti-patterns:
  - Do NOT pass a string path as targetFile -- must be a File object from FileSystem. Passing a string triggers script error "target file is not a file object"
  - Do NOT pass a directory as targetFile -- triggers script error "target file is a directory"
  - The callback receives 0 arguments. Access the Download object as `this` inside the callback (e.g., this.getProgress(), this.data.finished, this.data.success)
Source:
  ScriptingApi.cpp  Server::downloadFile()
    -> validates ScriptFile, checks !isDirectory()
    -> query parameter extraction from subURL if parameters is empty
    -> getWithParameters(subURL, parameters)
    -> GlobalServer::addDownload(newDownload) with ScopedLock on queueLock
    -> deduplication: if URL matches existing download, copies callback via copyCallBackFrom()
