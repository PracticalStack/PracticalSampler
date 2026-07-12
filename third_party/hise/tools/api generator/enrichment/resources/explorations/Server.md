# Server -- C++ Source Exploration

## Resources Consulted

- `enrichment/resources/survey/class_survey.md` -- Server is prerequisite #11 for Download
- `enrichment/resources/survey/class_survey_data.json` -- Server entry (network domain, service role)
- `enrichment/base/Server.json` -- 15 API methods
- No prerequisite Readmes needed (Server has no upstream dependencies)

## Source Files

| File | Content |
|------|---------|
| `hi_scripting/scripting/api/ScriptingApi.h` (lines 1641-1759) | Server class declaration |
| `hi_scripting/scripting/api/ScriptingApi.cpp` (lines 8047-8291) | Server implementation + Wrapper struct |
| `hi_scripting/scripting/api/GlobalServer.h` | GlobalServer struct (persistent backend) |
| `hi_scripting/scripting/api/GlobalServer.cpp` | GlobalServer + WebThread implementation |
| `hi_scripting/scripting/api/ScriptingApiObjects.h` (lines 954-1066) | ScriptDownloadObject declaration |
| `hi_scripting/scripting/api/ScriptingApiObjects.cpp` (lines 1144-1495) | ScriptDownloadObject implementation |
| `hi_scripting/scripting/ScriptProcessorModules.cpp` (line 314) | Registration as global API class |
| `hi_scripting/scripting/components/ScriptingPanelTypes.cpp` (lines 1955-2255) | ServerController debug panel (USE_BACKEND) |

---

## Class Declaration

```cpp
class Server : public ApiClass,
               public ScriptingObject,
               public GlobalServer::Listener
```

**Inheritance:**
- `ApiClass` -- standard namespace-style API class (not `ConstScriptingObject`; no instance creation needed)
- `ScriptingObject` -- provides `getScriptProcessor()`, `reportScriptError()`
- `GlobalServer::Listener` -- receives queue change notifications

**Identity:** `getObjectName()` returns `"Server"`.

**Registration:** Global singleton, registered via `scriptEngine->registerApiClass(serverObject = new ScriptingApi::Server(this))` in `JavascriptMidiProcessor::registerApiClasses()`. The `Server` identifier is reserved -- user-defined namespaces cannot use it (enforced in `JavascriptEngineParser.cpp`).

**Private members:**
- `GlobalServer& globalServer` -- reference to the persistent GlobalServer backend
- `WeakCallbackHolder serverCallback` -- user-supplied callback for server activity
- `JavascriptProcessor* jp` -- owning script processor
- `struct Wrapper` -- API method wrapper declarations

---

## Constructor -- Constants and Method Registration

```cpp
ScriptingApi::Server::Server(JavascriptProcessor* jp_):
    ApiClass(5),  // 5 constants
    ScriptingObject(dynamic_cast<ProcessorWithScriptingContent*>(jp_)),
    jp(jp_),
    globalServer(*getScriptProcessor()->getMainController_()->getJavascriptThreadPool().getGlobalServer()),
    serverCallback(getScriptProcessor(), this, {}, 1)
{
    globalServer.addListener(this);

    addConstant("StatusNoConnection", StatusNoConnection);     // 0
    addConstant("StatusOK", StatusOK);                         // 200
    addConstant("StatusNotFound", StatusNotFound);             // 404
    addConstant("StatusServerError", StatusServerError);       // 500
    addConstant("StatusAuthenticationFail", StatusAuthenticationFail); // 403
    ...
}
```

### Status Code Constants

| Name | Value | Type | Description |
|------|-------|------|-------------|
| `StatusNoConnection` | 0 | int | No internet connection or timeout |
| `StatusOK` | 200 | int | HTTP 200 success |
| `StatusNotFound` | 404 | int | HTTP 404 not found |
| `StatusServerError` | 500 | int | HTTP 500 internal server error |
| `StatusAuthenticationFail` | 403 | int | HTTP 403 forbidden/auth failure |

Defined as enum `StatusCodes` in the class declaration.

### Method Registrations

**Typed (ADD_TYPED_API_METHOD_N):**
- `callWithPOST`: `(String, ComplexType, Function)`
- `callWithGET`: `(String, ComplexType, Function)`
- `setHttpHeader`: `(String)`
- `downloadFile`: `(String, JSON, ScriptObject, Function)`
- `setServerCallback`: `(Function)`

