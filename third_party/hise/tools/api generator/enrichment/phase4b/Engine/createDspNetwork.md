Engine::createDspNetwork(String id) -> ScriptObject

Thread safety: UNSAFE -- heap allocation, XML parsing in backend, ValueTree setup
Creates or retrieves a scriptnode DSP network with the given ID. The script processor
must be a DspNetwork::Holder. If a network with the same ID already exists, it is
returned (no duplicate created).
Host modules: ScriptFX, PolyScriptFX, ScriptSynth, ScriptTimeVariantModulator,
  ScriptEnvelopeModulator. Plain Script Processors and Script Voice Start Modulators
  cannot host a DspNetwork.
Anti-patterns:
  - Do NOT call on a plain Script Processor or Script Voice Start Modulator -- only
    the five DspNetwork::Holder modules can create DSP networks. Produces script error
    otherwise.
Pair with:
  getDspNetworkReference -- access a network from another processor
Source:
  ScriptingApi.cpp  Engine::createDspNetwork()
    -> DspNetwork::Holder::getOrCreate(id)
    -> checks for existing .xml in DspNetworks folder (backend)
