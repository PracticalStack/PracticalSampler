Modulator::getGlobalModulatorId() -> String

Thread safety: UNSAFE -- constructs a new String via concatenation
("ContainerName:ModulatorName").
Returns the identifier of the connected global modulator in the format
"ContainerName:ModulatorName". Only works on modulator types whose type name
starts with "Global". Returns empty string for non-global modulators.

Dispatch/mechanics:
  checks mod->getType().toString().startsWith("Global")
    -> casts to GlobalModulator*
    -> gm->getItemEntryFor(connectedContainer, originalModulator)

Anti-patterns:
  - [BUG] Returns empty string silently for non-global modulators -- no error or
    indication that the call was inappropriate.

Source:
  ScriptingApiObjects.cpp:3008  getGlobalModulatorId()
    -> GlobalModulator::getItemEntryFor() returns "ContainerName:ModulatorName"
