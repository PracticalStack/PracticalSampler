SliderPackData::setNumSliders(var numSliders) -> undefined

Thread safety: UNSAFE -- allocates a new VariantBuffer when no preallocated length is set.
Sets the number of sliders. Existing values are preserved up to the new count. New
sliders are filled with the default value (1.0). With preallocation active, resizes
the view without allocating.
Dispatch/mechanics:
  Without preallocation: allocates new VariantBuffer, copies existing values,
    fills new slots with defaultValue (1.0f)
  With preallocation: adjusts referToData length (up to numPreallocated),
    preserves all existing values, no allocation
Pair with:
  setUsePreallocatedLength -- enable preallocation before frequent resizing
  getNumSliders -- read the count this method sets
Anti-patterns:
  - Do NOT pass values <= 0 -- silently ignored with no error
  - Do NOT resize frequently without preallocation -- each call allocates
    a new buffer
Source:
  SliderPack.cpp  SliderPackData::setNumSliders()
    -> checks numPreallocated: if set, adjusts view; else allocates new VariantBuffer
    -> fires ContentRedirected event
