Broadcaster::attachToComponentValue(var componentIds, var optionalMetadata) -> undefined

Thread safety: INIT -- runtime calls throw script error
Registers source that fires when component values change. Broadcaster must have 2 args
(component, value). Dispatches initial values on attachment. Uses attachValueListener()
which overwrites any previous broadcaster value listener on the same component.
Dispatch/mechanics:
  Attaches value listener via ScriptComponent::attachValueListener().
  Dispatches initial values on attachment via checkMetadataAndCallWithInitValues().
  Overwrites any previous broadcaster value listener on the same component.
Pair with:
  addComponentValueListener -- target that sets values on other components
  attachToComponentProperties -- for property changes instead of value changes
Notes:
  Listener callback signature matches setControlCallback(): function(component, value).
  Easy migration path from control callbacks to broadcaster system.
Anti-patterns:
  - Overwrites previous broadcaster value listener on the same component -- only last one receives updates.
  - Broadcaster must have exactly 2 args.
Source:
  ScriptBroadcaster.cpp  ComponentValueListener constructor
