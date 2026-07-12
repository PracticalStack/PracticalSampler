Engine::renderAudio(var eventList, var finishCallback) -> undefined

Thread safety: UNSAFE -- creates AudioRenderer thread (heap allocation, thread spawn)
Renders MIDI events to audio buffers on a background thread. eventList must be array of
MessageHolder objects. Callback receives {channels, finished, progress} object.
Callback signature: finishCallback(Object status)
Anti-patterns:
  - Calling renderAudio again while rendering replaces the current thread -- previous
    render callback will not receive a final finished:true call
Pair with:
  createMessageHolder -- create MIDI events for input
  playBuffer -- preview rendered output
Source:
  ScriptingApi.cpp  Engine::renderAudio()
    -> new AudioRenderer(eventList) -> builds HiseEventBuffer list -> thread.startThread()
