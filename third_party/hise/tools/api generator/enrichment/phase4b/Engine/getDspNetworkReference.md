Engine::getDspNetworkReference(String processorId, String id) -> ScriptObject

Thread safety: UNSAFE -- processor tree iteration, string comparisons
Returns a reference to a DSP network owned by another script processor.
Anti-patterns:
  - Do NOT assume a return value -- silently returns undefined if processorId does not
    match any DspNetwork::Holder. Only a wrong network id on a found processor errors.
Pair with:
  createDspNetwork -- create a network on the current processor
Source:
  ScriptingApi.cpp  Engine::getDspNetworkReference()
    -> iterates DspNetwork::Holder processors -> getOrCreate(id)
