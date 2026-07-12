Creates an NKS (Native Kontrol Standard) manager for integration with Native Instruments hardware controllers. Requires the `HISE_INCLUDE_NKS_SDK` preprocessor flag.

> [!Warning:Requires NKS SDK build flag] Throws a runtime script error if the NKS SDK is not compiled in. There is no way to check availability before calling.