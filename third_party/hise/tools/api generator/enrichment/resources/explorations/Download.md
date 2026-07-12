# Download -- C++ Source Exploration

## Resources Consulted
- `enrichment/resources/survey/class_survey_data.json` -- Download entry (domain: network, role: handle)
- `enrichment/phase1/Server/Readme.md` -- Prerequisite class analysis (architecture, state machine, download system overview)
- `enrichment/base/Download.json` -- 11 API methods, all zero-parameter

## Source Locations
- **Header:** `HISE/hi_scripting/scripting/api/ScriptingApiObjects.h` lines 954-1066
- **Implementation:** `HISE/hi_scripting/scripting/api/ScriptingApiObjects.cpp` lines 1129-1495
- **GlobalServer integration:** `HISE/hi_scripting/scripting/api/GlobalServer.h` and `GlobalServer.cpp`
- **Factory (Server.downloadFile):** `HISE/hi_scripting/scripting/api/ScriptingApi.cpp` lines 8194-8240

---

## Class Declaration

```cpp
struct ScriptDownloadObject : public ConstScriptingObject,
                              public URL::DownloadTask::Listener
```

- **HiseScript name:** `"Download"` (from `getObjectName()`)
- **Inheritance:** `ConstScriptingObject` (scripting API base) + `URL::DownloadTask::Listener` (JUCE download callbacks)
- **Ref-counted:** via `using Ptr = ReferenceCountedObjectPtr<ScriptDownloadObject>;`
- **Nested struct:** `Wrapper` (API method wrappers)

### Constructor Signature

```cpp
ScriptDownloadObject(ProcessorWithScriptingContent* pwsc, const URL& url,
                     const String& extraHeader, const File& targetFile, var callback);
```

Base class initialized with `ConstScriptingObject(pwsc, 3)` -- capacity for 3 constants (only 1 used: `data`).

---

## Constructor Analysis

### Constant Registration

Only one constant is registered:

```cpp
data = new DynamicObject();
addConstant("data", var(data.get()));
```

The `data` constant is a mutable DynamicObject that holds download state. It is accessible in HiseScript as `download.data`. This is the primary mechanism for the callback function to inspect download state.

### Callback Setup

```cpp
callback(pwsc, this, callback_, 0),   // WeakCallbackHolder with 0 expected args
...
callback.incRefCount();
callback.setThisObject(this);
```

The callback:
- Takes **zero** explicit arguments (4th param = 0)
- Has `this` (the ScriptDownloadObject) set as the `this` context
- Uses `incRefCount()` to prevent the anonymous function from being garbage collected
- Is invoked as `callback.call(nullptr, 0)` -- no arguments passed

**Callback access pattern in HiseScript:** Inside the callback, `this` refers to the Download object itself. The callback accesses state via `this.data.numDownloaded`, `this.data.finished`, `this.getProgress()`, etc.

### API Method Registration

All 11 methods use `ADD_API_METHOD_0` (untyped, zero parameters):

```cpp
ADD_API_METHOD_0(resume);
ADD_API_METHOD_0(stop);
ADD_API_METHOD_0(abort);
ADD_API_METHOD_0(isRunning);
ADD_API_METHOD_0(getProgress);
ADD_API_METHOD_0(getFullURL);
ADD_API_METHOD_0(getStatusText);
ADD_API_METHOD_0(getDownloadedTarget);
ADD_API_METHOD_0(getDownloadSpeed);
ADD_API_METHOD_0(getNumBytesDownloaded);
ADD_API_METHOD_0(getDownloadSize);
```

**No typed methods.** All wrappers are `API_METHOD_WRAPPER_0(ScriptDownloadObject, ...)`.

---

## Atomic State Flags

The object uses five atomic booleans to manage its state machine:

```cpp
std::atomic<bool> isWaitingForStop = { false };
std::atomic<bool> isWaitingForStart = { true };   // starts true!
std::atomic<bool> isRunning_ = { false };
std::atomic<bool> isFinished = { false };
std::atomic<bool> shouldAbort = { false };
```

These flags are set from the scripting thread (via API methods) and read/acted upon by the Server Thread (WebThread). This is a lock-free coordination pattern -- the API methods just set flags, and the WebThread run loop acts on them.

### State Transitions

