Broadcaster::attachToSampleMap(var samplerIds, var eventTypes, var optionalMetadata) -> undefined

Thread safety: INIT -- runtime calls throw script error
Registers source for samplemap events. Broadcaster must have 3 args (eventType, samplerId,
data). SampleMapChanged: data = reference string. SamplesAddedOrRemoved: data = sound count.
SampleChanged: requires integer index into SampleIds (not string). Auto-enables queue mode.
Dispatch/mechanics:
  Registers with ModulatorSampler samplemap events.
  SampleMapChanged: data = samplemap reference string.
  SamplesAddedOrRemoved: data = sound count.
  SampleChanged: requires integer index into SampleIds (not string).
  Auto-enables queue mode for multiple samplers/events.
Pair with:
  addListener -- to handle samplemap events
Notes:
  For SampleChanged events, pass Sampler constants (e.g. Sampler.LoKey, Sampler.HiKey) as eventTypes, not string.
  SampleChanged data object: { sound: Sample, id: int (Sampler constant), value: var }.
  Replaces ScriptPanel.setLoadingCallback() as best practice for samplemap monitoring.
Anti-patterns:
  - SampleChanged cannot be specified by string -- requires integer index.
  - Module must be ModulatorSampler -- error message incorrectly says "routing matrix" (bug).
Source:
  ScriptBroadcaster.cpp  SamplemapListener constructor
