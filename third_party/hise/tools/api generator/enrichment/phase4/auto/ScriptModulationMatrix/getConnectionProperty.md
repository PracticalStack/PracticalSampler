Returns the value of a specific property on the connection between the given source and target. Valid property IDs are `Intensity`, `Mode`, `Inverted`, `AuxIndex`, and `AuxIntensity`.

> [!Warning:Returns undefined for all failure cases] Returns `undefined` if the source is not found, no connection exists, or the property ID is invalid, with no way to distinguish between these cases.