**Untyped (ADD_API_METHOD_N):**
- `setBaseURL` (1 arg)
- `getPendingDownloads` (0 args)
- `getPendingCalls` (0 args)
- `isOnline` (0 args)
- `resendLastCall` (0 args)
- `setNumAllowedDownloads` (1 arg)
- `cleanFinishedDownloads` (0 args)
- `isEmailAddress` (1 arg)
- `setTimeoutMessageString` (1 arg)
- `setEnforceTrailingSlash` (1 arg)

### Diagnostic Registrations (USE_BACKEND only)

Three methods have raw diagnostics via `ADD_CALLBACK_DIAGNOSTIC_RAW`:
- `callWithPOST` -> `checkBaseURLAndCallbackArgs<2, 2>` (expects 2 callback args, callback at arg index 2)
- `callWithGET` -> `checkBaseURLAndCallbackArgs<2, 2>` (same)
- `downloadFile` -> `checkBaseURLAndCallbackArgs<0, 3>` (expects 0 callback args, callback at arg index 3)

One method has standard callback diagnostic:
- `setServerCallback` -> `ADD_CALLBACK_DIAGNOSTIC(serverCallback, setServerCallback, 0)` (callback arg at index 0)

The `checkBaseURLAndCallbackArgs` template method:
```cpp
template <int E, int I> static ApiClass::DiagnosticResult checkBaseURLAndCallbackArgs(ApiClass* c, const Identifier& fName, const Array<var>& args)
{
    if (auto s = dynamic_cast<Server*>(c))
    {
        if (!s->globalServer.isBaseURLDefined())
            return DiagnosticResult::fail("setBaseURL not called");
        return WeakCallbackHolder::checkCallbackNumArgs<E, I>(c, fName, args);
    }
    return DiagnosticResult::fail("not a Server object");
};
```

This validates:
1. `setBaseURL()` has been called before any server call
2. The callback function has the expected number of arguments

For `callWithGET`/`callWithPOST`: callback must accept 2 args (status, response)
For `downloadFile`: callback must accept 0 args (download object is `this` context)

---

## GlobalServer -- The Persistent Backend

The Server scripting API is a thin wrapper around `GlobalServer`, which lives on the `JavascriptThreadPool` and survives script recompilation. This is explicitly stated in the header comment: "This object will surpass the lifetime of a server API object."

### GlobalServer Structure

```cpp
struct GlobalServer: public ControlledObject
{
    enum class State { Inactive, Pause, Idle, WaitingForResponse, numStates };
    // Listener interface, PendingCallback inner class, WebThread inner class
    // ...
private:
    bool initialised;  // true in USE_BACKEND, false otherwise (set later in frontend)
    WebThread internalThread;
    PendingCallback::Ptr lastCall;
    URL baseURL;
    String extraHeader;
    Array<WeakReference<Listener>> listeners;
public:
    bool addTrailingSlashes = true;
    ProfileCollection webProfile;
    // ...
};
```

### Ownership Chain

`MainController` -> `JavascriptThreadPool` -> `GlobalServer` (unique_ptr)

Created in `JavascriptThreadPool` constructor:
```cpp
// ScriptProcessor.cpp line 2452
globalServer(new GlobalServer(mc))
```

The `Server` scripting object obtains its reference via:
```cpp
globalServer(*getScriptProcessor()->getMainController_()->getJavascriptThreadPool().getGlobalServer())
```

### State Machine

`GlobalServer::State` enum tracks the server lifecycle:

| State | Condition |
|-------|-----------|
| `Inactive` | WebThread not running (before `setBaseURL()` call) |
| `Pause` | WebThread running but `running` flag is false (after `stop()`) |
| `Idle` | WebThread running, no pending callbacks |
| `WaitingForResponse` | WebThread running, pending callbacks exist |

Implementation:
```cpp
GlobalServer::State GlobalServer::getServerState() const
{
    if (!internalThread.isThreadRunning()) return State::Inactive;
    if (!internalThread.running) return State::Pause;
    if (internalThread.pendingCallbacks.isEmpty()) return State::Idle;
    return State::WaitingForResponse;
}
```

### Initialization Guard

In frontend builds (`USE_FRONTEND`), `initialised` starts as `false`. The WebThread's main loop waits (200ms polling) until `setInitialised()` is called:

```cpp
// WebThread::run()
if (parent.initialised)
{
    // process callbacks and downloads
}
else
{
    Thread::wait(200); // postpone until loaded
}
```

