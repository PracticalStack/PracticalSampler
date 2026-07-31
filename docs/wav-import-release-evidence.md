# WAV Import Release Evidence

Signed by: Codex release audit  
Captured: July 31, 2026 UTC  
Scope: `wav-import-startup-decode-development-plan.html` through WAV-706

This note records the final release-facing evidence for the WAV startup/decode rollout after the
legacy synchronous shell path was retired.

## Release checklist

| Requirement | Evidence | Status |
|---|---|---|
| No production reference remains to the synchronous authoring import workflow. | `rg -n "createAuthoringImportQueue\|processNextAuthoringImportQueueItem\|prepareWavImportBatch\(|copySampleFileForImport\(" app/src -g "*.cpp" -g "*.h"` returned no matches on July 31, 2026. | Pass |
| Plugin and standalone use the same completion-driven request/result/commit workflow. | Both shells submit `WavImportRequest`, poll immutable snapshots, and apply `prepareWavImportBatchFromCompletion(...)`, `resolvePreparedWavImportManualRoot(...)`, `takePreparedWavImportCommit(...)`, `finalizePreparedWavImportCommit(...)`, and `rollbackPreparedWavImportCommit(...)`. | Pass |
| Startup and lifecycle surfaces stay free of hidden sample I/O. | `drs.wav_import.lifecycle_io_audit`, `drs.wav_import.lifecycle_stress`, `drs.wav_import.ci_budgets`, and `drs.wav_import.host_validation` all passed on July 31, 2026. | Pass |
| The supported-host validation matrix remains green. | `validation/reaper/run-wav-import-matrix.ps1` captured signed REAPER evidence for missing-local, removable-media-like, and UNC-like sample locations. | Pass |
| Architecture and diagnostics docs describe the async-only product path. | `architecture-overview.md`, `wav-import-baseline-report.md`, `host-validation.md`, and `wav-import-host-validation-evidence.md` were updated to distinguish the historical synchronous baseline from the shipped async workflow. | Pass |

## Focused validation

The following Windows Debug slice passed in `build/vs2022-debug` on July 31, 2026:

- `drs.wav_import.workflow`
- `drs.wav_import.shell_characterization`
- `drs.wav_import.processor_responsiveness`
- `drs.wav_import.lifecycle_io_audit`
- `drs.wav_import.ci_budgets`
- `drs.wav_import.host_validation`

The broader 22-test Windows Debug WAV audit slice also passed on July 31, 2026:

- `drs\.wav_import\.|drs\.phase2\.authoring_import|drs\.phase2\.waveform_preview|drs\.phase2\.authoring_ui|drs\.host_state\.project_recall|drs\.phase1\.sample_import`
- Result: `100% tests passed, 0 tests failed out of 22`
- Notable runtime: `drs.wav_import.lifecycle_stress` passed at `712.89 sec`

The REAPER matrix also passed on July 31, 2026:

- `wav-import-missing-local`
- `wav-import-removable-media`
- `wav-import-network-media`

The signed host evidence and hashes are cataloged in
[wav-import-host-validation-evidence.md](wav-import-host-validation-evidence.md).

## Historical baseline retention

`validation/wav-import/sync-shell-baseline.json` remains checked in as a historical artifact. It is
kept for comparison against the pre-async shell behavior, not as a description of the current
product path.
