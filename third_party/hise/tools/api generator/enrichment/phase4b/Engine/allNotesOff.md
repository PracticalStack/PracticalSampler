Engine::allNotesOff() -> undefined

Thread safety: SAFE -- sets an atomic boolean flag (allNotesOffFlag = true)
Sends an all-notes-off at the next audio buffer. Does not immediately stop voices --
the flag is checked at the start of the next processing block.
Source:
  ScriptingApi.cpp  Engine::allNotesOff()
    -> sets MainController::allNotesOffFlag = true (atomic)