| Initial state | Action | Flags changed |
|--------------|--------|---------------|
| Created | (default) | `isWaitingForStart = true` |
| Waiting | WebThread calls `start()` | `isWaitingForStart = false`, `isRunning_ = true` |
| Running | script calls `stop()` | `isWaitingForStop = true` |
| Running | WebThread calls `stopInternal()` | `isRunning_ = false`, `isFinished = false` |
| Stopped | script calls `resume()` | `isWaitingForStart = true` |
| Running | JUCE calls `finished()` | `isRunning_ = false`, `isFinished = true` |
| Running | script calls `abort()` | `shouldAbort = true`, then `stop()` |
| Aborting | WebThread calls `stopInternal()` | `isFinished = true`, file deleted |

---

## The `data` DynamicObject -- Property Schema

The `data` object carries download state accessible from HiseScript. Properties are set at various lifecycle points:

### Properties set in `start()` (initial download):

```cpp
data->setProperty("numTotal", 0);
data->setProperty("numDownloaded", 0);
data->setProperty("finished", false);
data->setProperty("success", false);
data->setProperty("aborted", false);
```

### Properties set in `start()` (connection failed, status != 200):

```cpp
data->setProperty("numTotal", 0);
data->setProperty("numDownloaded", 0);
data->setProperty("finished", true);
data->setProperty("success", false);
data->setProperty("aborted", false);
```

### Properties updated in `progress()`:

```cpp
data->setProperty("numTotal", totalLength + existingBytesBeforeResuming);
data->setProperty("numDownloaded", bytesDownloaded + existingBytesBeforeResuming);
```

### Properties set in `finished()`:

```cpp
data->setProperty("success", success);
data->setProperty("finished", true);
```

### Properties set in `stopInternal()`:

```cpp
data->setProperty("success", false);
data->setProperty("finished", true);
// If aborting:
data->setProperty("aborted", true);
```

### Properties set in `resumeInternal()` (already complete):

```cpp
data->setProperty("success", true);
data->setProperty("finished", true);
```

### Properties set in `resumeInternal()` (resuming partial):

```cpp
data->setProperty("numTotal", numTotal);
data->setProperty("numDownloaded", existingBytesBeforeResuming);
data->setProperty("finished", false);
data->setProperty("success", false);
```

### Complete Property Table

| Property | Type | Description |
|----------|------|-------------|
| `numTotal` | int64 | Total download size in bytes (includes pre-existing bytes on resume) |
| `numDownloaded` | int64 | Bytes downloaded so far (includes pre-existing bytes on resume) |
| `finished` | bool | Whether the download has completed (success or failure) |
| `success` | bool | Whether the download completed successfully |
| `aborted` | bool | Whether the download was aborted (only set to true on abort) |

---

## Factory / obtainedVia

Download objects are created exclusively by `Server.downloadFile()`:

```cpp
// In ScriptingApi::Server::downloadFile():
ScriptingObjects::ScriptDownloadObject::Ptr p =
    new ScriptingObjects::ScriptDownloadObject(
        getScriptProcessor(), urlToUse, globalServer.getExtraHeader(), sf->f, callback);
return globalServer.addDownload(p);
```

The `globalServer.addDownload()` method handles **deduplication**: if a download with the same URL already exists in the pending queue, it replaces the callback on the existing object and returns it instead of adding a new one:

```cpp
var GlobalServer::addDownload(ScriptingObjects::ScriptDownloadObject::Ptr newDownload)
{
    ScopedLock sl(internalThread.queueLock);
    for (auto ep : internalThread.pendingDownloads)
    {
        if (*newDownload == *ep)
        {
            ep->copyCallBackFrom(newDownload.get());
            return var(ep);
        }
    }
    internalThread.pendingDownloads.add(newDownload);
    internalThread.notify();
    sendMessage(true);
    return var(newDownload.get());
}
```

The equality operator compares by URL:
```cpp
bool operator==(const ScriptDownloadObject& other) const
{
    return downloadURL == other.downloadURL;
}
```

---

## WebThread Integration -- Download Lifecycle

The Server's WebThread run loop (in `GlobalServer::WebThread::run()`) manages all Download objects:

```
for each download in pendingDownloads:
    if d->isWaitingForStart && numActiveDownloads < numMaxDownloads:
        d->start()          // Actually begins the HTTP download
    if d->isWaitingForStop:
        d->stopInternal()   // Pauses the download
    if d->isRunning():
        if numActiveDownloads >= numMaxDownloads:
            d->stop()       // Throttle: too many concurrent downloads
        else:
            numActiveDownloads++
    if d->isFinished:
        d->flushTemporaryFile()  // Merge resume temp file into target
    if cleanDownloads && d->isFinished:
        remove from pendingDownloads list
```

