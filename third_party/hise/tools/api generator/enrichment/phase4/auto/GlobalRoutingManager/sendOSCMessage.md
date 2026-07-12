Sends an OSC message to the configured target. The sub-address is appended to the connection domain to form the full OSC address. Returns `true` on success, `false` if no sender is connected.

The data parameter accepts floats, integers, Strings, or an Array of these types for multi-argument messages. Note that the `Parameters` range defined in `connectToOSC` is not applied to values sent through this method - you send and receive raw data, so apply any scaling in your script.
