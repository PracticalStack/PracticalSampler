# Connection -- C++ Source Exploration

## Resources Consulted

- `enrichment/resources/survey/class_survey_data.json` -- Connection entry
- `enrichment/resources/survey/class_survey.md` -- prerequisite: DspNetwork -> Node, Parameter, Connection
- `enrichment/phase1/Node/Readme.md` -- Node class distillation (connection model, property system)
- `enrichment/phase1/Parameter/Readme.md` -- Parameter class distillation (connection system, addConnectionFrom)

## Class Declaration

**File:** `HISE/hi_scripting/scripting/scriptnode/api/NodeBase.h:713`

```cpp
class ConnectionBase final: public ConstScriptingObject
{
public:

    enum ConnectionSource
    {
        MacroParameter,           // = 0
        SingleOutputModulation,   // = 1
        MultiOutputModulation,    // = 2
        numConnectionSources
    };

    ConnectionBase(DspNetwork* n, ValueTree data_);;
    ~ConnectionBase() {};

    Identifier getObjectName() const override { return PropertyIds::Connection; };

    // API methods
    var getTarget() const;
    var getSourceNode(bool getSignalSource) const;
    void disconnect();
    int getConnectionType() const;
    int getUpdateRate() const;
    bool isConnected() const;

    // Object validity (ConstScriptingObject overrides)
    bool objectDeleted() const override { return !data.getParent().isValid(); }
    bool objectExists() const override { return data.getParent().isValid(); }

    // Static factory for DSP parameter chain creation
    static parameter::dynamic_base::Ptr createParameterFromConnectionTree(
        NodeBase* n, const ValueTree& connectionTree, bool scaleInput);

    struct Helpers
    {
        static NodeBase* findRealSource(NodeBase* source);
        static ValueTree findCommonParent(ValueTree v1, ValueTree v2);
    };

private:
    WeakReference<DspNetwork> network;
    WeakReference<NodeBase> sourceNode;
    WeakReference<NodeBase> sourceInSignalChain;
    WeakReference<NodeBase> commonContainer;
    ConnectionSource type;
    ValueTree data;
    WeakReference<NodeBase::Parameter> targetParameter;

    struct Wrapper;
};
```

**Key observations:**
- `final` class -- no subclassing
- All private members are `WeakReference` except `data` (ValueTree, ref-counted) and `type` (enum)
- Object lifetime is tied to the ValueTree parent validity (connection removal invalidates the object)

## ConnectionSource Enum

The enum defines three connection source types:

| Value | Name | Meaning |
|-------|------|---------|
| 0 | `MacroParameter` | Connection from a container macro parameter |
| 1 | `SingleOutputModulation` | Connection from a modulation source with single output |
| 2 | `MultiOutputModulation` | Connection from a modulation source with multiple outputs (SwitchTargets) |

**CRITICAL NOTE:** The `type` member field is **never assigned** anywhere in the constructor or any other method. The constructor does not set `type`, and no external code sets it after construction. This means `getConnectionType()` returns **uninitialized data**. The enum definition exists but the runtime classification is effectively non-functional. This appears to be vestigial infrastructure -- the TODO comment in the class docstring ("Make a hover info box that shows the connection update rate") suggests the class was intended for richer introspection that was never fully implemented.

## Wrapper Struct (API Method Registration)

**File:** `NodeBase.cpp:1622`

```cpp
struct ConnectionBase::Wrapper
{
    API_METHOD_WRAPPER_1(ConnectionBase,     getSourceNode);
    API_VOID_METHOD_WRAPPER_0(ConnectionBase, disconnect);
    API_METHOD_WRAPPER_0(ConnectionBase,     isConnected);
    API_METHOD_WRAPPER_0(ConnectionBase,     getConnectionType);
    API_METHOD_WRAPPER_0(ConnectionBase,     getUpdateRate);
    API_METHOD_WRAPPER_0(ConnectionBase,     getTarget);
};
```

All methods use plain `API_METHOD_WRAPPER_N` / `API_VOID_METHOD_WRAPPER_N` -- **no typed variants** (`ADD_TYPED_API_METHOD_N` is not used).

## Constructor

**File:** `NodeBase.cpp:1632`

```cpp
ConnectionBase::ConnectionBase(DspNetwork* network_, ValueTree data_) :
    ConstScriptingObject(network_->getScriptProcessor(), 0),  // 0 = no constants
    network(network_),
    data(data_)
{
    jassert(data.getType() == PropertyIds::Connection ||
            data.getType() == PropertyIds::ModulationTarget);

    ADD_API_METHOD_0(getTarget);
    ADD_API_METHOD_1(getSourceNode);
    ADD_API_METHOD_0(disconnect);
    ADD_API_METHOD_0(isConnected);
    ADD_API_METHOD_0(getConnectionType);
    ADD_API_METHOD_0(getUpdateRate);
```

