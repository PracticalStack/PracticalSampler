Adds or removes a modulation connection to this parameter. Pass a JSON object with `ID` (source node) and `ParameterId` (source parameter name) to create a connection. Pass any non-object value (e.g. `0`) to remove the existing connection. Returns the new [Connection]($API.Connection$) object on success, or `undefined` if the connection could not be created.

> [!Warning:Remove existing connection before adding] If the parameter already has a connection (Automated flag is true), passing a new connection descriptor silently returns `undefined`. Call `p.addConnectionFrom(0)` first to remove the existing connection.

> [!Warning:Invalid source node fails silently] If the source node ID does not match any node in the network, the call returns `undefined` without error. Double-check the `ID` string matches the node name exactly.
