Broadcaster::attachToComponentVisibility(var componentIds, var optionalMetadata) -> undefined

Thread safety: INIT -- runtime calls throw script error
Registers source that fires when component visibility changes. Broadcaster must have 2 args
(id, isVisible). Visibility check is recursive -- walks parent hierarchy, ANDing visible
properties. Hiding a parent triggers events for all watched children.
Dispatch/mechanics:
  Creates valuetree::RecursivePropertyListener on component tree root.
  Recursive check: ANDs visible property up the parent hierarchy.
  Sends asynchronously via sendAsyncMessage().
Pair with:
  attachToComponentValue -- for value changes
  attachToComponentProperties -- for property changes
Anti-patterns:
  - First arg is string ID (not component reference) -- differs from attachToComponentValue.
  - Parent visibility toggle generates N broadcasts (one per watched child).
  - Sent asynchronously.
Source:
  ScriptBroadcaster.cpp  ComponentVisibilityListener constructor
