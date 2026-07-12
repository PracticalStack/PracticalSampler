Builder::create(String type, String id, Integer rootBuildIndex, Integer chainIndex) -> Integer

Thread safety: INIT -- throws script error if called after onInit.
Creates a new module and adds it to the chain at chainIndex of the parent module
at rootBuildIndex. Returns the new module's build index. Idempotent: if a
processor with the given id already exists under the parent, returns the existing
index without creating a duplicate. Returns -1 on failure.

Required setup:
  const var b = Synth.createBuilder();
  b.clear();

Dispatch/mechanics:
  interfaceCreationAllowed() check -> createdModules[rootBuildIndex] resolve parent
    -> ProcessorHelpers::getFirstProcessorWithName (idempotent check)
    -> ScopedBadBabysitter -> raw::Builder::create(parent, type, chainIndex)
    -> Chain::getFactoryType() -> createProcessor() -> addInternal()

Pair with:
  flush -- must call after all create() operations to update UI
  setAttributes -- configure module parameters after creation
  get -- retrieve typed scripting wrapper by build index

Anti-patterns:
  - Do NOT call outside onInit -- throws "You can't use this method after the
    onInit callback!" script error.
  - Idempotent reuse is silent: if an ID collision occurs, the existing module
    is returned with no warning. Use unique IDs to avoid accidental reuse.

Source:
  ScriptingApiObjects.cpp  ScriptBuilder::create()
    -> interfaceCreationAllowed() guard
    -> MainController::ScopedBadBabysitter
    -> raw::Builder::create(parent, processorType, chainIndex)
      -> Chain->getFactoryType()->createProcessor() -> addInternal()
