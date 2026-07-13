# Phase 1 Diagnostics Panel

This note captures Sprint 4 task `P1-403`: the first developer-facing diagnostics surface for the product-owned runtime shell.

## Current scope

`StatusPanel` now exposes a structured diagnostics summary instead of only a long text dump.

The panel surfaces:

- current preset id, selected load profile, and selected articulation
- configured cache budget and max prefetch-per-voice budget
- active and peak voice counts
- page misses, cache hits/misses, background reads, and resident/pending page counts
- purge passes, dormant purges, cumulative/last purge eviction counts, and read latency
- an obvious failure-state banner when restore or diagnostics validation fails

The data comes from a product-owned synthetic runtime exercise driven by the checked-in Phase 1 reference instrument and stream container.

## Developer actions

The panel now includes three explicit developer actions:

- `Reset Default State`
- `Load Lead Fixture`
- `Inject Invalid State`

These are intentionally narrow. They let the team verify load-profile changes and failure-state reporting while the app is running without introducing a broad preset browser yet.

## Manual QA script

1. Build and launch the standalone shell or open the plugin editor.
2. Confirm the diagnostics panel shows:
   - a non-empty load profile and articulation
   - non-zero page misses and background reads
   - at least one dormant purge and a non-zero purge eviction count
3. Click `Load Lead Fixture` and confirm the panel switches to:
   - `loadProfile=performance`
   - `articulation=lead`
   - the larger cache budget
4. Click `Inject Invalid State` and confirm:
   - the failure-state banner becomes non-empty
   - the last known-good load profile and articulation remain visible
5. Click `Reset Default State` and confirm the failure banner clears and the default balanced session returns.

## Validation

`drs_phase1_diagnostics_tests` now proves:

- the default diagnostics snapshot is available and exposes page misses, purge activity, and peak voice counts
- restoring the lead/performance preset updates the diagnostics snapshot to the larger reviewed cache budget
- rejected invalid state leaves diagnostics available while surfacing an explicit failure state
