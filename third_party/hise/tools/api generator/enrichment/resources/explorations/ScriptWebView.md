# ScriptWebView -- C++ Source Exploration

## Resources Consulted

- `enrichment/resources/explorations/ScriptComponent_base.md` -- base class infrastructure
- `enrichment/resources/base_methods/ScriptComponent.md` -- pre-distilled base method entries
- `enrichment/resources/survey/class_survey_data.json` -- ScriptWebView entry
- `enrichment/resources/survey/class_survey.md` -- enrichment prerequisites

## Source Files

- **Header:** `hi_scripting/scripting/api/ScriptingApiContent.h` (lines 2160-2252)
- **Implementation:** `hi_scripting/scripting/api/ScriptingApiContent.cpp` (lines 5823-6071)
- **WebViewData (core data model):** `hi_tools/hi_standalone_components/ChocWebView.h` (lines 40-386)
- **WebViewWrapper (JUCE component):** `hi_tools/hi_standalone_components/ChocWebView.h` (lines 390-441)
- **Script component wrapper:** `hi_scripting/scripting/api/ScriptComponentWrappers.h` (lines 977-998)
- **Script component wrapper impl:** `hi_scripting/scripting/api/ScriptComponentWrappers.cpp` (lines 2908-2938)

---

## Class Declaration

```cpp
struct ScriptWebView : public ScriptComponent
```

Direct inheritance from `ScriptComponent`. No additional interfaces or mixins.

**Static identifier:** `"ScriptWebView"` (via `getStaticObjectName()`)

---

## Properties (enum)

```cpp
enum Properties
{
    enableCache = ScriptComponent::numProperties,
    enablePersistence,
    scaleFactorToZoom,
    enableDebugMode
};
```

Four WebView-specific properties, all Toggle type:

| Property | Default | Description |
|----------|---------|-------------|
| `enableCache` | `false` | Enables caching of web files. Set to false during development for live reload via browser refresh. |
| `enablePersistence` | `true` | When true, new webview instances are initialized with all prior function calls (persistent call replay). |
| `scaleFactorToZoom` | `true` | Applies the host/system scale factor as the browser zoom level. |
| `enableDebugMode` | `false` | Enables browser debug mode (developer tools). |

---

## Constructor Analysis

```cpp
ScriptWebView(ProcessorWithScriptingContent* base, Content* parentContent,
              Identifier webViewName, int x, int y, int width, int height)
```

### WebViewData acquisition

```cpp
data = mc->getOrCreateWebView(webViewName);
```

The `WebViewData` is obtained from `MainController` via `GlobalScriptCompileBroadcaster::getOrCreateWebView()`. This is a **singleton-per-name** pattern: all `ScriptWebView` instances with the same name share the same `WebViewData`. The data persists across recompilations because it lives on `GlobalScriptCompileBroadcaster`, not on the script component.

### Error logger setup

```cpp
data->setErrorLogger([mc](const String& error)
{
    mc->getConsoleHandler().writeToConsole(error, 1, mc->getMainSynthChain(), juce::Colours::orange);
});
```

Resource-not-found and other WebViewData errors are routed to the HISE console with orange color.

### Property registration

Four `ADD_SCRIPT_PROPERTY` calls (all `ToggleSelector`):
- `enableCache`
- `enablePersistence`
- `scaleFactorToZoom`
- `enableDebugMode`

### Default values

```cpp
setDefaultValue(ScriptComponent::Properties::width, 200);
setDefaultValue(ScriptComponent::Properties::height, 100);
setDefaultValue(ScriptComponent::Properties::saveInPreset, false);

setDefaultValue(Properties::enableCache, false);
setDefaultValue(Properties::enablePersistence, true);
setDefaultValue(Properties::scaleFactorToZoom, true);
setDefaultValue(Properties::enableDebugMode, false);
```

Note: `saveInPreset` defaults to `false` (unlike most components which default to `true`).

### Deactivated properties

Extensive list of base properties are deactivated:

```cpp
deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::saveInPreset));
deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::macroControl));
deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::isPluginParameter));
deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::min));
deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::max));
deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::defaultValue));
deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::pluginParameterName));
deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::text));
deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::tooltip));
deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::useUndoManager));
deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::processorId));
deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::parameterId));
deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::isMetaParameter));
deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::linkedTo));
deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::automationId));
```

This means ScriptWebView cannot be connected to processor parameters, macros, plugin parameters, undo, or value linking. It is purely a display/interaction component, not a parameter control.

### API method registration

All use `ADD_API_METHOD_N` (untyped):

