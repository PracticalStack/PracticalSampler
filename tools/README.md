# Tools

Local developer scripts, validation helpers, and repository automation live here.

Phase 0 should keep tools small, explicit, and documented so a fresh clone does not depend on hidden machine state.

Current helpers:

- `bootstrap-windows.ps1` configures and builds the Windows-first app, plugin, and baseline smoke target.
- `install-vst3-windows.ps1` copies the built VST3 bundle into the system VST3 directory for host validation.
- `package-phase1-reference-instrument.ps1` builds the Phase 1 runtime fixture tool and either verifies or refreshes the checked-in tiny-open-instrument package metadata plus the sealed package corpus.
- `run-phase1-benchmark-scene.ps1` builds and runs the checked-in Phase 1 benchmark scene, then writes a comparable JSON report artifact.
