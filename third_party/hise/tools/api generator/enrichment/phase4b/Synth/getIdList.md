Synth::getIdList(String type) -> Array

Thread safety: INIT -- checks objectsCanBeCreated() (onInit only).
Returns an array of processor IDs for all processors of the given type name within the parent
synth's subtree. The type parameter is the processor class name (e.g., "LFO Modulator"),
NOT the user-assigned ID. The calling script processor is excluded from results.

Anti-patterns:
  - Do NOT pass a processor ID (e.g., "LFO1") instead of a type name (e.g., "LFO Modulator")
    -- returns an empty array with no error, making the mistake hard to diagnose.
  - When called outside onInit, silently returns undefined instead of reporting an error.

Source:
  ScriptingApi.cpp  Synth::getIdList()
    -> Processor::Iterator<Processor>(owner)
    -> matches p->getName() against type parameter
    -> excludes self (getProcessor())
    -> returns Array of matching processor IDs
