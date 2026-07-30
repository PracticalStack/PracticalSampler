# REAPER Host-State Validation Evidence

Signed by: Codex automated REAPER validation  
Host: REAPER 7.39/win64  
Platform: Windows x64  
Captured: July 30, 2026 UTC  
Plug-in: VST3i Decent Rhapsody Studio  
Audio configuration: REAPER Dummy Audio, 44.1 kHz, 256-frame block, looped transport

The reproducible harness is in `validation/reaper/`. It launches REAPER with an isolated
configuration, injects a bounded JUCE VST3 state chunk, runs transport to exercise block-boundary
activation, waits for deferred restore, captures plug-in parameters and track chunks, and closes
the host. Each evidence file contains its own signature and UTC capture time.

## Signed checklist

| Scenario | Expected result | Captured result | Status |
|---|---|---|---|
| Editor open | Authored project restores and the editor does not own restore progress. | Tone `0.6200000048`, Motion `0.7799999714`, one enabled online VST3 instance, floating editor present. | Pass |
| Editor closed | Restore completes without constructing an editor. | Tone `0.6200000048`, Motion `0.7799999714`, one enabled online VST3 instance, no floating editor. | Pass |
| Moved project | Unverified content does not mutate preset/audio state; recovery UI is available. | Startup Tone `0.3499999940`, Motion `0.1500000060`; floating recovery editor present. | Pass |
| Changed manifest | Matching ID with a different canonical digest is rejected. | Startup Tone `0.3499999940`, Motion `0.1500000060`; floating recovery editor present. | Pass |
| Missing sample/content | Preparation cannot masquerade as successful recall. | Startup Tone `0.3499999940`, Motion `0.1500000060`; floating recovery editor present. | Pass |
| Duplicate instances | Each new processor completes its own deferred restore with editors closed. | Two tracks; both report Tone `0.6200000048`, Motion `0.7799999714`; neither has a floating editor. | Pass |

The active restored track chunk reserialized a complete `drs.hostState` publication, including
project ID `drs.phase2.authoring-foundation`, canonical manifest digest, authoring revision
metadata, project generation, authored-content digest, macro-schema digest, and prepared-content
digest. This proves the host receives project-aware state after activation rather than the
startup/reference preset.

## Evidence files and SHA-256

| File | SHA-256 |
|---|---|
| `active-editor-open.reaper-evidence.txt` | `4904CC61727A0F3DBE7FCA073C01B970A8A40019AE9F3112EC1418F006B2A636` |
| `active-editor-closed.reaper-evidence.txt` | `9000B7DE8972374BE7496ABCDD4FD79B8BE012C88C83B750EBA305EE8418942D` |
| `moved-project.reaper-evidence.txt` | `9473AA2D847EF2D22282C199AA95C4AD40A97A463ED1F05EBB70E120149D4C22` |
| `changed-manifest.reaper-evidence.txt` | `4EB57F93B88515E5CB10212B767E22F1756E0332B56C8F7A2C5330D14BD4BC12` |
| `missing-sample.reaper-evidence.txt` | `7024098A1136A2A2DD16A8D63978282702EE10DBFE8165C466FC48059CC3FE36` |
| `duplicate-instances.reaper-evidence.txt` | `F71A9CA37C4EDE7C56C714563F50B72B6F65571AF8AD062E99E448E007479FEB` |
| `validate-scenario.lua` | `BBBED0273005F67B8225FEE34E6B406345BFF4B110736EDD9981CF99F72CAE81` |
| `inject-host-state.ps1` | `F96200C3E0790A7A2B8FB96CCBF6CCE0225870221FE64EB96ED3F5433C2B635E` |

## Reproduction

1. Build `drs_plugin_bundle_VST3` in `build/vs2022-debug`.
2. Make the bundle discoverable to the isolated REAPER configuration.
3. Run `validation/reaper/make-scenarios.ps1`.
4. Start REAPER with `-cfgfile validation/reaper/reaper.ini`, one scenario `.rpp`, and
   `validation/reaper/validate-scenario.lua`.
5. Wait for the signed evidence file. Duplicate instances deliberately receive a second settle
   window after REAPER's native Duplicate Tracks action.

The harness uses a dummy audio device so it is deterministic and does not modify the user's
normal REAPER audio configuration.
