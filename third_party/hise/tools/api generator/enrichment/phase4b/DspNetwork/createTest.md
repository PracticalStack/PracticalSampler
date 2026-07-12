DspNetwork::createTest(JSON testData) -> ScriptObject

Thread safety: UNSAFE -- allocates a ScriptNetworkTest object. Backend-only (USE_BACKEND).
Creates a ScriptNetworkTest object for automated testing of this network. The testData
JSON is augmented with the network's ID (as NodeId) before creating the test. Returns
undefined silently in frontend (exported plugin) builds.
Required setup:
  const var nw = Engine.createDspNetwork("MyNetwork");
Anti-patterns:
  - Do NOT call in exported plugins expecting a result -- returns undefined silently
    with no error. Any subsequent method calls on the result will fail.
Source:
  DspNetwork.cpp  createTest()
    -> testData augmented with NodeId = network ID
    -> creates ScriptNetworkTest (backend only, TestClasses.h)
    -> frontend: returns var() (no-op)
