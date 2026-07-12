ScriptTable::setConsumedKeyPresses(NotUndefined listOfKeys) -> undefined

Thread safety: UNSAFE
Defines which key presses this component consumes before key callback registration.

Pair with:
  setKeyPressCallback -- callback requires consumed-key configuration first

Source:
  HISE/hi_scripting/scripting/api/ScriptingApiContent.cpp:421  ScriptComponent API registration -> key-consumption setup
