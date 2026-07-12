DspNetwork::create(String path, String id) -> ScriptObject

Thread safety: UNSAFE -- creates a ValueTree and instantiates a node object via factory lookup. Involves heap allocations and string operations.
Creates a node with the given factory path and ID and returns a reference to it.
If a node with the specified id already exists, returns the existing node. If id is
empty, a unique name is auto-generated from the path suffix. Returns undefined if
no registered factory matches the path.
Required setup:
  const var nw = Engine.createDspNetwork("MyNetwork");
Pair with:
  createAndAdd -- creates and immediately parents to a container
  get -- retrieves an existing node by ID
Anti-patterns:
  - Do NOT rely on the path parameter when reusing an existing id -- if a node named
    "myOsc" already exists as core.oscillator, create("core.gain", "myOsc") returns the
    oscillator, not a gain node, with no warning.
  - Do NOT expect a script error on invalid factory path -- returns undefined silently
    (only a message window popup on the message thread).
Source:
  DspNetwork.cpp:814  create()
    -> checkValid() ensures parent holder alive
    -> get(id) checks for existing node, returns if found
    -> creates ValueTree {Node, ID=id, FactoryPath=path}
    -> createFromValueTree() iterates registered nodeFactories
