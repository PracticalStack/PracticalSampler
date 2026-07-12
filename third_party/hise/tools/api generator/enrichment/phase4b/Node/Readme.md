Node (object)
Obtain via: DspNetwork.create(factoryPath, id) or DspNetwork.get(id)

Scriptnode DSP processing unit within a network graph. Each node wraps a specific
DSP algorithm (oscillator, filter, gain, etc.) identified by its factory path.
Nodes are organized hierarchically -- leaf nodes process audio, container nodes
route signals and manage macro parameters.

Common mistakes:
  - Calling getOrCreateParameter() on a leaf node -- only container nodes support
    dynamic parameter creation. Leaf nodes report a script error.
  - Using connectToBypass() on a non-SoftBypassNode target -- dynamic bypass
    connections require container.soft_bypass. Other types raise IllegalBypassConnection.
  - Reading ValueTree properties via get() -- get() only reads node properties
    (Mode, Frequency). Use isBypassed() for bypass state; set() can write both
    locations but get() cannot read both.

Example:
  const var nd = nw.get("myGain");

  // Read and write node properties
  nd.set("Mode", 1);
  var mode = nd.get("Mode");

  // Access parameters
  var p = nd.getOrCreateParameter("Volume");

  // Bypass control
  nd.setBypassed(true);
  Console.print(nd.isBypassed()); // true

  // Get child nodes (containers only)
  var children = nd.getChildNodes(false);

Methods (12):
  connectTo            connectToBypass      get
  getChildNodes        getNumParameters     getOrCreateParameter
  isBypassed           reset                set
  setBypassed          setComplexDataIndex  setParent
