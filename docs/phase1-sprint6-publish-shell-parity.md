# Mini Sprint 6.8 Publish Commands, Status, Routing, And Shell Parity Contract

Date: July 20, 2026  
Status: Implemented

## One command path

Standalone, plug-in editor, authoring workspace, and `StatusPanel` Publish actions submit a typed
`PerformancePublishCommand` to the processor-owned `PerformancePublishCommandAdapter`. Only the
processor invokes the facade Publish implementation. The adapter records command source,
acceptance/rejection, and execution outcome without owning controller eligibility policy.

Closing an editor destroys only UI callbacks. The processor continues to own the command adapter,
Publish controller, preparation service, activation payloads, macro bindings, realtime contexts,
and diagnostics.

## Immutable presentation truth

`EngineFacade` atomically publishes one immutable `PerformancePublishPresentationSnapshot` whenever
Publish or Draft playback truth changes. It contains:

- typed Idle, Queued, Preparing, Ready, Activating, Active, Stale, Failed, Canceled, and Superseded
  state;
- enabled and dirty state plus progress and recovery guidance;
- Draft, Preview, requested Publish, active published, failed, and last-known-good revisions and
  content digests;
- active macro schema and prepared build identity;
- failure code/path/message and retained payload bytes; and
- queue depth, in-flight depth, preparation, request-to-ready, and request-to-active metrics.

The Performance surface and diagnostics panel read this snapshot. Both shells use the same shared
components and providers, so compact plug-in and expanded standalone views cannot invent separate
lifecycle policy.

## Routing ownership

- Host MIDI and the Performance keyboard enter only the Performance event queue/context.
- Authoring keyboard, summary Preview, zone map, inspector, and audition actions enter only the
  typed Preview adapter and Preview event queue/context.
- Publish commands cannot emit note events or mutate Preview ownership.
- Preview stop/reset cannot release Performance voices; host all-notes-off/reset cannot address
  Preview.

## Accessibility and lifetime

Apply/Publish controls have stable component IDs, title, description, help text, and enabled state.
The diagnostics action describes the current typed state and guidance. Editor close/reopen and
editor-closed processing preserve controller, activation, macro, and presentation identity.