`setInitialised()` is called:
- In backend: immediately (field initialized to `true`)
- In frontend: from `FrontEndProcessor.cpp` (line 483) after loading completes
- Also called from `MainController.cpp` (lines 434, 584) in various init paths

This means **server calls queued during frontend loading are buffered** and processed once initialization completes.

---

## GlobalServer::WebThread -- The Request Processing Thread

```cpp
struct WebThread : public Thread, public ProfiledRecordingSession
{
    WebThread(GlobalServer& p);
    GlobalServer& parent;
    void run() override;
    CriticalSection queueLock;
    std::atomic<bool> cleanDownloads = { false };
    std::atomic<bool> running = { true };
    int numMaxDownloads = 1;
    ReferenceCountedArray<PendingCallback> pendingCallbacks;
    ReferenceCountedArray<ScriptingObjects::ScriptDownloadObject> pendingDownloads;
    var timeoutMessage;  // default: "{}"
};
```

Thread name: `"Server Thread"`.

### Thread Main Loop

The WebThread::run() method processes two queues:

**1. Download Queue:**
- Iterates all pending downloads
- Starts waiting downloads up to `numMaxDownloads` limit
- Stops downloads that exceed the parallel limit
- Flushes finished downloads' temporary files
- Cleans finished downloads when `cleanDownloads` flag is set
- Fires `sendMessage(true)` (download queue changed) when state changes

**2. Callback Queue (only when `running` is true):**
- Removes callbacks from front of queue one at a time
- Creates `WebInputStream` from callback's URL using `job->isPost` flag
- Uses `HISE_SCRIPT_SERVER_TIMEOUT` (default 10000ms) for timeout
- Reads entire stream as string, or uses `timeoutMessage` if stream is null
- Parses response as JSON; if JSON parse fails, passes raw string
- Calls the script callback with `(status, response)` as 2 arguments
- Stores response in `job->responseObj` for debug panel inspection
- Fires `sendMessage(false)` (callback queue changed) after processing

**Polling interval:** 500ms wait between iterations.

### PendingCallback Structure

```cpp
struct PendingCallback : public ReferenceCountedObject
{
    WeakCallbackHolder f;
    URL url;
    String extraHeader;
    bool isPost;
    int status = 0;
    const uint32 creationTimeMs;
    uint32 requestTimeMs = 0;
    uint32 completionTimeMs = 0;
    var responseObj;
    int profileTrackId = -1;
};
```

Key: `f` is the HiseScript callback. `setHighPriority()` is called on construction, and `incRefCount()` is used to prevent GC.

### Thread Timeout Constant

```cpp
// hi_scripting.h
#ifndef HISE_SCRIPT_SERVER_TIMEOUT
#define HISE_SCRIPT_SERVER_TIMEOUT 10000
#endif
```

Default: 10 seconds. Configurable at compile time.

---

## Server Activity Listener Pattern

The Server class implements `GlobalServer::Listener`:

```cpp
void queueChanged(int numItems) override
{
    if (serverCallback)
    {
        if(numItems < 2)
            serverCallback.call1(numItems == 1);
    }
}

void downloadQueueChanged(int) override
{
    // empty -- not used
}
```

The `queueChanged` callback:
- Only fires when `serverCallback` is set (via `setServerCallback`)
- Only calls the script callback when `numItems < 2` (0 or 1 items remain)
- Passes `true` if 1 item remains (server is busy), `false` if 0 items remain (server is idle)
- The callback signature is: `function(isActive)` -- 1 argument

This is called synchronously from the WebThread (via `GlobalServer::sendMessage(false)`) after processing the callback queue.

---

## URL Construction -- getWithParameters

The URL construction logic handles three cases for the `parameters` argument:

```cpp
juce::URL GlobalServer::getWithParameters(String subURL, var parameters)
{
    auto url = baseURL.getChildURL(subURL);

    if (auto d = parameters.getDynamicObject())
    {
        bool isComplexObject = false;
        for(const auto& v: d->getProperties())
            isComplexObject |= v.value.isArray() || v.value.getDynamicObject() != nullptr;

        if(isComplexObject)
        {
            extraHeader = "Content-Type: application/json";
            url = url.withPOSTData(JSON::toString(parameters, true));
        }
        else
        {
            for (auto& p : d->getProperties())
                url = url.withParameter(p.name.toString(), p.value.toString());
        }
    }
    else if (parameters.isString())
    {
        url = url.withPOSTData(parameters.toString());
    }

    return url;
}
```

