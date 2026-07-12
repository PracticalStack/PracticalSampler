# Node -- Class Analysis

## Brief
Scriptnode DSP processing unit with parameters, connections, bypass, and container hierarchy.

## Purpose
Node represents a single processing unit within a scriptnode DSP network graph. Each node wraps a specific DSP algorithm (oscillator, filter, gain, etc.) identified by its factory path (e.g. `core.gain`, `container.chain`). Nodes can be connected to form signal chains, bypassed, configured with node-type-specific properties, and organized hierarchically within container nodes. Created exclusively through DspNetwork, Node provides the scripting interface for programmatic graph construction and runtime control of the scriptnode system.

## Details

### Node Types and Hierarchy

Every node in a scriptnode graph is a NodeBase subclass. There are two broad categories:

- **Leaf nodes** (WrapperNode subclasses): Individual DSP processors like `core.gain`, `core.oscillator`, `math.add`. These have fixed parameter lists defined by their factory type.
- **Container nodes** (SerialNode, ParallelNode): Signal routing containers like `container.chain`, `container.split`, `container.multi`. These can hold child nodes and support dynamic macro parameter creation via `getOrCreateParameter()`.

Container nodes implement both NodeBase and the NodeContainer mix-in, which provides child node management, macro parameter routing, and the `getChildNodes()` traversal.

### Property System

Each node has two property storage locations:

| Location | Access | Examples |
|----------|--------|----------|
| **Node properties** (Properties child tree) | `get()`/`set()` | Mode, Frequency, Q -- node-type-specific |
| **ValueTree properties** (direct on node) | `set()` only | Bypassed, NodeColour, Comment, Folded |

The `get()` method only reads node properties. The `set()` method writes to both locations. Node-type-specific property IDs are registered as constants on each Node instance, so they can be accessed as `node.Mode` rather than the string `"Mode"`. See `get()` and `set()` for access details and asymmetry caveats.

### Connection Model

Nodes participate in the DspNetwork connection system at two levels:

1. **Parameter connections** (`connectTo`): Route a container's macro parameter or a modulation source's output to a target parameter. The behavior depends on the source node type -- containers route via macro parameters, modulation source nodes create modulation target entries.
2. **Bypass connections** (`connectToBypass`): Route a parameter or modulation source to control a node's bypass state. The target node must be a `SoftBypassNode` (a container with soft-bypass capability); other node types raise an `IllegalBypassConnection` error.

See `connectTo()` and `connectToBypass()` for connection API details.

### Complex Data Binding

Nodes that consume external data (tables, slider packs, audio files) have a ComplexData child tree in their ValueTree. The `setComplexDataIndex()` method changes which data slot a node references. Valid data type strings:

| dataType | Description |
|----------|-------------|
| `"Table"` | Lookup table (512 floats) |
| `"SliderPack"` | Resizable float array |
| `"AudioFile"` | Multichannel audio file |
| `"FilterCoefficients"` | Filter display coefficients |
| `"DisplayBuffer"` | FIFO visualization buffer |

See `setComplexDataIndex()` for the binding API.

### Graph Manipulation

Nodes can be moved between containers at runtime. See `setParent()` for the full move API, including connection preservation and detach semantics.

## obtainedVia
`DspNetwork.create(factoryPath, id)` or `DspNetwork.get(id)` -- nodes are created and looked up through the parent network.

## minimalObjectToken
nd

## Constants
None. Node registers zero fixed constants via `addConstant()`. Instead, node-type-specific property IDs are dynamically registered as constants at construction time from the node's property tree.

## Dynamic Constants
| Name | Type | Description |
|------|------|-------------|
| *(varies by node type)* | String | Property IDs from the node's Properties tree (e.g. "Mode", "Frequency"). Each node instance registers its own property names as constants for convenient access in `get()`/`set()` calls. |

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| `node.getOrCreateParameter("Gain")` on a leaf node | Use `getOrCreateParameter()` only on container nodes | Only container nodes (container.chain, container.split, etc.) support dynamic parameter creation. Leaf nodes have fixed parameters and will report a script error. |
| `node.connectToBypass(sourceInfo)` on a regular container | Use a `container.soft_bypass` node as the target | Dynamic bypass connections require a SoftBypassNode. Other node types produce an `IllegalBypassConnection` error. |

## codeExample
```javascript
// Get a node from an existing network
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
```

## Alternatives
- `DspModule` -- legacy API for loading external DSP libraries with manual processBlock calls; Node is the modern scriptnode graph unit with automatic signal routing.
- `DspNetwork` -- the container that holds and manages all nodes; Node is a single processing element within it.

## Related Preprocessors
`HISE_INCLUDE_PROFILING_TOOLKIT`, `USE_BACKEND`

## Diagrams

### node-valuetree-structure
- **Brief:** Node ValueTree Layout
- **Type:** topology
- **Description:** A Node ValueTree contains: direct properties (ID, Name, FactoryPath, Bypassed, NodeColour, Comment, Folded), a Parameters child tree (each Parameter has ID, Value, range properties, and a Connections child tree), a Properties child tree (node-type-specific key-value pairs), an optional Nodes child tree (for containers only, holding child Node trees), and an optional ComplexData child tree (with Tables, SliderPacks, AudioFiles sub-trees for data-consuming nodes).

## Diagnostic Ideas
Reviewed: Yes
Count: 0
Rationale: Node methods operate on ValueTree state and delegate to virtual implementations. The only hard constraint (container-only parameter creation) already produces a runtime script error. No silent-failure preconditions are detectable at parse time.