```
ADD_API_METHOD_1(setIndexFile);
ADD_API_METHOD_2(bindCallback);
ADD_API_METHOD_2(callFunction);
ADD_API_METHOD_2(evaluate);
ADD_API_METHOD_0(reset);
ADD_API_METHOD_1(setHtmlContent);
ADD_API_METHOD_1(setEnableWebSocket);
ADD_API_METHOD_2(sendToWebSocket);
ADD_API_METHOD_2(addBufferToWebSocket);
ADD_API_METHOD_1(updateBuffer);
ADD_API_METHOD_1(setWebSocketCallback);
```

**No `ADD_TYPED_API_METHOD_N` registrations.** All 11 child-class methods are untyped.

### No addConstant() calls

No constants are registered in the constructor.

---

## Wrapper struct

```cpp
struct Wrapper
{
    API_VOID_METHOD_WRAPPER_2(ScriptWebView, bindCallback);
    API_VOID_METHOD_WRAPPER_2(ScriptWebView, callFunction);
    API_VOID_METHOD_WRAPPER_2(ScriptWebView, evaluate);
    API_VOID_METHOD_WRAPPER_0(ScriptWebView, reset);
    API_VOID_METHOD_WRAPPER_1(ScriptWebView, setIndexFile);
    API_VOID_METHOD_WRAPPER_1(ScriptWebView, setHtmlContent);
    API_VOID_METHOD_WRAPPER_1(ScriptWebView, setEnableWebSocket);
    API_VOID_METHOD_WRAPPER_2(ScriptWebView, sendToWebSocket);
    API_VOID_METHOD_WRAPPER_2(ScriptWebView, addBufferToWebSocket);
    API_VOID_METHOD_WRAPPER_1(ScriptWebView, setWebSocketCallback);
    API_VOID_METHOD_WRAPPER_1(ScriptWebView, updateBuffer);
};
```

All methods return void.

---

## Factory / obtainedVia

Created via `Content.addWebView(name, x, y)`:

```cpp
ScriptWebView* addWebView(Identifier webviewName, int x, int y);
// implementation:
return addComponent<ScriptWebView>(webviewName, x, y);
```

---

## WebViewData -- Core Data Model

`WebViewData` is the central data object that holds all web content state. It lives in `hi_tools/hi_standalone_components/ChocWebView.h`.

### Inheritance

```cpp
struct WebViewData : public ReferenceCountedObject
```

Uses `ReferenceCountedObjectPtr<WebViewData>` (aliased as `WebViewData::Ptr`).

### ServerType enum

```cpp
enum class ServerType
{
    Uninitialised,
    FileBased,      // Uses a root directory to load content from files
    Embedded,       // Uses cached data to load content from embedded data
    Hardcoded,
    numServerTypes
};
```

Determines how external resources (CSS, JS, images) are resolved. `FileBased` is for development (reads files from disk); `Embedded` is for exported plugins (resources are baked into a ValueTree).

### Key members

- `CallbackType = std::function<var(const var&)>` -- callback signature for JS-to-C++ calls
- `StringPairArray initScripts` -- persistent evaluated scripts (keyed by identifier)
- `bool usePersistentCalls = true` -- replay calls on new webview instances
- `bool enableCache = true` -- cache file resources
- `bool applyScaleFactorAsZoom = true` -- scale factor behavior
- `bool debugModeEnabled = false` -- debug tools
- `File rootDirectory` -- root for file-based resource resolution
- `std::string rootFile = "/"` -- index file path (relative to root)
- `OwnedArray<ExternalResource> embeddedResources` -- embedded resource cache

### TCP Server (WebSocket infrastructure)

WebSocket communication is implemented via a TCP server (`TCPServer` inner class):

```cpp
ScopedPointer<TCPServer> currentTcpServer;
```

The TCP server has:
- `ConnectionThread` -- accepts incoming socket connections
- `CommunicationThread` -- sends queued data to connected clients
- `Data` -- serialized message (binary or string) with identifier
- `BufferSlot` -- maps a buffer index to a `VariantBuffer::Ptr` for efficient binary data streaming

The websocket is NOT a true WebSocket protocol -- it uses raw TCP with a custom framing protocol (identifier prefix + type byte + payload).

### Resource Provider

```cpp
struct ExternalResourceProviderBase
{
    virtual Image getImage(const String& hiseReference) = 0;
    virtual std::pair<String, String> getMimeContent(const String& hiseReference) = 0;
};
```

WebViewData supports pluggable resource providers. HISE adds `HiseWebResourceProvider` (in `GlobalScriptCompileBroadcaster.cpp`) which resolves `{PROJECT_FOLDER}` image references through the HISE image pool and expansion handler.