**Three parameter modes:**
1. **Simple JSON object** (all values are primitives) -- appended as URL query parameters
2. **Complex JSON object** (any value is array or nested object) -- sent as JSON POST data with `Content-Type: application/json` header. Note: this MUTATES `extraHeader` on the GlobalServer.
3. **String** -- sent as raw POST data

---

## POST Trailing Slash Logic

```cpp
void ScriptingApi::Server::callWithPOST(String subURL, var parameters, var callback)
{
    // ...
    const bool isNotAFile = !subURL.containsChar('.');
    const bool trailingSlashMissing = !subURL.endsWithChar('/');
    
    if(isNotAFile && trailingSlashMissing && globalServer.addTrailingSlashes)
    {
        // We need to append a slash in order to prevent redirecting to a GET call
        subURL << '/';
    }
    // ...
}
```

By default, POST calls have a trailing slash appended to prevent HTTP 301 redirect from POST to GET. This can be disabled with `setEnforceTrailingSlash(false)`. The trailing slash is NOT added if:
- The subURL contains a dot (assumed to be a file endpoint)
- The subURL already ends with a slash

---

## isOnline Implementation

```cpp
bool ScriptingApi::Server::isOnline()
{
    const char* urlsToTry[] = { "https://google.com/generate_204", "https://amazon.com", nullptr };
    for (const char** url = urlsToTry; *url != nullptr; ++url)
    {
        URL u(*url);
        auto ms = Time::getMillisecondCounter();
        std::unique_ptr<InputStream> in(u.createInputStream(false, nullptr, nullptr, String(), HISE_SCRIPT_SERVER_TIMEOUT, nullptr));
        dynamic_cast<JavascriptProcessor*>(getScriptProcessor())->getScriptEngine()->extendTimeout(Time::getMillisecondCounter() - ms);
        if (in != nullptr) return true;
    }
    return false;
}
```

Key details:
- Tries Google's 204 endpoint first, then Amazon as fallback
- **Blocks the calling thread** (scripting thread) for up to `HISE_SCRIPT_SERVER_TIMEOUT` (10s) per URL
- Uses `extendTimeout()` to prevent the HiseScript engine from timing out during the check
- Returns true as soon as any URL responds

---

## isEmailAddress Implementation

```cpp
bool ScriptingApi::Server::isEmailAddress(String email)
{
    URL u("");
    return u.isProbablyAnEmailAddress(email);
}
```

Delegates to JUCE's `URL::isProbablyAnEmailAddress()` -- basic regex-style validation.

---

## resendLastCall Implementation

```cpp
bool ScriptingApi::Server::resendLastCall()
{
    if(isOnline())
    {
        return globalServer.resendLastCallback();
    }
    return false;
}
```

Key: checks `isOnline()` first (blocking), then calls `GlobalServer::resendLastCallback()`:

```cpp
bool GlobalServer::resendLastCallback()
{
    if(lastCall != nullptr)
    {
        auto r = resendCallback(lastCall.get());
        return r.wasOk();
    }
    return false;
}

Result GlobalServer::resendCallback(PendingCallback* p)
{
    if (p != nullptr)
    {
        if (p->f)  // WeakCallbackHolder still valid
        {
            p->reset();  // clears requestTimeMs, completionTimeMs, responseObj, status
            internalThread.pendingCallbacks.add(p);
            internalThread.notify();
            return Result::ok();
        }
        else
            return Result::fail("Callback was from previous compilation");
    }
    else
        return Result::fail("Callback was deleted");
}
```

The `lastCall` is set every time `addPendingCallback()` is called, so it tracks the most recent GET or POST call. If the script was recompiled since the last call, the callback's WeakCallbackHolder will be invalid and resend will fail.

---

## downloadFile Implementation Details

```cpp
var ScriptingApi::Server::downloadFile(String subURL, var parameters, var targetFile, var callback)
{
    if (auto sf = dynamic_cast<ScriptingObjects::ScriptFile*>(targetFile.getObject()))
    {
        // URL parameter extraction from query string
        if (subURL.contains("?") && parameters.getDynamicObject() != nullptr && parameters.getDynamicObject()->getProperties().isEmpty())
        {
            // Parse "?key=value&key2=value2" from subURL into parameters object
            // Strips query from subURL, builds DynamicObject
        }

        if (sf->f.isDirectory())
        {
            reportScriptError("target file is a directory");
            return var();
        }

        auto urlToUse = getWithParameters(subURL, parameters);
        if(urlToUse.isWellFormed())
        {
            ScriptingObjects::ScriptDownloadObject::Ptr p = new ScriptingObjects::ScriptDownloadObject(
                getScriptProcessor(), urlToUse, globalServer.getExtraHeader(), sf->f, callback);
            return globalServer.addDownload(p);
        }
    }
    else
    {
        reportScriptError("target file is not a file object");
    }
    return var();
}
```

