Creates or removes a dynamic bypass connection on this node. When called with valid source information, a connection is created from the source to this node's bypass state. When called without valid source data, any existing bypass connection targeting this node is removed.

The target node must be a `container.soft_bypass` node. Other node types produce an IllegalBypassConnection error when the connection is processed.
