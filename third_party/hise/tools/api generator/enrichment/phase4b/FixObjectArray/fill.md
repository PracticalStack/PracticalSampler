FixObjectArray::fill(NotUndefined obj) -> undefined

Thread safety: SAFE
Fills every element with the given value. If obj is a FixObject, deep-copies it
into every slot (memcpy). If obj is anything else, resets all elements to defaults.

Dispatch/mechanics:
  FixObject arg: *item = *obj (memcpy) for each slot
  Non-FixObject arg: item->clear() for each slot (reset to prototype defaults)

Pair with:
  clear -- equivalent to fill(non-FixObject)
  copy -- extracts a property column out; fill broadcasts a template in

Anti-patterns:
  - Do NOT pass a plain JSON object expecting it to set property values -- triggers the
    non-FixObject branch, resetting all elements to defaults instead (no error reported)

Source:
  FixLayoutObjects.cpp:955  Array::fill()
    -> dynamic_cast<ObjectReference*> branch: memcpy per slot
    -> else branch: ObjectReference::clear() per slot
