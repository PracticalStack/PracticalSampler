FixObjectStack::insert(ScriptObject obj) -> Integer

Thread safety: WARNING -- O(n) linear scan for duplicate detection via indexOf.
Inserts a copy of obj at the end of the used portion if no duplicate exists.
Duplicates detected using the factory's compare function. Returns 1 on success,
0 if duplicate exists or obj is not a valid FixObject.

Dispatch/mechanics:
  indexOf(obj) for duplicate check (virtual size() = position)
  -> if not found: *items[position] = *ref (data copy into preallocated slot)
  -> position = jmin(position + 1, numElements - 1)

Pair with:
  remove -- to remove a previously inserted element
  set -- upsert alternative (replaces if exists, inserts if not)
  contains -- check before insert if you need the reason for rejection

Anti-patterns:
  - [BUG] Off-by-one capacity: position is clamped to numElements-1 after writing,
    so the last slot is written but never counted by size(). Effective capacity is
    length-1, not length. On a capacity-1 stack, insert() always writes an
    invisible element.
  - Do NOT create a new FixObject for each insert -- reuse a single temp object
    and mutate its properties before calling insert()

Source:
  FixLayoutObjects.cpp:1312  Stack::insert()
    -> indexOf(obj) for duplicate detection
    -> *items[position] = *ref (ObjectReference copy)
    -> position = jmin(position + 1, numElements - 1)
