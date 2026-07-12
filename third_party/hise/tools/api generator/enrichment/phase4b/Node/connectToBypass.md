Node::connectToBypass(var sourceInfo) -> undefined

Thread safety: UNSAFE -- modifies ValueTree connection structure with undo manager.
Creates or removes a dynamic bypass connection on this node. Connect mode: when sourceInfo
contains valid source data, creates a Connection entry targeting this node's Bypassed
property. Disconnect mode: when no valid source found, searches for and removes existing
bypass connections.
Dispatch/mechanics:
  Connect: creates Connection ValueTree in source's connection tree targeting Bypassed
  Disconnect: searches container macro, switch target, and modulation source trees
    for existing bypass connection and removes it
Pair with:
  setBypassed -- for direct (non-dynamic) bypass control
  connectTo -- for parameter connections instead of bypass connections
Anti-patterns:
  - Target node must be a SoftBypassNode for dynamic bypass. The validation error
    (IllegalBypassConnection) is raised downstream, not in this method.
Source:
  NodeBase.cpp  NodeBase::connectToBypass()
    -> searches source parameter/modulation data from sourceInfo
    -> creates/removes Connection ValueTree with parameterId = "Bypassed"
    -> DynamicBypassParameter handles runtime bypass logic
