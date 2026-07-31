# WAV Import Host Validation Evidence

Signed by: Codex automated WAV host validation  
Captured: July 31, 2026 UTC  
Standalone target: `drs.wav_import.host_validation`  
Host: REAPER 7.39/x64  
Plug-in: VST3i Decent Rhapsody Studio  
Audio configuration: REAPER Dummy Audio, 44.1 kHz, looped transport

This evidence closes WAV-705 by pairing a checked CTest gate with a reproducible REAPER startup
matrix. The CTest side proves standalone construction and project replacement remain zero-I/O for
missing-local, removable-drive-like, and UNC-like sample-source paths. The REAPER side proves the
same path classes do not delay plug-in discovery or host instantiation: each project produced one
enabled online VST3 instance, the full 2,086-parameter surface, the safe startup Tone/Motion macro
defaults, and a captured track chunk without waiting on missing media.

## Signed checklist

| Surface | Scenario | Captured result | Status |
|---|---|---|---|
| Standalone | Missing local path | Construction `891 ms`, replace `420 ms`, zero import I/O, snapshot state `not-run`. | Pass |
| Standalone | Removable-drive-like path | Construction `805 ms`, replace `411 ms`, zero import I/O, snapshot state `not-run`. | Pass |
| Standalone | UNC-like network path | Construction `836 ms`, replace `425 ms`, zero import I/O, snapshot state `not-run`. | Pass |
| REAPER | Missing local path | `instantiation_elapsed_ms=3`, `parameter_count=2086`, Tone `0.3499999940`, Motion `0.1500000060`, track chunk captured. | Pass |
| REAPER | Removable-drive-like path | `instantiation_elapsed_ms=0`, `parameter_count=2086`, Tone `0.3499999940`, Motion `0.1500000060`, track chunk captured. | Pass |
| REAPER | UNC-like network path | `instantiation_elapsed_ms=0`, `parameter_count=2086`, Tone `0.3499999940`, Motion `0.1500000060`, track chunk captured. | Pass |

The REAPER harness lives in `validation/reaper/`:

- `make-wav-import-scenarios.ps1` rewrites the Phase 2 reference project to the three location classes.
- `validate-wav-import-startup.lua` starts transport, waits only until the plug-in is online with a readable parameter surface, records instantiation timing, and captures the restored track chunk.
- `run-wav-import-matrix.ps1` launches the isolated REAPER config against each scenario and stops the host after the signed evidence appears.

## Evidence files and SHA-256

| File | SHA-256 |
|---|---|
| `wav-import-missing-local.wav-import-evidence.txt` | `5217FFD34A4F5EB4508A37A91EFE36BFB3D94753F6DAD2092539DE3ED8A20D91` |
| `wav-import-removable-media.wav-import-evidence.txt` | `44C19461662C781A03A538B5DA279CC244BEC3EEB5A8054232D7FC733211907F` |
| `wav-import-network-media.wav-import-evidence.txt` | `AD00FC57DBC281ECE16A588084825BC5299AB58FB26C97DFA51825A2C6CBFF04` |
| `wav-import-missing-local.wav-import-track-chunks.txt` | `ABA12655CF20E5D62F90EF0CFBD79A78D50D92DC17433E758410A931526E56CD` |
| `wav-import-removable-media.wav-import-track-chunks.txt` | `ABA12655CF20E5D62F90EF0CFBD79A78D50D92DC17433E758410A931526E56CD` |
| `wav-import-network-media.wav-import-track-chunks.txt` | `ABA12655CF20E5D62F90EF0CFBD79A78D50D92DC17433E758410A931526E56CD` |
| `make-wav-import-scenarios.ps1` | `FAB6785DB4A004604E2B85786E01B5D886A8852666402E7450045B06F9612B3E` |
| `validate-wav-import-startup.lua` | `7D2089B995EACB5CD7630671CEC28339B1A9C7FE01F073EE576BF24F79BC15EB` |
| `run-wav-import-matrix.ps1` | `9A221155DDAF2DFC8A1ECBC0D6E778038BF66ADC0935185DC1B01317B13C3262` |

## Reproduction

1. Build `drs_wav_import_host_validation_tests` and `DecentRhapsodyStudioPlugin` in `build/vs2022-debug`.
2. Run `validation/reaper/make-wav-import-scenarios.ps1`.
3. Run `validation/reaper/run-wav-import-matrix.ps1`.
4. Run the focused CTest slice that includes `drs.wav_import.host_validation`.

The harness uses the isolated `validation/reaper/reaper.ini` configuration, so it does not alter a
user's normal REAPER device settings or scan cache.
