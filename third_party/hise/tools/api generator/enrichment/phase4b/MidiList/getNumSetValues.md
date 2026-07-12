MidiList::getNumSetValues() -> int

Thread safety: SAFE
Returns the number of slots containing a value other than -1. O(1) read of internal counter, not an array scan.
Pair with: isEmpty -- checks if count is zero; getValueAmount -- counts occurrences of a specific value
Source:
  ScriptingApiObjects.h:290  getNumSetValues() -- inline: return numValues
