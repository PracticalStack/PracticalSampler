MidiList::isEmpty() -> bool

Thread safety: SAFE
Returns true if all 128 slots contain sentinel value -1 (numValues == 0). O(1) check against internal counter, not an array scan.
Pair with: clear -- resets list to empty state; getNumSetValues -- returns the actual count
Source:
  ScriptingApiObjects.h:288  isEmpty() -- inline: return numValues == 0
