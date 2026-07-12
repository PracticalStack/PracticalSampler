Restores this component's properties, children, and optionally values from a Base64-encoded string produced by `toBase64()`.

> [!Warning:Restoration is deferred] The component tree is not updated immediately when this method returns. Subsequent reads may still reflect the old state until the deferred update completes.