**Key points:**
- Passes `0` to `ConstScriptingObject` -- zero constants registered
- Asserts that the ValueTree type is either `PropertyIds::Connection` or `PropertyIds::ModulationTarget`
- No `addConstant()` calls

### Constructor Resolution Logic

After registering API methods, the constructor resolves the connection's graph topology:

```cpp
    auto nodeId = data[PropertyIds::NodeId].toString();
    auto nodeTree = findParentTreeOfType(data, PropertyIds::Node);
    sourceNode = network->getNodeForValueTree(nodeTree);

    if (auto targetNode = network->getNodeWithId(nodeId))
    {
        for(auto p: NodeBase::ParameterIterator(*targetNode))
        {
            if (p->getId() == data[PropertyIds::ParameterId].toString())
            {
                targetParameter = p;
                break;
            }
        }
    }

    if ((sourceInSignalChain = Helpers::findRealSource(sourceNode)))
    {
        if (targetParameter != nullptr)
        {
            auto containerTree = Helpers::findCommonParent(
                sourceInSignalChain->getValueTree(), targetParameter->data);
            commonContainer = network->getNodeForValueTree(containerTree.getParent());
        }
    }
```

**Resolution steps:**
1. **sourceNode**: Found by walking up the ValueTree hierarchy from the connection data to the nearest `Node` tree, then looking up the corresponding NodeBase object in the network.
2. **targetParameter**: The connection data stores `NodeId` and `ParameterId`. The target node is found by ID, then its parameters are iterated to find the matching one.
3. **sourceInSignalChain**: Calls `Helpers::findRealSource()` which traces through cable nodes to find the actual signal-producing node (see below).
4. **commonContainer**: The lowest common ancestor of the source and target in the ValueTree hierarchy. This is used by `getUpdateRate()`.

## ValueTree Data Model

A Connection ValueTree has this structure:

```
Connection (or ModulationTarget)
  - NodeId: String      (target node ID)
  - ParameterId: String (target parameter ID, or "Bypassed" for bypass connections)
```

These ValueTrees live as children of:
- `Connections` tree (child of a Parameter tree) -- for macro parameter connections
- `ModulationTargets` tree (child of a modulation source node) -- for modulation connections
- `SwitchTargets/SwitchTarget[n]/Connections` tree -- for multi-output modulation connections

The connection's **source** is determined by its position in the ValueTree hierarchy (which node/parameter owns the connection tree), while the **target** is stored explicitly via NodeId/ParameterId properties.

## Factory Methods (obtainedVia)

ConnectionBase objects are created in three places:

### 1. ConnectionSourceManager::addTarget (macro/modulation connections)
**File:** `NodeBase.cpp:2031`
```cpp
var ConnectionSourceManager::addTarget(NodeBase::Parameter* p)
{
    p->data.setProperty(PropertyIds::Automated, true, p->parent->getUndoManager());
    auto newC = Helpers::getOrCreateConnection(connectionsTree,
        p->parent->getId(), p->getId(), p->parent->getUndoManager());
    return var(new ConnectionBase(n, newC));
}
```
Called by: `NodeContainer::addMacroConnection()` and `ModulationSourceNode::addModulationConnection()`

### 2. WrapperNode::addModulationConnection (multi-output/switch targets)
**File:** `ModulationSourceNode.cpp:439`
```cpp
var WrapperNode::addModulationConnection(var source, Parameter* n)
{
    int sourceIndex = (int)source;
    auto cTree = getValueTree().getChildWithName(PropertyIds::SwitchTargets)
        .getChild(sourceIndex).getChildWithName(PropertyIds::Connections);
    auto newC = ConnectionSourceManager::Helpers::getOrCreateConnection(
        cTree, n->parent->getId(), n->getId(), getUndoManager());
    return var(new ConnectionBase(getRootNetwork(), newC));
}
```

### 3. Scripting API entry points that return Connection objects:
- **`Node.connectTo(parameterTarget, sourceInfo)`** (`NodeBase.cpp:293`): Delegates to `addModulationConnection(sourceInfo, p)` which dispatches to container or modulation source paths above
- **`Parameter.addConnectionFrom(connectionData)`** (`NodeBase.cpp:1345`): When passed an object, sets `Automated=true` then delegates to either `ModulationSourceNode::addModulationConnection()` or `NodeBase::addModulationConnection()` via the source node

## Helpers

### findRealSource
**File:** `NodeBase.cpp:2123`

```cpp
NodeBase* ConnectionBase::Helpers::findRealSource(NodeBase* source)
{
    if (auto cableNode = dynamic_cast<InterpretedCableNode*>(source))
    {
        source = nullptr;
        auto valueParam = cableNode->getParameterFromIndex(0);
        if(valueParam != nullptr && valueParam->isModulated())
        {
            source = nullptr;
            for (auto allMod : cableNode->getRootNetwork()->getListOfNodesWithType<ModulationSourceNode>(false))
            {
                auto am = dynamic_cast<ModulationSourceNode*>(allMod.get());
                if (am->isConnectedToSource(valueParam))
                    return findRealSource(am);
            }
        }
    }
    return source;
}
```

