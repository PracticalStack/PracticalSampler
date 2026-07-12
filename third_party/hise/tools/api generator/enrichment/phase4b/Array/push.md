Array::push(var value1) -> Integer

Thread safety: WARNING -- triggers audio thread warning only when the push
would exceed allocated capacity. Use reserve() in onInit to pre-allocate.
Appends one or more values to the end of the array and returns the new length.
Supports variadic arguments.

Dispatch/mechanics:
  WARN_IF_AUDIO_THREAD(numArgs + size >= numAllocated, ScriptGuard::ArrayResizing)
  -> array->add(value) for each argument

Pair with:
  reserve -- pre-allocate capacity to avoid audio-thread warnings
  pop -- remove from end (stack pattern)
  pushIfNotAlreadyThere -- append only if not duplicate

Anti-patterns:
  - Do NOT push onto arrays in MIDI callbacks without calling reserve() in
    onInit first -- reallocation on the audio thread causes glitches.

Source:
  JavascriptEngineObjects.cpp  ArrayClass::push()
    -> WARN_IF_AUDIO_THREAD check on capacity
    -> array->add(get(a, i)) for each variadic argument
