Engine::getDeviceResolution() -> Array

Thread safety: UNSAFE -- allocates Array, may query OS display API
Returns [x, y, width, height] of the primary display resolution.
Source:
  ScriptingApi.cpp  Engine::getDeviceResolution()
    -> HiseDeviceSimulator::getDisplayResolution()
