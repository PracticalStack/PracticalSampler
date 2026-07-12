Modulator::asTableProcessor() -> ScriptObject

Thread safety: UNSAFE -- allocates a new ScriptingTableProcessor wrapper on
the heap.
Converts this modulator to a TableProcessor handle if the underlying modulator
implements LookupTableProcessor (e.g., TableEnvelope, table-based modulators).
Returns undefined if not a table processor type. Does not report an error on
failure -- check the return value with isDefined().

Dispatch/mechanics:
  dynamic_cast<LookupTableProcessor*>(mod)
    -> success: creates new ScriptingTableProcessor wrapper
    -> failure: returns var() (undefined), no error

Source:
  ScriptingApiObjects.cpp:3330  asTableProcessor()
    -> casts to LookupTableProcessor* -> wraps in ScriptingTableProcessor
