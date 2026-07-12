UnorderedStack::insert(var value) -> Integer

Thread safety: SAFE
Inserts a value if not already present (set semantics). Returns true if added,
false if duplicate or stack is full (128 elements).

Dispatch/mechanics:
  Float mode: (float)value -> data.insert() which calls contains() first (O(n))
    -> appends at position, increments size -> updateElementBuffer()
  Event mode: dynamic_cast to ScriptingMessageHolder -> getMessageCopy()
    -> eventData.insert() with same duplicate check

Anti-patterns:
  - Silently returns false when the stack is full (128 elements) -- no error reported
  - In event mode, passing a non-MessageHolder value silently returns false
  - Do NOT insert into a float-mode stack with MessageHolder values --
    call setIsEventStack first

Source:
  ScriptingApiObjects.cpp  insert()
    -> float: hise::UnorderedStack::insert() -- contains() + append at position
    -> event: ScriptingMessageHolder::getMessageCopy() -> eventData.insert()
  CustomDataContainers.h:300  hise::UnorderedStack::insert()
    -> contains() check, clamp position to SIZE-1
