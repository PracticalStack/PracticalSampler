Array::pushIfNotAlreadyThere(var value1) -> Integer

Thread safety: WARNING -- triggers audio thread warning if push would exceed
capacity. Also performs a linear scan for duplicate checking.
Appends one or more values only if not already present. Returns new length.
Uses loose comparison for duplicate checking (1 == 1.0). Variadic.

Pair with:
  push -- append without duplicate check
  contains -- check existence without appending
  reserve -- pre-allocate for audio-thread safety

Anti-patterns:
  - Do NOT rely on type distinction for deduplication -- loose comparison
    means int 1 and double 1.0 are treated as duplicates.

Source:
  JavascriptEngineObjects.cpp  ArrayClass::pushIfNotAlreadyThere()
    -> WARN_IF_AUDIO_THREAD check on capacity
    -> array->addIfNotAlreadyThere(get(a, i)) for each argument
