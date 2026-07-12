Builder::setAttributes(Integer buildIndex, var attributeValues) -> undefined

Thread safety: UNSAFE -- calls setAttribute on the processor and sends async
  change notification via sendOtherChangeMessage.
Sets multiple attributes on the module at buildIndex in a single call. The
attributeValues parameter is a JSON object mapping attribute names to numeric
values. Individual attributes are set with dontSendNotification; a single batch
async notification is sent after all are applied.

Required setup:
  const var b = Synth.createBuilder();
  var idx = b.create(b.SoundGenerators.SineSynth, "MySine", 0, b.ChainIndexes.Direct);

Dispatch/mechanics:
  createdModules[buildIndex] -> build attributeIds from module parameters
    -> iterate JSON properties -> lookup by Identifier -> setAttribute(idx, float(v), dontSendNotification)
    -> sendOtherChangeMessage() once after all attributes

Pair with:
  create -- create the module first, then configure attributes
  flush -- finalize after all modifications

Anti-patterns:
  - Stops on first unrecognized attribute name: reports a script error and
    breaks the loop. Subsequent valid attributes in the object are skipped.
    Double-check all attribute names before calling.
  - All values are cast to float. Non-numeric values (strings, objects) silently
    become 0.0 with no warning.

Source:
  ScriptingApiObjects.cpp  ScriptBuilder::setAttributes()
    -> setAttribute(idx, float(v), dontSendNotification) per property
    -> sendOtherChangeMessage() batch notification
