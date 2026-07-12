UnorderedStack::removeIfEqual(var holder) -> undefined

Thread safety: SAFE -- method's own code is lock-free with bounded O(128)
iteration. Custom compare callbacks execute synchronously within the search loop.
Event-mode only. Finds the first matching event using the configured compare
function, removes it, and writes the removed event back into the holder. This
"pop matching" operation preserves event metadata that may differ between the
search key and the stored event (e.g., different timestamps or velocities when
matching by event ID only).

Dispatch/mechanics:
  getIndexForEvent(holder) -> finds matching index using compare function
  Copies actual stack event into holder via MessageHolder write-back
  eventData.removeElement(index) -- swap-with-last removal

Pair with:
  setIsEventStack -- must configure event mode and compare function first
  insert -- to add events that removeIfEqual later pops
  storeEvent -- alternative read-only access without removal

Anti-patterns:
  - Do NOT call on a float-mode stack -- reports a script error
  - The holder is modified in-place with the removed event's data. If no match
    is found, the holder is unchanged.

Source:
  ScriptingApiObjects.cpp  removeIfEqual()
    -> getIndexForEvent(holder) using configured compare
    -> writes eventData[index] back into ScriptingMessageHolder
    -> eventData.removeElement(index)
