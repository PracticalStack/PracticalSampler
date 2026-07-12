Threads::startProfiling(var options, Function finishCallback) -> undefined

Thread safety: UNSAFE -- Starts a recording session involving heap allocations and listener registration.
Starts a thread profiling session. Accepts either a plain number (milliseconds,
clamped to 10-10000) or a JSON options object. When recording completes,
finishCallback is called with a Base64-encoded string of profiling data.
Requires HISE_INCLUDE_PROFILING_TOOLKIT -- throws script error without it.
Callback signature: finishCallback(String base64Data)

Required setup:
  // Enable HISE_INCLUDE_PROFILING_TOOLKIT in project settings

Dispatch/mechanics:
  Stores finishCallback in WeakCallbackHolder
    -> If options is DynamicObject: Options::fromDynamicObject() -> DebugSession::startRecording()
    -> If options is Number: startRecording(ms, holder) directly
    -> On completion: recordingFlushBroadcaster fires -> toBase64() -> call1(b64)

Anti-patterns:
  - Do NOT call without HISE_INCLUDE_PROFILING_TOOLKIT defined -- throws script
    error rather than silently failing

Source:
  ScriptingApi.cpp  startProfiling()
    -> threadProfileCallback = WeakCallbackHolder(finishCallback)
    -> DebugSession::startRecording(ms, holder)
  ScriptingApi.cpp:7864  constructor registers recordingFlushBroadcaster listener
    -> converts ProfileInfoBase::Ptr to Base64 -> threadProfileCallback.call1(b64)
