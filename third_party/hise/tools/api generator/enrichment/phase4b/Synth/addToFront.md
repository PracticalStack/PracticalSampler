Synth::addToFront(Integer addToFront) -> undefined

Thread safety: SAFE -- sets a boolean flag; in frontend builds unsuspends the content update dispatcher.
Designates this script processor's interface as the main plugin UI. When true, the Content
area becomes the visible front-end interface in the compiled plugin.

Anti-patterns:
  - Do NOT call addToFront(true) on multiple script processors -- only one should be the
    front interface. If multiple processors call it, getFirstInterfaceScriptProcessor
    returns the first one found during iteration (undefined order).

Source:
  ScriptingApi.cpp  Synth::addToFront()
    -> dynamic_cast<JavascriptMidiProcessor*> -> addToFront(value)
    -> FRONTEND_ONLY: unsuspends content update dispatcher
