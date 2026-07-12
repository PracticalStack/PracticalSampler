Engine::getDeviceType() -> String

Thread safety: WARNING -- string construction, atomic ref-count operations
Returns device type: "Desktop", "iPad", "iPadAUv3", "iPhone", or "iPhoneAUv3".
On desktop platforms, always returns "Desktop".
Source:
  ScriptingApi.cpp  Engine::getDeviceType()
    -> HiseDeviceSimulator::getDeviceName()
