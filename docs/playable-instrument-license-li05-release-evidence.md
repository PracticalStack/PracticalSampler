# Playable Instrument License LI-05 Release Evidence

Status: complete on 2026-08-14. This closes the five-slice playable-instrument license iteration.

## Host parity

- The generated licensed package exports, opens, and renders audible output through standalone and plugin hosts.
- Both hosts retain the same authenticated BOM-free display text and report `View License` available through the production menu policy.
- Both hosts instantiate the shared viewer with exact selectable, multiline, read-only text, a Close action, accessibility metadata, and compact bounds.
- Plugin editor close/reopen preserves the immutable license pointer. Saved package-locator state restores the package, DSP graph, audible playback, license text, menu availability, and viewer before a new editor opens.
- The licensed host-validation run measured 14 ms standalone open, 13 ms plugin open, 907 microseconds standalone activation, 763 microseconds plugin activation, 42 microseconds synchronous `setStateInformation`, and 223 ms asynchronous locator restore. Both activations remain below the existing 16 ms message-thread ceiling.

## Realtime boundary

The viewer accepts only `shared_ptr<const string>` package-owned text and has no file or package path API. Package authentication and UTF-8 validation complete before activation; viewer construction and selection occur on the message thread.

After viewer assertions and licensed playback, standalone, plugin, and locator-restored processors each reported zero guarded audio-thread allocations, deallocations, file opens, file reads, path resolutions, decodes, locks, waits, or final shared-ownership releases. Audible output and callback budgets remain unchanged.

## Release artifact

The release gate exported a 714,614-byte package containing 18 records. Its canonical license evidence was:

- Payload id `license-text`, logical path `LICENSE.txt`, and media type `text/plain; charset=utf-8`.
- One 32-byte authenticated record with byte-identical content.
- Package schema remained at the legacy-compatible version; the optional license caused no schema promotion.
- Reported package bytes matched filesystem bytes exactly.
- Flipping one sealed license-record byte caused a `payload-corruption` authentication rejection before activation.
- Standalone and plugin rendered identical magnitude (`0.1401476264`), exposed the menu item, passed viewer component assertions, preserved license ownership through plugin editor reopen, and reported zero realtime violations.

Existing package-v1 compatibility fixtures, packages without licenses, missing/wrong-kind/oversized/invalid-UTF-8 declarations, cancellation cleanup, and authenticated tampering remain covered by the package and export-lifecycle suites.

## Verification

The following 15-test Debug matrix passed with zero failures:

- Phase-0 smoke, package-v2 records, the dedicated realtime guard, and realtime safety.
- Conventional package, package loader, package session, export lifecycle, host validation, and release gate.
- License contract, project import, and viewer tests.
- UI responsiveness baseline and Performance responsiveness.

The Debug standalone application and production VST3 bundle targets also built successfully. The LI-01 direct seams `activation-ownership`, `plugin-menu-viewer`, and `standalone-menu-viewer` remain implemented and backed by registered behavioral coverage.
