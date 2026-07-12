Array (object)
Obtain via: var a = []; or var a = [1, 2, 3];

Dynamic mixed-type container with JavaScript-style iteration, search, sort,
and mutation methods. Holds any combination of numbers, strings, objects, and
nested arrays. Some methods differ from standard JavaScript -- notably concat
modifies in-place and default sort works numerically, not lexicographically.

Complexity tiers:
  1. Basic collection: push, pop, contains, indexOf, clear, join, length.
     Simple data storage and lookup. Every plugin uses arrays at this level.
  2. Data processing: + sort, sortNatural, filter, find, map, clone, concat.
     Transforming and querying structured data for preset management, sound
     browsers, and MIDI processing.
  3. Audio-thread patterns: + reserve with push/pop/removeElement using
     pre-allocated capacity. Required for any array modified inside onNoteOn,
     onNoteOff, or onController callbacks.

Practical defaults:
  - Use reserve(128) for arrays modified on the audio thread -- 128 matches
    MIDI note count and covers most use cases.
  - Use sortNatural() for file/sample names. Use sort() only for purely
    numeric arrays.
  - Use for (x in array) instead of forEach() on the audio thread -- the
    for-in loop does not allocate scope objects.
  - Use clone() when creating multiple objects from a template -- assignment
    only copies the reference.
  - Use pushIfNotAlreadyThere() when collecting items from parsed data where
    duplicates are expected.

Common mistakes:
  - Modifying arrays in MIDI callbacks without reserve() -- each push that
    exceeds capacity triggers a reallocation warning and potential audio glitch.
  - Using forEach() on the audio thread -- allocates scope objects internally.
    Use for (x in array) instead.
  - a.sort() on string array -- strings all compare as equal and remain
    unsorted. Use sortNatural() or a custom comparator.
  - var b = a.concat([4,5]) -- concat modifies in-place and returns undefined,
    not a new array. Unlike JavaScript.
  - removeElement(i) in a forward loop without adjusting index --
    removeElement(i--) or iterate backward to avoid skipping elements.
  - var b = a (reference copy) when independent copy needed -- use clone()
    for a deep copy.

Example:
  // Create and populate an array
  const a = [3, 1, 4, 1, 5, 9];

  // Functional iteration
  const doubled = a.map(function(x){ return x * 2; });
  const big = a.filter(function(x){ return x > 3; });

  // Search
  Console.print(a.indexOf(4));    // 2
  Console.print(a.contains(9));   // true

  // Sort numerically
  a.sort();
  Console.print(a.join(", "));    // "1, 1, 3, 4, 5, 9"

Methods (29):
  clear               clone               concat
  contains            every               filter
  find                findIndex           forEach
  includes            indexOf             insert
  isArray             isEmpty             join
  lastIndexOf         map                 pop
  push                pushIfNotAlreadyThere  remove
  removeElement       reserve             reverse
  shift               slice               some
  sort                sortNatural
