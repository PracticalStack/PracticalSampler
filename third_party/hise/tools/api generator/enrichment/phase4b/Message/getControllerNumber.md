Message::getControllerNumber() -> Integer

Thread safety: SAFE
Returns the controller number of the current event. Works on Controller (0-127),
PitchBend (returns 128 = PITCH_BEND_CC), and Aftertouch (returns 129 = AFTERTOUC_CC).
All three event types trigger onController, enabling uniform handling via virtual CC numbers.

Anti-patterns:
  - Do NOT filter by range (cc < 128) without considering that pitch wheel and aftertouch
    return 128/129 respectively -- they will be silently excluded.

Source:
  ScriptingApi.cpp  Message::getControllerNumber()
    -> HiseEvent::getControllerNumber()
    -> if PitchBend: return 128; if Aftertouch: return 129; else: return number
