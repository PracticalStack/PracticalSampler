Download (object)
Obtain via: Server.downloadFile(url, params, targetFile, callback)

Handle for an active or completed HTTP file download with progress tracking,
pause/resume, and abort control. Executes on the Server's background WebThread;
API methods set atomic flags that the WebThread acts upon asynchronously.

The callback passed to Server.downloadFile() takes zero arguments. The Download
object is bound as `this`, so state is accessed via `this.data`, `this.getProgress()`,
etc. The `data` constant is a mutable object with properties: numTotal, numDownloaded,
finished, success, aborted.

Constants:
  State:
    data = (DynamicObject)    Mutable object carrying download state (numTotal, numDownloaded, finished, success, aborted)

Common mistakes:
  - Passing the Download as a callback argument instead of using `this` -- the
    callback takes zero arguments; the Download is the `this` context.
  - Calling resume() after abort() and expecting it to work -- abort deletes the
    target file and marks the download as terminal. Start a new download instead.
  - Treating getStatusText() == "Completed" as success -- connection failures also
    produce "Completed". Always check data.success.
  - Reading getProgress()/isRunning() immediately after stop()/abort() and expecting
    updated state -- flag-based; actual state change happens on WebThread (up to 500ms).

Example:
  const var dl = Server.downloadFile("https://example.com/file.zip", {},
      FileSystem.getFolder(FileSystem.Downloads).getChildFile("file.zip"),
      function()
      {
          if (this.data.finished)
          {
              if (this.data.success)
                  Console.print("Download complete: " + this.getDownloadedTarget());
              else
                  Console.print("Download failed");
          }
          else
          {
              Console.print("Progress: " + Math.round(this.getProgress() * 100) + "%");
              Console.print("Speed: " + this.getDownloadSpeed() + " bytes/s");
          }
      });

Methods (11):
  abort                   getDownloadedTarget     getDownloadSize
  getDownloadSpeed        getFullURL              getNumBytesDownloaded
  getProgress             getStatusText           isRunning
  resume                  stop
