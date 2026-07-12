ScriptPanel::setFileDropCallback(String callbackLevel, String wildcard, Function dropFunction) -> undefined

Thread safety: UNSAFE -- registers file drop listener
Registers a callback for file drag-and-drop events. The callback level controls
which events fire, the wildcard filters accepted file types (e.g. "*.wav;*.aif").
Callback signature: f(Object dropInfo)
Callback levels: "No Callbacks", "Drop Only", "Drop & Hover", "All Callbacks"
Pair with:
  startExternalFileDrag -- initiate outbound file drag from this panel
Source:
  ScriptingApiContent.cpp  ScriptPanel::setFileDropCallback()
