Table::linkTo(ScriptObject otherTable) -> undefined

Thread safety: UNSAFE -- unregisters/re-registers event listeners and calls ExternalDataHolder::linkTo which may involve lock acquisition and data swapping.
Makes this Table handle refer to the same underlying data as another Table object.
After linking, both handles share the same graph points and lookup table --
modifications through either handle affect both. Callbacks registered on this handle
continue to work and fire based on the linked data's events.

Dispatch/mechanics:
  linkToInternal(otherTable) -> validates matching DataType
    -> unregister from current updater
    -> holder->linkTo(type, *otherHolder, otherIndex, thisIndex)
    -> re-resolve complexObject -> re-register event listener

Anti-patterns:
  - Linking is one-directional at the handle level: this handle starts pointing to the
    other's data, but the other handle is unaffected. Both then share the same underlying
    data, so modifications through either are visible to both.

Source:
  ScriptingApiObjects.cpp:1560  ScriptComplexDataReferenceBase::linkToInternal()
    -> validates ScriptComplexDataReferenceBase with matching type
    -> holder->linkTo(type, *otherHolder, otherIndex, thisIndex)
    -> re-registers event listener on new data object