### Persistence / Export

- `exportAsValueTree()` -- exports all cached resources as a ValueTree for embedding in exported plugins
- `restoreFromValueTree()` -- restores from embedded ValueTree (sets mode to `Embedded`)
- `shouldBeEmbedded()` -- returns true if root directory is a child of project root and cache is enabled
- `explode()` -- converts embedded mode back to file-based (development)

---

## preRecompileCallback

```cpp
void ScriptWebView::preRecompileCallback()
{
    if(data->hasWebViews())
    {
        debugToConsole(..., "Clearing webviews...");
        SafeAsyncCall::call<WebViewData>(*data, [](WebViewData& d)
        {
            d.unloadRegisteredWebViews();
        });
        // Block until all webviews are unloaded
        while(!t->threadShouldExit() && data->hasWebViews())
        {
            t->sleep(10);
        }
        debugToConsole(..., "Done");
    }
}
```

Before recompilation, the component asynchronously unloads all registered webview instances and waits synchronously for completion. This ensures no stale webview references exist during script recompilation.

---

## setScriptObjectPropertyWithChangeMessage

Overrides the base to forward property changes to `WebViewData`:

```cpp
if (id == getIdFor(Properties::enableCache))
    data->setEnableCache((bool)newValue);
else if (id == getIdFor(Properties::enablePersistence))
    data->setUsePersistentCalls((bool)newValue);
else if (id == getIdFor(Properties::scaleFactorToZoom))
    data->setUseScaleFactorForZoom((bool)newValue);
else if (id == getIdFor(Properties::enableDebugMode))
    data->setEnableDebugMode((bool)newValue);
```

All four WebView-specific properties are forwarded to the underlying `WebViewData`.

---

## Private Inner Class: HiseScriptCallback

```cpp
struct HiseScriptCallback
{
    HiseScriptCallback(ScriptWebView* wv, const String& callback, const var& function) :
        f(wv->getScriptProcessor(), nullptr, function, 1),
        callbackId(callback)
    {
        f.incRefCount();
        f.setHighPriority();
        f.setThisObject(wv);
    };

    var operator()(const var& args);

    const String& callbackId;
    WeakCallbackHolder f;
};
```

This wraps a HiseScript function as a `WebViewData::CallbackType`. Key details:
- Uses `WeakCallbackHolder` with 1 argument
- `setHighPriority()` -- ensures callback is executed promptly
- `setThisObject(wv)` -- binds `this` to the ScriptWebView instance
- The `operator()` calls `f.callSync(&copy, 1, &rv)` -- synchronous execution on whatever thread the JS callback fires on

### Callback storage

```cpp
OwnedArray<HiseScriptCallback> callbacks;
```

Multiple callbacks can be bound simultaneously, each with a unique `callbackId`.

---

## WebSocket callback (separate from bindCallback)

```cpp
WeakCallbackHolder webSocketCallback;
```

Initialized in constructor:
```cpp
webSocketCallback(base, this, var(), 1)
```

Set via `setWebSocketCallback()`:
```cpp
webSocketCallback = WeakCallbackHolder(getScriptProcessor(), this, callbackFunction, 1);
webSocketCallback.incRefCount();
auto ok = data->setWebSocketCallback([this](const var& v)
{
    webSocketCallback.call1(v);
    return var();
});
if(!ok)
    reportScriptError("You have to enable the WebSocket before calling this method");
```

The websocket callback is distinct from `bindCallback` callbacks -- it receives raw messages from the TCP socket, not JS function calls.

---

## Script Component Wrapper (UI layer)

```cpp
class WebViewWrapper : public ScriptCreatedComponentWrapper,
                       public GlobalSettingManager::ScaleFactorListener,
                       public ZoomableViewport::ZoomListener
```

The wrapper:
1. Creates a `hise::WebViewWrapper` (the JUCE component wrapping choc::ui::WebView)
2. Registers as a `ScaleFactorListener` on `GlobalSettingManager`
3. Registers as a `ZoomListener` on the parent `ZoomableViewport` (if any)
4. On `postInit()`, calls `wv->refresh()` to load/reload the web content
5. `scaleFactorChanged()` and `zoomChanged()` both call `refreshBounds()` on the underlying wrapper

`updateComponent()` is a no-op -- the webview manages its own rendering.

---

## Method Implementation Details

### bindCallback

```cpp
void bindCallback(const String& callbackId, const var& functionToCall)
{
    data->addCallback(callbackId, HiseScriptCallback(this, callbackId, functionToCall));
}
```