Key behaviors:
- `targetFile` must be a `ScriptFile` object (not a raw string) -- throws script error otherwise
- Cannot download to a directory -- throws script error
- If `subURL` contains query parameters AND the `parameters` object is empty, the query string is parsed and moved to the parameters object
- Returns a `Download` object (ScriptDownloadObject)

### Download Deduplication

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

Downloads are deduplicated by URL (`operator==` compares `downloadURL`). If a download with the same URL already exists, the callback is replaced and the existing download object is returned. This means calling `downloadFile` twice with the same URL does not create a duplicate download.

---

## ScriptDownloadObject (Download class) -- Overview for Server Context

The Download object returned by `downloadFile` is `ScriptingObjects::ScriptDownloadObject`:

**Key fields:**
- `data` -- DynamicObject with properties: `numTotal`, `numDownloaded`, `finished`, `success`, `aborted`
- `callback` -- WeakCallbackHolder with 0 args; `this` (the Download) is set as the callback's `thisObject`

**Callback invocation:** The download callback receives 0 explicit arguments. The Download object itself is accessible as `this` inside the callback function. Properties like `getProgress()`, `isRunning()`, `getDownloadSpeed()` etc. are called on `this`.

The `data` constant is also available directly on the Download object (registered as `addConstant("data", ...)`).

**Download lifecycle states (from `getStatusText()`):**
- `"Waiting"` -- initial state, before WebThread picks it up
- `"Downloading"` -- actively downloading
- `"Paused"` -- stopped but not aborted
- `"Aborted"` -- abort requested
- `"Completed"` -- finished (success or failure)

**Resume support:** If the target file already exists and has content, the download automatically resumes using HTTP Range headers. This is transparent to the user.

---

## setBaseURL -- Thread Lifecycle

```cpp
void GlobalServer::setBaseURL(String url)
{
    startTime = Time::getMillisecondCounter();
    baseURL = URL(url);
    internalThread.startThread();
}
```

Calling `setBaseURL()` starts the WebThread. This is the activation point for the entire server subsystem. Without calling `setBaseURL()`, the WebThread never starts and no requests are processed.

`isBaseURLDefined()` checks `!baseURL.isEmpty()` -- used by the diagnostic system to warn if GET/POST/download calls are made before `setBaseURL()`.

---

## setHttpHeader

```cpp
void GlobalServer::setHttpHeader(String newHeader)
{
    extraHeader = newHeader;
}
```

Sets the extra HTTP header string for ALL subsequent requests. The header is copied into each `PendingCallback` when it is added to the queue (`p->extraHeader = extraHeader`). The header is also passed to download objects on creation.

Note: `getWithParameters()` can also mutate `extraHeader` when complex JSON objects are detected (sets `Content-Type: application/json`). This is a side effect that affects all subsequent requests.

---

## Profiling Infrastructure

Under `HISE_INCLUDE_PROFILING_TOOLKIT`, GET and POST calls record profiling data:

```cpp
GlobalServer::GlobalServer(MainController* mc):
    ControlledObject(mc), internalThread(*this)
{
    webProfile.setPrefix("Server.");
    webProfile.setHolder(&mc->getDebugSession(), true);
    webProfile.setSourceType(DebugSession::ProfileDataSource::SourceType::Server);
    pCallGET = webProfile.add("callWithGET()");
    pCallPOST = webProfile.add("callWithPOST()");
    pResponse = webProfile.add("doRequest()");
    pDownload = webProfile.add("download()");
}
```

Profile track events are opened when a GET/POST is initiated and closed when the response arrives. Debug data items record request parameters and server responses.

---

## ServerController Debug Panel (USE_BACKEND only)

`ServerControllerPanel` (a `PanelWithProcessorConnection`) creates a `ServerController` component that provides:

- **Request table:** Shows all pending/completed callbacks with columns: StatusLED, Status, URL, Timestamp, Duration, Parameters, Response, Resend
- **Download table:** Shows all downloads with columns: StatusLED, Status, URL, Size, Speed, Pause, Abort, ShowFile
- **State indicator:** Shows current `GlobalServer::State`
- **Controls:** Toggle request/download views, pause/resume server, clear finished items

