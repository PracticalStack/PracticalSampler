Engine::setMaximumBlockSize(Number numSamplesPerBlock) -> undefined

Thread safety: INIT -- may call prepareToPlay(), re-initializing entire audio chain
Sets maximum internal processing buffer size. Host buffers larger than this are split.
Rounded down to nearest multiple of HISE_EVENT_RASTER (8). Clamped to 16..512.
Smaller = higher control rate resolution but more CPU.
Anti-patterns:
  - Value is silently rounded down -- 100 becomes 96 with no warning
Source:
  ScriptingApi.cpp  Engine::setMaximumBlockSize()
    -> MainController::setMaximumBlockSize() -> prepareToPlay() if changed
