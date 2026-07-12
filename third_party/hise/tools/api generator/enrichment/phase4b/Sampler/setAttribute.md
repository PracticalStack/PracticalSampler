Sampler::setAttribute(Number index, Number newValue) -> undefined

Thread safety: SAFE
Sets a sampler module attribute (processor parameter) by index. Uses the standard
attribute notification for proper UI/state updates.
Pair with:
  getAttribute -- read a parameter value
  getAttributeId -- get parameter name from index
  getNumAttributes -- get total parameter count
Source:
  ScriptingApi.cpp  Sampler::setAttribute()
    -> Processor::setAttribute(index, newValue, sendAttributeNotification)
