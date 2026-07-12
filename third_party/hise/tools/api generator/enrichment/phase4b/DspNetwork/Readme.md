DspNetwork (object)
Obtain via:
  - UI workflow (package icon / XML file selection) -- default, no script needed
  - Engine.createDspNetwork(id) -- script reference on same processor
  - Engine.getDspNetworkReference(processorId, id) -- cross-processor access from
    main interface script, avoids complex cross-module communication
  - SlotFX.setEffect(networkName) -- dynamic network swapping (returns Effect, not
    DspNetwork)

Top-level scriptnode DSP graph container. Manages creation, lookup, lifecycle,
and audio processing of nodes organized in a hierarchical tree rooted at a
container.chain node. Integrates with the HISE parameter system for DAW automation.

Host modules (DspNetwork::Holder implementations):
  ScriptFX (Script FX), PolyScriptFX (Polyphonic Script FX),
  ScriptSynth (Scriptnode Synthesiser), ScriptTimeVariantModulator,
  ScriptEnvelopeModulator. Plain Script Processors cannot host a DspNetwork.

Complexity tiers:
  1. Hosting: Engine.createDspNetwork. Build the graph visually in the scriptnode
     editor, leave prepareToPlay/processBlock callbacks empty. The hosting
     processor handles audio routing automatically.
  2. Parameter forwarding control: + setForwardControlsToParameters. Choose whether
     UI controls drive network parameters directly (DAW automation) or route through
     script callbacks. Default (true) is correct for most cases.
  3. Programmatic graph construction: + create, createAndAdd, createFromJSON, get,
     clear. Build node trees from script for dynamically generated signal chains.
     Not used in typical production workflows.

Practical defaults:
  - Use the visual scriptnode editor to build graphs rather than programmatic
    node creation. All production plugins use the XML-based workflow.
  - Leave prepareToPlay and processBlock callbacks empty when hosting a network.
     The hosting processor handles audio routing automatically.
  - Keep setForwardControlsToParameters(true) (the default) for DAW automation.
    Only disable when you need custom script-side parameter processing.
  - Use GlobalCable nodes within the scriptnode graph to bridge data between
    HiseScript and the DSP network rather than calling DspNetwork methods from
    audio callbacks.

Common mistakes:
  - Calling processBlock before prepareToPlay -- silently produces no output,
    no error thrown.
  - Writing custom processBlock logic in a hosted network -- the hosting
     processor handles audio routing automatically; manual processBlock is only
     needed for standalone buffer processing outside a hosted context.
  - Creating nodes programmatically when the graph is static -- use the visual
    scriptnode editor and save as XML instead.
  - Using setForwardControlsToParameters(false) without a clear reason -- breaks
    DAW automation and requires manual parameter bridging through script callbacks.
  - Calling Engine.createDspNetwork() in a non-scriptnode processor -- requires
     a DspNetwork::Holder (see host modules list above).
  - Passing mismatched buffer sizes to processBlock -- all channel buffers must
    have the same sample count.

Example:
  // Create a DspNetwork (must be in a DspNetwork::Holder processor)
  const var nw = Engine.createDspNetwork("MyNetwork");

  // Create nodes programmatically
  const var gain = nw.create("core.gain", "myGain");
  const var osc = nw.create("core.oscillator", "myOsc");

  // Access a node by ID (bracket syntax)
  const var ref = nw["myGain"];

Methods (11):
  clear                       create
  createAndAdd                createFromJSON
  createTest                  get
  prepareToPlay               processBlock
  setForwardControlsToParameters
  setParameterDataFromJSON    undo