The panel registers as a `GlobalServer::Listener` and updates tables when queue changes occur.

The "Resend" button calls `GlobalServer::resendCallback()` directly. The "Parameters" and "Response" columns open JSON editors for inspection/editing.

---

## Preprocessor Guards Summary

| Guard | Where Used | Effect |
|-------|-----------|--------|
| `USE_BACKEND` | `checkBaseURLAndCallbackArgs` diagnostic template, `initialised` default value | Diagnostics only available in HISE IDE |
| `HISE_INCLUDE_PROFILING_TOOLKIT` | `callWithGET`, `callWithPOST`, WebThread response handling | Profiling data collection |
| `HISE_SCRIPT_SERVER_TIMEOUT` | Timeout for all HTTP operations | Default 10000ms, compile-time configurable |

---

## Threading Model

| Thread | What Happens |
|--------|-------------|
| **Scripting Thread** | `callWithGET()`, `callWithPOST()`, `downloadFile()` queue requests. `isOnline()` blocks synchronously. |
| **Server Thread** (WebThread) | Processes callback queue sequentially, manages downloads. Calls script callbacks via `WeakCallbackHolder.call()`. |
| **UI Thread** | ServerController panel updates via timer polling (`requestDirty`/`downloadsDirty` flags). |

The `queueLock` (CriticalSection) protects the download list. The callback list uses `ReferenceCountedArray` which provides thread-safe reference counting but the array itself is accessed from both the scripting thread (add) and server thread (remove). The `pendingCallbacks.add()` and `pendingCallbacks.removeAndReturn(0)` calls are not explicitly locked -- they rely on JUCE's ReferenceCountedArray being safe for concurrent add/remove-from-front patterns.

---

## Wrapper Struct

All methods use standard `API_VOID_METHOD_WRAPPER_N` / `API_METHOD_WRAPPER_N` patterns -- no typed wrappers at the C++ wrapper level:

```cpp
struct ScriptingApi::Server::Wrapper
{
    API_VOID_METHOD_WRAPPER_1(Server, setBaseURL);
    API_VOID_METHOD_WRAPPER_3(Server, callWithPOST);
    API_VOID_METHOD_WRAPPER_3(Server, callWithGET);
    API_METHOD_WRAPPER_4(Server, downloadFile);
    API_VOID_METHOD_WRAPPER_1(Server, setHttpHeader);
    API_VOID_METHOD_WRAPPER_1(Server, setEnforceTrailingSlash);
    API_METHOD_WRAPPER_0(Server, getPendingDownloads);
    API_METHOD_WRAPPER_0(Server, getPendingCalls);
    API_METHOD_WRAPPER_0(Server, isOnline);
    API_VOID_METHOD_WRAPPER_1(Server, setNumAllowedDownloads);
    API_VOID_METHOD_WRAPPER_0(Server, cleanFinishedDownloads);
    API_VOID_METHOD_WRAPPER_1(Server, setServerCallback);
    API_VOID_METHOD_WRAPPER_1(Server, setTimeoutMessageString);
    API_METHOD_WRAPPER_0(Server, resendLastCall);
    API_METHOD_WRAPPER_1(Server, isEmailAddress);
};
```

---

## Complete Method List (15 methods)

| Method | Args | Typed? | Return |
|--------|------|--------|--------|
| `setBaseURL` | 1 | No | void |
| `callWithGET` | 3 | Yes (String, ComplexType, Function) | void |
| `callWithPOST` | 3 | Yes (String, ComplexType, Function) | void |
| `downloadFile` | 4 | Yes (String, JSON, ScriptObject, Function) | var (Download) |
| `setHttpHeader` | 1 | Yes (String) | void |
| `setEnforceTrailingSlash` | 1 | No | void |
| `getPendingDownloads` | 0 | No | var (Array) |
| `getPendingCalls` | 0 | No | var (Array) |
| `setNumAllowedDownloads` | 1 | No | void |
| `isOnline` | 0 | No | bool |
| `cleanFinishedDownloads` | 0 | No | void |
| `setServerCallback` | 1 | Yes (Function) | void |
| `isEmailAddress` | 1 | No | bool |
| `setTimeoutMessageString` | 1 | No | void |
| `resendLastCall` | 0 | No | bool |
