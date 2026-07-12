Loads trained weights into the current model. Call this after `build` to complete the two-step PyTorch workflow. The weight data must be a JSON object matching the format exported by PyTorch's `state_dict()`. The model is automatically reset after loading.

> [!Warning:Only works with models created via build] Calling `loadWeights` on a TensorFlow model fails because TensorFlow models initialise weights from the model JSON. Use `loadWeights` only after `build` for the two-step PyTorch workflow.
