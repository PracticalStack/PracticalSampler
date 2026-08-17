# Sprint 2 — Interactive viewer and scalable peak pipeline

Status: implemented; focused Debug and Release validation passed on 2026-08-17.

## Delivered contract

- The existing 192-point whole-source preview remains the fit-to-file overview.
- Zoomed viewports request independent source-frame ranges at up to 4,096 peak points.
- Range requests read only their requested frames through the existing 4,096-frame worker buffer.
- Paint, resize, pointer, wheel, and keyboard handlers perform no file or decoder I/O. They publish viewport intent only.
- Exact tiles are cached by project, source identity, file size, modification time, frame range, point count, and channel policy.
- The default cache is bounded to 64 entries and 8 MiB. Snapshots publish entry, byte, hit, and eviction metrics.
- New generations cancel older work. Publications older than the newest submitted generation are rejected.
- A compatible completed result remains available while newer detail is queued or building.

## Viewer interaction

- Mouse wheel: zoom around pointer.
- Drag: horizontal pan.
- Left/Right: pan by one tenth of the visible range.
- `+` / `-`: zoom around viewport center.
- `Home`, `0`, or double-click: fit whole source.
- Viewport frame bounds remain unchanged when the component is resized.

The editor renders detailed peaks only when a detail segment covers the viewport. Otherwise the overview remains aligned to its full-source frame positions, so the 192-point overview is never stretched and presented as detailed audio.

## Focused validation

- `drs.waveform_region.policy`
- `drs.wav_import.waveform_peak_builder`
- `drs.wav_import.waveform_preview_service`
- `drs.phase2.waveform_preview`
- `drs.phase2.authoring_ui`
- `drs.open_workbench.phase2`
- Debug `DecentRhapsodyStudioPlugin` build
- Debug `DecentRhapsodyStudioApp` build

Coverage includes pointer-centered zoom, edge-clamped pan, exact visible-range coverage, range-only chunk reads, cache hits without new reads, cache eviction bounds, cancellation, supersession, selected-zone preview regression, and authoring layout/reachability regression.

## Deliberate Sprint 3 boundary

Sprint 2 is read-only navigation. It exposes selection, playhead, provenance, region, and presentation-state fields in the waveform presentation model, but does not yet persist waveform gestures or mutate SFZ-derived region values. Loop handles, selection commands, transactions, undo/redo, and audition remain Sprint 3 work.
