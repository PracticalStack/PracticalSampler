Message::isProgramChange() -> Integer

Thread safety: SAFE
Returns whether the current event is a MIDI program change message. Program change events
route through onController. Use this to detect program changes before calling
getProgramChangeNumber().

Pair with:
  getProgramChangeNumber -- get the program number (0-127)

Source:
  ScriptingApi.cpp  Message::isProgramChange()
    -> constMessageHolder->isProgramChange()
    -> checks type == Type::ProgramChange
