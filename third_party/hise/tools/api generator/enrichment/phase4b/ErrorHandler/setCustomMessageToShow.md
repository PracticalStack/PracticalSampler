ErrorHandler::setCustomMessageToShow(int state, String messageToShow) -> undefined

Thread safety: UNSAFE -- String copy into internal StringArray involves heap allocation.
Overrides the default error message for a specific state. The custom message takes
highest priority when getErrorMessage() resolves text, above both event-supplied
messages and built-in defaults. Works for all states, not only custom message states.

Required setup:
  const var eh = Engine.createErrorHandler();

Anti-patterns:
  - Custom messages persist across error cycles. If a state is cleared and later
    re-triggered, the custom message still applies. Pass an empty string to revert
    to the default message.

Source:
  ScriptingApiObjects.cpp  ScriptErrorHandler::setCustomMessageToShow()
    -> customErrorMessages.set(state, messageToShow)
