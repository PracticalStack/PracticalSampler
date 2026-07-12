ErrorHandler::getErrorMessage() -> String

Thread safety: WARNING -- String return type involves atomic ref-count operations.
Returns the error message for the current highest-priority active error. Returns
empty string if no errors are active.

Dispatch/mechanics:
  getCurrentErrorLevel() -> if custom message set via setCustomMessageToShow(), use it
    -> else if event-supplied message (CustomErrorMessage/CriticalCustomErrorMessage/
       CustomInformation states), use it
    -> else fall back to built-in default from getOverlayTextMessage()

Pair with:
  getCurrentErrorLevel -- get the state constant alongside the message
  setCustomMessageToShow -- override messages for specific states

Source:
  ScriptingApiObjects.cpp  ScriptErrorHandler::getErrorMessage()
    -> customErrorMessages[el] -> getOverlayTextMessage(state) fallback chain
