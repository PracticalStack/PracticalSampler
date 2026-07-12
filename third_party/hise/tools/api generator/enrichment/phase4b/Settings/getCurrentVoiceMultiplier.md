Settings::getCurrentVoiceMultiplier() -> Integer

Thread safety: SAFE
Returns the current voice amount multiplier. Default is 2. Multiplied with
the base voice count to determine total available voices.

Pair with:
  setVoiceMultiplier -- change the multiplier

Source:
  ScriptingApi.cpp  Settings::getCurrentVoiceMultiplier()
    -> driver->voiceAmountMultiplier (direct member read)
