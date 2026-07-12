Message::delayEvent(Number samplesToDelay) -> undefined

Thread safety: SAFE
Adds a sample offset to the current event's timestamp, delaying when it is processed
by downstream modules. If the resulting timestamp exceeds the current buffer size, the
event is automatically deferred to a future buffer. Additive relative to existing timestamp.

Dispatch/mechanics:
  messageHolder->addToTimeStamp(samplesToDelay)
    -> getTimeStamp() + delta, clamped to >= 0 via jmax
    -> events beyond buffer size split via moveEventsAbove()

Pair with:
  getTimestamp -- read the current sample offset before delaying
  setStartOffset -- controls where in the sample playback begins (different purpose)

Anti-patterns:
  - Do NOT use delayEvent to skip ahead in a sample -- it delays event processing, not
    playback position. Use setStartOffset() for sample-start offset.

Source:
  ScriptingApi.cpp  Message::delayEvent()
    -> HiseEvent::addToTimeStamp(delta)
    -> jmax<int>(0, getTimeStamp() + delta)
