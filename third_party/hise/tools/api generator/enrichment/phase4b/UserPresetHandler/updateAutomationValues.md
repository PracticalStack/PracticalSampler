UserPresetHandler::updateAutomationValues(var data, var sendMessage, Integer useUndoManager) -> undefined

Thread safety: WARNING -- allocates DynamicObjects for IndexSorter, Array::sort, and undo path; dispatches through connections
Updates custom automation values from an array of value objects or syncs from
processor connections via integer index.
  Mode 1 (Integer data): refreshes all slots from processor connection at the
    given index. sendMessage is ignored.
  Mode 2 (Array data): each element {id, value} sets the corresponding slot.
    sendMessage: SyncNotification, AsyncNotification, or false (dontSendNotification).
Required setup:
  const var uph = Engine.createUserPresetHandler();
  // Custom automation must be set up first
Dispatch/mechanics:
  Mode 1: iterates slots -> updateFromConnectionValue(preferredProcessorIndex)
  Mode 2: sorts array by automation index -> CustomAutomationData::call per entry
  When useUndoManager=true: wraps in AutomationValueUndoAction
Anti-patterns:
  - Do NOT pass a single object -- must be wrapped in an array. Single objects
    throw a script error.
  - [BUG] Internal IndexSorter has a copy-paste bug: both i1 and i2 use
    first["id"]. Sort is effectively disabled; values apply in array order.
  - [BUG] Undo path does not capture old values for array inputs. Engine.undo()
    silently does nothing instead of restoring previous values.
Pair with:
  createObjectForAutomationValues -- capture snapshot for later restore
  setAutomationValue -- set individual slot values (simpler API)
Source:
  ScriptExpansion.cpp:554  updateAutomationValues()
    -> Mode 1: CustomAutomationData::updateFromConnectionValue()
    -> Mode 2: IndexSorter sort -> CustomAutomationData::call() per entry