Creates a `HiseScriptCallback` wrapping the HiseScript function and registers it on `WebViewData`. The JS side calls this using the Promise pattern:
```javascript
callbackId(someArgs).then((result) => { doSomethingWith(result); });
```

### callFunction

```cpp
void callFunction(const String& javascriptFunction, const var& args)
{
    auto copy = data;
    MessageManager::callAsync([copy, javascriptFunction, args]()
    {
        copy->call(javascriptFunction, args);
    });
}
```

Dispatches to the message thread asynchronously. The function must be in the global JS scope.

### evaluate

```cpp
void evaluate(const String& uid, const String& jsCode)
{
    auto copy = data;
    MessageManager::callAsync([uid, copy, jsCode]()
    {
        copy->evaluate(uid, jsCode);
    });
}
```

Also dispatched asynchronously to the message thread. The `uid` parameter is used for persistent call tracking -- when `enablePersistence` is true, the code is stored in `initScripts` keyed by `uid` and re-evaluated when new webview instances are created.

### setHtmlContent

```cpp
void setHtmlContent(const String& htmlCode)
{
    data->setHtmlContent(htmlCode);
}
```

Direct passthrough. Sets inline HTML content (as opposed to file-based content).

### setIndexFile

```cpp
void setIndexFile(var file)
{
    if(auto sf = dynamic_cast<ScriptingObjects::ScriptFile*>(file.getObject()))
    {
        String s = "/" + sf->f.getFileName();
        data->setRootDirectory(sf->f.getParentDirectory());
        data->setIndexFile(s);
    }
    else
    {
        reportScriptError("setIndexFile must be called with a file object");
    }
}
```

Requires a `File` object (not a string path). Extracts the parent directory as the root directory and the filename as the index file. Reports a script error if not passed a File object.

### setEnableWebSocket

```cpp
void setEnableWebSocket(int port)
{
    data->setEnableWebsocket(port);
}
```

Direct passthrough. Creates the TCP server on the specified port. Pass -1 for a random port.

### sendToWebSocket

```cpp
void sendToWebSocket(String id, var nd)
{
    if(nd.isString())
        data->sendStringToWebsocket(id, nd.toString());
    else
    {
        if(nd.isBuffer())
        {
            auto ptr = nd.getBuffer()->buffer.getReadPointer(0);
            data->sendDataToWebsocket(id, ptr, nd.getBuffer()->size * sizeof(float));
        }
        else if (auto obj = nd.getDynamicObject())
        {
            data->sendStringToWebsocket(id, JSON::toString(nd, true));
        }
    }
}
```

Three dispatch paths based on data type:
- **String**: sent directly as string
- **Buffer**: sent as raw binary float data
- **Object (JSON)**: serialized to JSON string and sent as string

### addBufferToWebSocket

```cpp
void addBufferToWebSocket(int bufferIndex, var buffer)
{
    if(auto b = buffer.getBuffer())
        data->addBufferToWebsocket(bufferIndex, b);
}
```

Registers a buffer at a specific index for efficient streaming. Silently does nothing if the var is not a Buffer.

### updateBuffer

```cpp
void updateBuffer(int bufferIndex)
{
    data->updateBuffer(bufferIndex);
}
```

Marks the buffer at the given index as dirty so it will be sent on the next communication cycle.

### setWebSocketCallback

See websocket callback section above. Must call `setEnableWebSocket` first or reports a script error.

### reset

```cpp
void reset()
{
    data->reset(false);
}
```

Clears caches and persistent calls but preserves the file structure (`resetFileStructure=false`).

---

## Threading Model

- `callFunction` and `evaluate` are dispatched asynchronously to the message thread via `MessageManager::callAsync`
- `bindCallback` callbacks execute synchronously via `WeakCallbackHolder::callSync` -- the thread depends on the choc::WebView callback dispatch mechanism
- `sendToWebSocket` and related websocket methods interact with the TCP server which has its own `ConnectionThread` and `CommunicationThread`
- `setHtmlContent`, `setIndexFile`, `setEnableWebSocket` are direct calls with no thread dispatch
- `preRecompileCallback` blocks the compilation thread while waiting for webviews to unload

---

## Virtual Method Overrides

ScriptWebView does NOT override any of the virtual ScriptComponent methods (`getValue`, `setValue`, `changed`, `sendRepaintMessage`, etc.). All base class virtual methods behave identically to the default ScriptComponent implementation.

---

## No Preprocessor Guards

No `#if USE_BACKEND` or other conditional compilation guards in the ScriptWebView code itself. The underlying choc::WebView dependency has platform-specific code in ChocWebView.h (`JUCE_MAC`, `JUCE_WINDOWS`, `JUCE_LINUX`).
