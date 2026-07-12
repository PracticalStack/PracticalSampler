Creates a BX Licenser object for copy protection using the proprietary BX SDK. Requires the `HISE_INCLUDE_BX_LICENSER` preprocessor flag to be enabled in the project's extra definitions before export.

> [!Warning:Requires BX SDK build flag] Calling this without the BX SDK enabled throws a runtime script error. There is no way to check at script level whether the SDK is compiled in.