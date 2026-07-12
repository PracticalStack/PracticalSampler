Broadcaster::attachToComplexData(String dataTypeAndEvent, var moduleIds, var dataIndexes, var optionalMetadata) -> undefined

Thread safety: INIT -- runtime calls throw script error
Registers source that fires on complex data changes. Broadcaster must have 3 args (processorId,
index, value). Format: "DataType.EventType" (e.g. "Table.Content", "AudioFile.Display").
Total attached count = NumberOfModules * NumberOfIndexes.
Content events: value is base64-encoded data string (table points, slider values, audio range).
Display events: value is normalised double (0...1) -- ruler position, playback position, last active slider.
Dispatch/mechanics:
  Parses 'DataType.EventType' string. Creates ExternalDataHolder listener.
  Content events: fires with base64-encoded data string.
  Display events: fires with numeric display value.
  Auto-enables queue mode when multiple processors or indices.
Pair with:
  addListener -- to handle the (processorId, index, value) events
Anti-patterns:
  - Broadcaster must have exactly 3 args -- wrong count throws error.
  - dataTypeAndEvent must contain a dot separator -- empty parts throw error.
  - Common mistake: "Audio" instead of "AudioFile".
Source:
  ScriptBroadcaster.cpp:4288  ComplexDataListener constructor