This recursively traces through cable nodes (e.g. `routing.local_cable`, `routing.global_cable`) to find the actual modulation source. Cable nodes act as intermediaries -- `findRealSource` looks at who modulates the cable's first parameter and follows the chain. If the source is not a cable node, it returns immediately.

### findCommonParent
**File:** `NodeBase.cpp:2112`

```cpp
ValueTree ConnectionBase::Helpers::findCommonParent(ValueTree v1, ValueTree v2)
{
    if (!v1.isValid())
        return v1;
    if (v2.isAChildOf(v1))
        return v1;
    return findCommonParent(v1.getParent(), v2);
}
```

Walks up v1's parent chain until v2 is a child of it. Returns the lowest common ancestor ValueTree node. Used to determine `commonContainer` for block rate computation.

### findParentTreeOfType
**File:** `NodeBase.cpp:588` (free function, not a member)

```cpp
ValueTree findParentTreeOfType(const ValueTree& v, const Identifier& t)
{
    if (!v.isValid()) return v;
    if (v.getType() == t) return v;
    return findParentTreeOfType(v.getParent(), t);
}
```

Used in the constructor to find the parent Node tree from the connection data.

## ConnectionSourceManager

**File:** `NodeBase.h:631`

`ConnectionSourceManager` is not a base class of ConnectionBase -- it is a separate management struct that owns the connection ValueTree and creates ConnectionBase objects. It is mixed into `ModulationSourceNode` and `NodeContainer::MacroParameter`.

Key responsibilities:
- Maintains `connectionsTree` (the Connections or ModulationTargets ValueTree)
- Creates `CableRemoveListener` objects that auto-remove connections when source/target nodes are deleted
- `addTarget(Parameter*)`: Creates a connection ValueTree and returns a new ConnectionBase
- `rebuildCallback()`: Pure virtual -- subclasses rebuild their DSP parameter chains when connections change
- `isConnectedToSource(Parameter*)`: Checks if a parameter is a target of any connection in this source

### CableRemoveListener
Listens for node removal in the ValueTree and auto-cleans up orphaned connections. Each connection gets its own CableRemoveListener that watches both the source and target node trees.

## createParameterFromConnectionTree (DSP Infrastructure)

**File:** `NodeBase.cpp:1673`

This static method is the core DSP infrastructure that builds the actual parameter callback chain from a connection tree. It is NOT an API method but is called internally when connections change.

**Valid parent tree types:**
- `PropertyIds::Connections`
- `PropertyIds::ModulationTargets`
- `PropertyIds::SwitchTargets`

**Logic:**
1. For each child connection in the tree, find the target node and parameter
2. If the target parameter ID is "Bypassed", create a `DynamicBypassParameter` (requires `SoftBypassNode`)
3. Otherwise, get the parameter's `getDynamicParameter()` callback
4. If single connection with matching ranges and no cable/macro indirection, return the parameter directly (optimization)
5. Otherwise, wrap in a `parameter::dynamic_chain<scaleInput>` that fans out to multiple targets with range scaling

**Called by:**
- `ModulationSourceNode::rebuildCallback()` -- rebuilds modulation chain
- `NodeContainer.cpp:813` -- rebuilds macro parameter chain
- `DynamicParameterList.cpp:339` -- rebuilds switch target chains

## InterpretedCableNode

**File:** `StaticNodeWrappers.h:562`

```cpp
struct InterpretedCableNode : public ModulationSourceNode,
                              public InterpretedNodeBase<OpaqueNode>
```

Cable nodes (`routing.local_cable`, `routing.global_cable`, and other `control.*` nodes) are instantiated as InterpretedCableNode. They are ModulationSourceNodes that act as signal intermediaries. The `findRealSource()` helper specifically checks for this type to trace through cables.

## Threading / Lifecycle

- ConnectionBase is created on the scripting/message thread
- All API methods are read-only except `disconnect()` which modifies the ValueTree (with UndoManager)
- The object uses `WeakReference` for all node/parameter references -- safe against deletion
- `isConnected()` and `objectDeleted()`/`objectExists()` all check parent ValueTree validity
- After `disconnect()` is called, `isConnected()` returns false and all getters return empty/null/0

## Preprocessor Guards

None. The ConnectionBase class has no `#if USE_BACKEND` or other preprocessor guards. It is available in all build targets. The `createParameterFromConnectionTree` method is also unconditional.

(The `getCurrentBlockRate()` method on NodeBase that `getUpdateRate()` calls is also unconditional, though signal peak display code elsewhere is `USE_BACKEND`-only.)
