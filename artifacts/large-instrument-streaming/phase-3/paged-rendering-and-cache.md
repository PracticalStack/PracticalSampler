# Phase 3 paged rendering and cache evidence

Date: 2026-08-05

## Result

STR-300 through STR-304 are complete. The later actual-corpus qualification produced audible selected-zone and selected-group playback with zero normal-profile misses/underrun frames and a 273 us maximum callback; see `../phase-7/accurate-salamander-qualification.md`.

## Render and intent contract

- `SamplerVoice` obtains current and interpolation-next frames only through bounded `ISampleDataSource` views.
- An all-ready deterministic paged source matches the resident voice output across 4-frame head/page boundaries at the established `1e-6` tolerance.
- Each voice publishes a primitive look-ahead intent initially and at a bounded 256-frame cadence, plus imminent/look-ahead intents on a miss.
- The SPSC ring owns 256 fixed entries. Its saturation test accepts exactly 256, rejects the 257th without allocation or blocking, and drains FIFO.
- The worker scheduler deduplicates generation/page keys, upgrades priorities, displaces lower-priority work deterministically, and cancels obsolete generations off audio.

## Late-page policy

The v1 policy is bounded silence. Musical position and release time continue to advance; no callback wait or log occurs. A forced three-frame page miss advances the cursor from frame 3 to frame 6. Publishing that page then renders source frames 6 and 7 and records exactly one recovery, without replaying missed material.

Voice results expose page misses, underrun frames, and recovery counts. The voice pool aggregates those values into activation/block render diagnostics.

## Cache contract and measured trace

- Heads are pinned outside the evictable page budget.
- Page allocation is admitted only after off-audio reclamation makes room.
- An atomic active-acquisition handshake plus page leases prevents reclamation racing a render lookup.
- LRU selection ignores leased pages; retired storage is reclaimed only off audio after acquisitions and leases reach zero.
- Metrics expose head/page hits, page misses, range-read latency, queue depth, allocated/resident/leased/retired bytes, evictions, pinned skips, and pressure failures.

```text
Page cache trace: budgetBytes=65536 peakAllocatedBytes=65536 evictions=1 pinnedSkips=1 pressureFailures=1 hits=2 misses=1 maxReadMicros=274
```

The pinned replacement attempt failed at 65,536 bytes without overshoot. After lease release, one page was reclaimed before its replacement was allocated; peak allocation remained exactly the configured budget.

## Realtime and conformance

- The streamed two-layer voice-pool matrix crosses page boundaries, starts four voices for two overlapping notes, applies release to the expected two voices, and reports zero allocations/deallocations while the render probe is enabled.
- Resident render model, voice, scheduler, lifecycle, playback context, offline golden, and crossfade suites pass.
- Performance-engine S0–S4, S6, and S8 pass.
- Four existing Debug-baseline defects remain red in the broader focused run: realtime-guard counter contamination, S5 keyswitch selection, S7 pedal root behavior, and S9 round-robin advancement. The streaming implementation does not modify those resolution paths and does not suppress these failures. The focused result improved from the pre-fix stale 13/18 to 14/18 after updating the render-model rejection expectation for the new source-or-resident contract.

## Focused commands

```powershell
cmake --build --preset build-debug --target drs_sprint4_render_model_tests drs_sprint4_voice_kernel_tests drs_sprint4_scheduler_tests drs_sprint4_voice_lifecycle_tests drs_sprint4_playback_context_tests drs_sprint4_offline_renderer_tests drs_sprint4_crossfade_mixing_tests drs_sprint4_entry_realtime_guard_tests drs_performance_engine_s0_fixture_tests drs_performance_engine_s1_articulation_tests drs_performance_engine_s2_rule_tests drs_performance_engine_s3_program_tests drs_performance_engine_s4_state_tests drs_performance_engine_s5_keyswitch_tests drs_performance_engine_s6_release_tests drs_performance_engine_s7_pedal_tests drs_performance_engine_s8_choke_tests drs_performance_engine_s9_round_robin_tests
ctest --test-dir build\vs2022-debug -C Debug --output-on-failure -R "^(drs\.sprint4\.(render_model|voice_kernel|scheduler|voice_lifecycle|playback_context|offline_renderer|crossfade_mixing)|drs\.sprint4_entry\.realtime_guard|drs\.performance_engine\.s[0-9]+\.(fixtures|articulations|rules|program|state|keyswitch|release|pedal|choke|round_robin))$"
```
