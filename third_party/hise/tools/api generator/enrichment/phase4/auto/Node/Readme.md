# Node

Node is the scripting handle for a single processing unit within a scriptnode DSP network. Every node in the graph - oscillators, filters, gain processors, containers, and modulation sources - is accessed through this interface.

![Scriptnode Class Hierarchy](topology_scriptnode-hierarchy.svg)

`Node` sits inside a `DspNetwork` and owns zero or more `Parameter` objects that control its DSP behaviour.

A Node object lets you:

1. Set parameters and properties to configure the node's behaviour
2. Create and manage connections between modulation outputs and target parameters
3. Move nodes between containers to build or rearrange signal chains
4. Control bypass state
5. Query and traverse child nodes within containers

```js
const var nw = Engine.createDspNetwork("my_network");
const var root = nw.get("my_network");
const var node = nw.create("core.oscillator", "osc");
node.setParent(root, -1);
Console.print(node.getNumParameters());
```

Each node has two kinds of configurable state:

| Kind | What it controls | Examples |
|------|-----------------|----------|
| **Parameters** | Real-time DSP values that can be modulated or connected | Volume, frequency, filter type |
| **Properties** | Static configuration that becomes a compile-time constant when the network is exported to C++ | Data slot index, crossfade curve, converter function |

Use `set()` for both kinds. Use `get()` to read properties only - for parameter access, use `getOrCreateParameter()` to obtain a [Parameter]($API.Parameter$) object.

Nodes come in two broad categories. Leaf nodes (e.g. `core.gain`, `core.oscillator`) are individual DSP processors with a fixed parameter set. Container nodes (e.g. `container.chain`, `container.split`) hold child nodes and support dynamic macro parameter creation.

## Common Mistakes

- **Creating new parameters on leaf nodes**
  **Wrong:** `leafNode.getOrCreateParameter("MyMacro")` on a `core.gain` node to create a new macro parameter
  **Right:** Use `getOrCreateParameter()` to create new parameters only on container nodes. For leaf nodes, call it with an existing parameter name (e.g. `"Gain"`) to get a reference.
  *Leaf nodes have fixed parameter sets defined by their factory type. Only containers support dynamic macro parameter creation. However, `getOrCreateParameter()` with an existing name works on all node types and is the only way to obtain a Parameter reference.*

- **Connecting bypass to a non-soft-bypass container**
  **Wrong:** `node.connectToBypass(sourceInfo)` where node is a regular `container.chain`
  **Right:** Use a `container.soft_bypass` node as the target for dynamic bypass connections
  *Dynamic bypass connections require a SoftBypassNode. Other node types produce an IllegalBypassConnection error.*

- **Expecting get() to read everything set() can write**
  **Wrong:** `node.set("NodeColour", 0xFF0000); var c = node.get("NodeColour");` - `c` is undefined
  **Right:** Use `get()` only for node-type-specific properties (Mode, Frequency). Use `isBypassed()` for bypass state.
  *`get()` only reads node-type-specific properties. Direct attributes like NodeColour, Comment, and Folded are writable via `set()` but not readable via `get()`.*
