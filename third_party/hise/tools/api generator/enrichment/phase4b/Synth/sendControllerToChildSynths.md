Synth::sendControllerToChildSynths(Integer controllerNumber, Integer controllerValue) -> undefined

Thread safety: SAFE -- delegates directly to sendController.
Identical to sendController -- exists only for backwards compatibility. Despite the name
suggesting it sends to child synths specifically, it behaves exactly the same as sendController.

Source:
  ScriptingApi.cpp  Synth::sendControllerToChildSynths()
    -> sendController(controllerNumber, controllerValue)
