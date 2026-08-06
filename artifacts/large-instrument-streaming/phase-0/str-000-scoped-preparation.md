# STR-000 Scoped Preparation Evidence

Date: 2026-08-05  
Status: verified complete

## Implemented contract

- `PlaybackPreparationScopeRequest` carries current-draft, selected-zone, or selected-group scope into snapshot construction.
- `scopePlaybackSnapshotForPreparation()` computes the dependency closure before `PreparedPlaybackService` realizes sample data.
- Selected-zone closure retains crossfade neighbors, round-robin peers, matching release triggers, group routes, and inherited instrument/master routing while excluding unrelated zones, sources, and buses.
- Activation payloads expose unscoped/retained zone and source counts plus the exact scope identity.
- Current-draft preparation remains an explicit forced full-scope request.
- Equivalent scoped requests preserve/reuse deterministic content identity.

## Regression evidence

Build command:

```text
cmake --build --preset build-debug --target drs_sprint4_offline_renderer_tests drs_sprint5_preview_contract_tests drs_sprint5_preview_controller_tests drs_sprint5_preview_controller_integration_tests drs_sprint5_preview_coalescing_tests drs_sprint5_preview_preparation_tests drs_sprint5_preview_audition_tests drs_sprint5_preview_recovery_tests drs_sprint5_preview_shell_parity_tests drs_sprint5_preview_contract_red_tests drs_sprint5_integration_hardening_tests
```

CTest command:

```text
ctest --test-dir build\vs2022-debug -C Debug --output-on-failure -R "^(drs\.sprint5\.|drs\.sprint4\.offline_renderer$|drs\.phase0\.smoke$)"
```

Result: 12/12 passed in 46.45 seconds.

`drs.sprint5.preview_preparation` specifically proves:

- a 5-zone synthetic selected-zone closure retains exactly 4 dependent zones/sources and removes the unrelated route;
- a production 3-zone/2-source selected-zone request prepares exactly 1 zone and 1 source;
- a selected-group request prepares exactly 2 zones sharing 1 source;
- explicit current-draft preparation retains all 3 zones and both sources;
- repeat requests retain deterministic snapshot identity.

Additional fixes qualified by the same run:

- the cancellation race begins with a genuine authored last-known-good activation;
- explicit bundled-session reset prepares an immutable reference payload even when the authoring shell is unbound;
- a manifest-only performance workspace falls back to that explicitly restored bundled payload only when no real package payload exists.

## Files

- `engine_adapter/include/drs/engine/PlaybackSnapshot.h`
- `engine_adapter/src/PlaybackSnapshot.cpp`
- `engine_adapter/include/drs/engine/DraftPlaybackContract.h`
- `engine_adapter/src/DraftPlaybackContract.cpp`
- `engine_adapter/include/drs/engine/EngineFacade.h`
- `engine_adapter/src/EngineFacade.cpp`
- `app/src/plugin/PluginProcessor.cpp`
- `tests/src/Sprint5PreviewPreparationTests.cpp`
- `tests/src/Sprint5PreviewControllerIntegrationTests.cpp`
- `tests/src/Sprint5PreviewCoalescingTests.cpp`
- `tests/src/Phase0SmokeTests.cpp`
