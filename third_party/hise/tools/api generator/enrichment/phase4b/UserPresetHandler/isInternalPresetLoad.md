UserPresetHandler::isInternalPresetLoad() -> Integer

Thread safety: SAFE
Returns true if the current preset load originates from a DAW state restore or
initial state load, as opposed to user selection from the preset browser. The
flag is set by ScopedInternalPresetLoadSetter (RAII). Only meaningful during
pre/post callbacks.
Anti-patterns:
  - Do NOT call outside pre/post callbacks -- the flag retains its stale value
    from the most recent load, producing incorrect conditional logic.
Source:
  MainController.h:939  isInternalPresetLoad()
    -> returns isInternalPresetLoadFlag (simple getter)
