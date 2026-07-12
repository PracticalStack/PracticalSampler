MessageHolder::setControllerNumber(Number newControllerNumber) -> undefined

Thread safety: SAFE
Sets the controller number. Has type coercion for virtual CC numbers: 128 changes
the event type to PitchBend, 129 changes it to Aftertouch. For 0-127, writes
directly to the number byte. No event-type guard -- calling on a NoteOn event
overwrites the note number.

Dispatch/mechanics:
  128 -> e.setType(HiseEvent::Type::PitchBend)
  129 -> e.setType(HiseEvent::Type::Aftertouch)
  other -> e.setControllerNumber((uint8)newControllerNumber)

Pair with:
  getControllerNumber -- read controller number (returns 128/129 for PitchBend/Aftertouch)
  setControllerValue -- set the value after setting the number

Anti-patterns:
  - Setting controller number 128 or 129 changes the event TYPE, not just the number
    field. This is intentional but may surprise if you expect only the number to change.

Source:
  ScriptingApiObjects.cpp  ScriptingMessageHolder::setControllerNumber()
    -> checks HiseEvent::AfterTouchCCNumber (129) and PitchWheelCCNumber (128)
    -> type coercion or e.setControllerNumber()
