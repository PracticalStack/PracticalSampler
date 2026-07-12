Purges or unpurges a mic position channel by name. The name must match the channel suffix exactly - use `Sampler.getMicPositionName()` to get valid names. Only works with multi-mic samplers.

> [!Warning:Call on each sampler individually] When managing mic positions across multiple samplers, you must call this on each sampler individually. There is no global purge API.
