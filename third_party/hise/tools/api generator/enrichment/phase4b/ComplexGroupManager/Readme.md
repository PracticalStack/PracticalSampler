ComplexGroupManager (object)
Obtain via: Sampler.getComplexGroupManager()

Multi-dimensional bitmask-based sample group controller for advanced layer,
crossfade, and articulation management. Replaces the Sampler's simple
round-robin group system with independent layers that can simultaneously
filter by articulation, round-robin, crossfade, release triggers, legato
intervals, and choke groups.

Constants:
  LayerControl:
    IgnoreFlag = 255    Special value indicating a layer does not apply to a sample or should be bypassed in filtering

Common mistakes:
  - Calling setGroupVolume() on a non-Custom LogicType layer -- silently
    does nothing (internal CustomLayer cast fails without error).
  - Calling isNoteNumberMapped() without createNoteMap() first -- throws
    a script error.
  - Calling getCurrentPeak() without setEnableGainTracking() first --
    throws a script error.
  - Passing IgnoreFlag to setActiveGroup() -- rejected with a script error.
    Use a valid zero-based group index.

Example:
  // Get the ComplexGroupManager from a Sampler
  const var cgm = Sampler.getComplexGroupManager();

  // Query layer structure
  const var numGroups = cgm.getNumGroupsInLayer(0);

  // Set the active group for the first layer
  cgm.setActiveGroup(0, 0);

Methods (16):
  addGroupEventStartOffset        createNoteMap
  delayGroupEvent                 fadeInGroupEvent
  fadeOutGroupEvent               getCurrentPeak
  getLayerIndex                   getLayerProperty
  getNumGroupsInLayer             isNoteNumberMapped
  registerGroupStartCallback      setActiveGroup
  setEnableGainTracking           setFixedGroupEventLength
  setGroupVolume                  setLayerProperty
