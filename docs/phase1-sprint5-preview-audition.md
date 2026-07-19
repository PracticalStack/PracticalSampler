# Mini Sprint 5.5 Audition Command And Routing Contract

Completed July 19, 2026.

## Outcome

All creator audition surfaces now enter one typed `AuthoringPreviewCommandAdapter`. The adapter owns
Preview note gestures, emits sample-offset events for the Preview queue only, and never reads or
mutates Performance activation, MIDI, voices, or diagnostics.

## Command vocabulary

The public command set is `noteOn`, `noteOff`, `stopAll`, `emergencyReset`,
`auditionSelectedZone`, and `auditionCurrentDraft`. Every command identifies its source as summary
Preview, authoring keyboard, zone map, or inspector. Note commands carry MIDI note, normalized
velocity, and sample offset. Selected-zone auditions also carry the selected-zone identity.

Selected/current audition commands request preparation explicitly. Plain note-on/off commands do
not service controller or worker lifecycle work implicitly.

## Ownership and lifecycle

- Ownership is counted per source and note; repeated presses require matching releases.
- A shared note-off is emitted only after the final source owner releases that note.
- Summary, zone-map, and inspector gestures use component-owned timers instead of detached delayed
  callbacks. Editor teardown releases every outstanding timed gesture.
- `stopAll` clears ownership and emits ordinary Preview all-notes-off release.
- `emergencyReset` clears ownership and emits an immediate Preview reset.
- Selection or activation replacement does not reset old voices. Existing voices retain their
  immutable model lease; later notes use the newly active model.

## Routing boundary

The processor translates adapter events into the bounded Authoring Preview queue. Exact offsets are
clamped only to the current audio block. The queue drains into `authoringPreviewPlaybackContext`;
host MIDI and performance-surface events continue to drain into `performancePlaybackContext`.
Preview stop/reset therefore cannot release same-note Performance voices or change Performance
revision/build identity.

Zone-map audition copies the hit-zone value before invoking the synchronous selection callback.
This is required because selection refresh can replace the canvas view-model storage before the
audition callback runs.

## Frozen behavior

The Mini Sprint 5.1 stale-note policy remains unchanged: selection and activation replacement let
old-model Preview voices finish or release against their original immutable model. Missed UI
note-offs recover through Preview-only stop/reset or editor-owned timed release. No recovery command
is forwarded to Performance.

