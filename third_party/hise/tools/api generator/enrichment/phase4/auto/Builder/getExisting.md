Registers a pre-existing module (one not created by this Builder instance) into the Builder's tracking array and returns its build index. This lets you reference modules from a previous build pass or from the original module tree.

A common pattern is multi-pass building: create containers in one pass, then use `getExisting()` in a second pass to look up those containers and add children to them. This is particularly useful for send effect architectures where per-channel sends must reference a shared send bus created separately.

If the module is already tracked by this Builder, the existing index is returned without adding a duplicate. Reports a script error if no processor with the given ID is found.