Key observations:
- Downloads are **not started immediately** on creation. They sit in `isWaitingForStart` state until the WebThread picks them up.
- The `numMaxDownloads` limit (default: 1, configurable via `Server.setNumAllowedDownloads()`) enforces parallel download concurrency. Excess downloads are stopped and will be picked up when slots free.
- `cleanFinishedDownloads()` sets a flag that causes the WebThread to remove finished downloads on its next iteration.
- The WebThread loop runs every 500ms (`Thread::wait(500)`).

---

## Resume Mechanism

The resume mechanism uses HTTP Range headers for partial downloads:

### `start()` Method

```cpp
void ScriptDownloadObject::start()
{
    isWaitingForStart = false;

    if (targetFile.existsAsFile() && targetFile.getSize() > 0)
    {
        resumeInternal();   // Target exists, try to resume
        return;
    }
    // Fresh download: create InputStream to check status, then downloadToFile
    auto wis = downloadURL.createInputStream(false, ..., &status);
    if (status == 200)
    {
        isRunning_ = true;
        download = downloadURL.downloadToFile(targetFile, options).release();
        // Set initial data properties...
    }
    else
    {
        // Connection failed
        isFinished = true;
        // Set failure data properties...
    }
}
```

### `resumeInternal()` Method

1. Checks existing file size: `existingBytesBeforeResuming = targetFile.getSize()`
2. Creates a probe `WebInputStream` to get total remote file size
3. If local file size == remote total: marks as complete immediately (already fully downloaded)
4. If local < remote and status 200: creates a **temporary sibling file** (`resumeFile = targetFile.getNonexistentSibling(true)`) and downloads with `Range: bytes=N-M` header
5. Otherwise: calls `stopInternal()` to mark as failed

```cpp
String rangeHeader;
rangeHeader << "Range: bytes=" << existingBytesBeforeResuming << "-" << numTotal;
// Downloads remaining bytes to resumeFile
download = downloadURL.downloadToFile(resumeFile, options).release();
```

### `flushTemporaryFile()` Method

When the resumed download completes, the temporary file content is appended to the original target file:

```cpp
void flushTemporaryFile()
{
    if (resumeFile.existsAsFile())
    {
        ScopedPointer<FileInputStream> fis = new FileInputStream(resumeFile);
        FileOutputStream fos(targetFile);
        fos.writeFromInputStream(*fis, -1);
        fos.flush();
        fis = nullptr;
        download = nullptr;
        resumeFile.deleteFile();
    }
}
```

This is also called from the destructor to ensure data integrity.

**Important detail:** The `getProgress()`, `getDownloadSize()`, and `getNumBytesDownloaded()` methods all add `existingBytesBeforeResuming` to their return values, so they report the total progress including the pre-existing portion.

---

## Callback Invocation Pattern

The `call()` method invokes the WeakCallbackHolder:

```cpp
void ScriptDownloadObject::call(bool highPriority)
{
    callback.call(nullptr, 0);
}
```

The `highPriority` parameter is currently unused -- there is a commented-out block (`#if 0`) that previously dispatched to the JavascriptThreadPool with different priority levels. The current implementation always calls directly.

Callback is invoked at these lifecycle points:
1. **`start()`** -- initial call after starting (whether success or failure)
2. **`progress()`** -- throttled to every 100ms
3. **`stopInternal()`** -- when stopped or aborted
4. **`resumeInternal()`** -- when resume finds file already complete
5. **`finished()`** -- when JUCE's DownloadTask completes

---

## Progress Tracking

### Speed Measurement

```cpp
int getDownloadSpeed()
{
    return isRunning() ? jmax((int)bytesInLastSecond, (int)bytesInCurrentSecond) : 0;
}
```

Speed is calculated in the `progress()` callback:
- `bytesInCurrentSecond` accumulates bytes within the current 1-second window
- When 1 second elapses, `bytesInLastSecond = bytesInCurrentSecond` and current resets
- `getDownloadSpeed()` returns the max of both to avoid showing 0 at window boundaries
- Returns 0 when not running

### Callback Throttling

