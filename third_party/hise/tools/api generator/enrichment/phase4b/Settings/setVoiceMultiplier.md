Settings::setVoiceMultiplier(Integer newVoiceAmount) -> undefined

Thread safety: SAFE
Sets the voice amount multiplier. Multiplied with the base voice count to determine
total available voices. Default is 2.

Anti-patterns:
  - [BUG] No validation -- negative or zero values are accepted silently without
    range checking

Pair with:
  getCurrentVoiceMultiplier -- read the current multiplier

Source:
  ScriptingApi.cpp  Settings::setVoiceMultiplier()
    -> driver->voiceAmountMultiplier = newVoiceAmount (direct member write)
