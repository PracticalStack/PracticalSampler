Builder::getExisting(String processorId) -> Integer

Thread safety: UNSAFE -- traverses the processor tree with string comparisons.
  May grow the internal array (heap allocation).
Registers a pre-existing processor (not created by this Builder instance) into
the Builder's tracking array and returns its build index. If already tracked,
returns the existing index. Reports a script error if no processor with the
given ID is found.

Required setup:
  const var b = Synth.createBuilder();

Dispatch/mechanics:
  Check createdModules for existing match -> if not found:
    ProcessorHelpers::getFirstProcessorWithName(mainSynthChain, id)
    -> append to createdModules -> return new index

Pair with:
  get -- retrieve a typed wrapper after registering
  create -- mix existing modules with newly created ones

Source:
  ScriptingApiObjects.cpp  ScriptBuilder::getExisting()
    -> ProcessorHelpers::getFirstProcessorWithName() global search
    -> createdModules.add() -> return index