```cpp
void progress(URL::DownloadTask*, int64 bytesDownloaded, int64 totalLength)
{
    // Update internal tracking variables...
    data->setProperty("numTotal", totalLength + existingBytesBeforeResuming);
    data->setProperty("numDownloaded", bytesDownloaded + existingBytesBeforeResuming);

    if ((thisTimeMs - lastTimeMs) > 100)
    {
        call(false);
        lastTimeMs = thisTimeMs;
    }
}
```

The callback is throttled to fire **at most every 100ms** during active downloads. The `data` properties are updated on every `progress()` call from JUCE, but the script callback only fires on the 100ms interval.

---

## Status Text State Machine

```cpp
String getStatusText()
{
    if (isRunning_)        return "Downloading";
    if (shouldAbort)       return "Aborted";
    if (isFinished)        return "Completed";
    if (isWaitingForStop)  return "Paused";
    return "Waiting";
}
```

The check order matters -- `isRunning_` takes priority. Possible return values:

| Value | Condition |
|-------|-----------|
| `"Downloading"` | Download is actively transferring |
| `"Aborted"` | Download was aborted (even if also finished) |
| `"Completed"` | Download finished (check `data.success` for success/failure) |
| `"Paused"` | Download was stopped, can be resumed |
| `"Waiting"` | Download is queued, not yet started |

---

## Threading Model

This class operates across two threads:

1. **Scripting Thread** -- All API methods are called from here. The methods `stop()`, `resume()`, and `abort()` only set atomic flags. They do NOT directly manipulate the download.

2. **Server Thread (WebThread)** -- The GlobalServer's WebThread reads the atomic flags and performs the actual download operations: `start()`, `stopInternal()`, `resumeInternal()`. The JUCE `URL::DownloadTask::Listener` callbacks (`progress()`, `finished()`) are also called from JUCE's internal thread (typically the same as WebThread).

The `data` DynamicObject is written to from the Server Thread and read from the Scripting Thread. There is no explicit synchronization on this object -- it relies on the fact that the scripting callback (`call()`) is dispatched after properties are set.

---

## Private Member Variables

```cpp
int64 bytesInLastSecond = 0;       // Speed: bytes in previous 1s window
int64 bytesInCurrentSecond = 0;    // Speed: bytes in current 1s window
int64 lastBytesDownloaded = 0;     // Speed: last total for delta calculation

int64 bytesDownloaded_ = 0;        // Raw JUCE progress (without resume offset)
int64 totalLength_ = 0;            // Raw JUCE total (without resume offset)

int64 existingBytesBeforeResuming = 0;  // Bytes in target file before resume
File resumeFile;                    // Temporary file for resumed download

uint32 lastTimeMs = 0;             // Throttle: last callback fire time
uint32 lastSpeedMeasure = 0;       // Speed: last 1s window reset time

DynamicObject::Ptr data;           // Script-visible state object
URL downloadURL;                   // The full URL being downloaded
File targetFile;                   // Destination file
WeakCallbackHolder callback;       // Script callback function
String extraHeaders;               // HTTP headers from Server.setHttpHeader()
ScopedPointer<URL::DownloadTask> download;  // Active JUCE download task
JavascriptProcessor* jp = nullptr;  // Owning script processor
```

---

## Preprocessor Guards

None. The Download class has no `#if USE_BACKEND` or other conditional compilation. It is available in all build targets (backend, frontend, DLL).

The only preprocessor-dependent value is `HISE_SCRIPT_SERVER_TIMEOUT` (default: 10000ms), which is used in `start()` and `resumeInternal()` for the HTTP connection timeout.

---

## Key Design Patterns

### Deduplication by URL
When `Server.downloadFile()` is called with a URL that is already being downloaded, the existing Download object's callback is replaced (via `copyCallBackFrom()`) and the existing object is returned. This prevents duplicate downloads of the same resource.

### Flag-based Cross-thread Coordination
API methods set atomic flags; the WebThread acts on them. This avoids locks in the API methods (which run on the scripting thread).

### Resume via Temporary Sibling File
Resumed downloads write to a temporary file (e.g., `targetFile_1.zip`), then `flushTemporaryFile()` appends the new data to the original target file. This preserves the original file during resume.

### Callback `this` Binding
The Download object itself is the `this` context in the callback. The callback takes 0 arguments -- all state is accessed via `this.data`, `this.getProgress()`, etc.

### Progress Offset Accounting
All public-facing byte counts (`getProgress()`, `getDownloadSize()`, `getNumBytesDownloaded()`, and `data` properties) include the `existingBytesBeforeResuming` offset, so resumed downloads report total progress, not just the remaining portion.
