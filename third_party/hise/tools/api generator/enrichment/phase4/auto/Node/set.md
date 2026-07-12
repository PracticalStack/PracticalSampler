Sets a node-type-specific property or a direct node attribute. If the ID matches a node property (e.g. Mode, Frequency), it updates that value. If it matches a direct attribute (Bypassed, NodeColour, Comment, Folded), it updates that instead. Both locations are checked independently.

> [!Warning:Unknown property IDs fail silently] If the property ID does not match any known property or attribute, the call does nothing and reports no error.
