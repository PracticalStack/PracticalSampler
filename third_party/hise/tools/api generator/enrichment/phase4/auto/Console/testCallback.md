Synchronously invokes a named callback on a UI component for automated testing. The callback executes immediately with the provided arguments. Diagnostic messages are printed to the console, and the method throws a script error if callback execution fails. Intended for use with AI agents and automated testing tools, not for production code.

> [!Warning:Intended for automated testing only] Intended only for automated testing. A warning is logged if called outside a testing configuration. The component must support the specified callback or an error is thrown.
