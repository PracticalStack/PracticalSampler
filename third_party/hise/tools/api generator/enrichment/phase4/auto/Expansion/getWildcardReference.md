Constructs a wildcard reference string in the format `{EXP::ExpansionName}relativePath` by prepending this expansion's wildcard prefix to the given path. The resulting string can be used with any pool-based API to address expansion-specific resources - for example, setting an image on a UI component or loading a sample map.

> [!Warning:No file existence check] The method performs string concatenation only. It does not verify that the referenced file exists. A wrong relative path produces a valid-looking reference that silently fails to load.
