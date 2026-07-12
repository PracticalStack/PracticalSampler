GlobalRoutingManager::setEventData(Integer eventId, Integer dataSlot, Double value) -> undefined

Thread safety: SAFE -- direct write to a fixed-size array with lock-free broadcaster
notification. No allocations or locks.
Stores a double value in per-event data storage, keyed by MIDI event ID and slot index.
Accessible downstream by EventDataModulator, scriptnode routing nodes, and
ComplexGroupManager. Uses bitmask hashing (1024 event slots, 16 data slots per event).

Pair with:
  getEventData -- read back stored values by event ID and slot

Anti-patterns:
  - Hash collisions can occur for event IDs sharing the same lower 10 bits (eventId & 1023)
    -- a collision silently overwrites the previous entry. Rare in practice since MIDI event
    IDs cycle sequentially.
  - Slot index is masked to 4 bits (0-15) -- values outside this range wrap silently

Source:
  ScriptingApiObjects.cpp:9140  setEventData()
    -> m->additionalEventStorage.setValue((uint16)eventId, (uint8)dataSlot, value, sendNotificationSync)
  MiscToolClasses.h:2716  AdditionalEventStorage
    -> data[eventId & 1023][slotIndex & 15] = {eventId, value}
    -> fires LambdaBroadcaster<uint16, uint8, double> synchronously
