Broadcaster::addModuleParameterSyncer(String moduleId, var parameterIndex, var metadata) -> Integer

Thread safety: UNSAFE -- looks up processor by name, allocates OwnedArray entry, forces synchronous execution
Adds a target that synchronizes a module parameter from the broadcaster's last argument value.
Casts args.getLast() to float, sanitizes NaN/Inf, calls setAttribute() on the target module.
Side effect: forces the entire broadcaster into synchronous execution mode.
Dispatch/mechanics:
  Forces setForceSynchronousExecution(true) on the broadcaster.
  callSync(): casts args.getLast() to float, sanitizes NaN/Inf,
  calls target->setAttribute(parameterIndex, v, sendNotificationAsync).
Pair with:
  attachToModuleParameter -- source that produces parameter change events
  setForceSynchronousExecution -- auto-enabled by this method
Anti-patterns:
  - Forces entire broadcaster into synchronous execution mode -- affects all sends.
  - Value is always args.getLast() -- position-dependent on broadcaster arg layout.
  - Deleted module (weak ref null) silently skips setAttribute.
Source:
  ScriptBroadcaster.cpp:920  ModuleParameterSyncer::callSync()
