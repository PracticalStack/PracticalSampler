UnorderedStack::remove(var value) -> Integer

Thread safety: SAFE
Removes the first matching value. Returns true if found and removed, false
otherwise. Float mode uses exact float equality. Event mode uses the configured
compare function. Element order is NOT preserved.

Dispatch/mechanics:
  Float mode: data.remove((float)value) -- linear search + removeElement (swap-with-last)
  Event mode: getIndexForEvent(value) -> eventData.removeElement(index)

Anti-patterns:
  - Removal does not preserve element order -- the removed slot is filled by
    swapping in the last element

Source:
  ScriptingApiObjects.cpp  remove()
    -> float: hise::UnorderedStack::remove() -- indexOf() + removeElement()
    -> event: getIndexForEvent() + removeElement()
  CustomDataContainers.h  removeElement() -- swap with last, decrement position
