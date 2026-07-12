SliderPackData::setUsePreallocatedLength(int length) -> undefined

Thread safety: UNSAFE -- allocates a HeapBlock for the preallocated buffer.
Configures a fixed-size preallocated memory block. Once set, subsequent setNumSliders()
calls up to the limit adjust the view without allocating, preserving all values.
Pass 0 to disable and return to normal allocation.
Required setup:
  const var spd = Engine.createAndRegisterSliderPackData(0);
  spd.setUsePreallocatedLength(32); // call before setNumSliders
Dispatch/mechanics:
  length > 0: allocates HeapBlock<float>(length), copies current data,
    makes VariantBuffer reference this memory
  length == 0: creates new owned VariantBuffer, copies data back, frees HeapBlock
Pair with:
  setNumSliders -- resizes within preallocated limit without allocation
Source:
  SliderPack.cpp  SliderPackData::setUsePreallocatedLength()
    -> HeapBlock allocation -> VariantBuffer::referToData()
