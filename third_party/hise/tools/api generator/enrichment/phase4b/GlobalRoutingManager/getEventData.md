GlobalRoutingManager::getEventData(Integer eventId, Integer dataSlot) -> Double

Thread safety: SAFE -- direct read from a fixed-size array. No allocations or locks.
Reads a value from per-event data storage. Returns the stored double if the event ID and
slot match. Returns undefined if the slot was never written or if a hash collision occurred.

Pair with:
  setEventData -- write values that this method reads

Source:
  ScriptingApiObjects.cpp:9150  getEventData()
    -> m->additionalEventStorage.getValue((uint16)eventId, (uint8)dataSlot)
    -> if match: returns var(double)
    -> if miss: returns var() (undefined)
  MiscToolClasses.h:2716  AdditionalEventStorage::getValue()
    -> reads data[eventId & 1023][slotIndex & 15]
    -> validates stored eventId matches requested eventId
