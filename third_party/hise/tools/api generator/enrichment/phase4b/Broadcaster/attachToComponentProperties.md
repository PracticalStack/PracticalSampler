Broadcaster::attachToComponentProperties(var componentIds, var propertyIds, var optionalMetadata) -> undefined

Thread safety: INIT -- runtime calls throw script error
Registers source that fires when watched properties change on specified components.
Broadcaster must have 3 args (component, propertyId, value). Uses ValueTree property listeners.
Does NOT dispatch initial values to existing targets on attachment.
Dispatch/mechanics:
  Creates valuetree::PropertyListener on each component's property ValueTree.
  Does NOT dispatch initial values to existing targets on attachment.
  Targets added after source are initialized via initItem().
Pair with:
  addComponentPropertyListener -- target that sets properties on other components
Anti-patterns:
  - Does not dispatch initial values to existing targets -- add source before listeners or call resendLastMessage.
  - Invalid property IDs validated against all components -- partial validity fails entirely.
Source:
  ScriptBroadcaster.cpp  ComponentPropertyListener constructor
