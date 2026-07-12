<!-- Diagram triage:
  - No diagrams specified in Phase 1 data.
-->

# Date

Date provides wall-clock time access and bidirectional conversion between millisecond timestamps and ISO-8601 formatted date strings. Use it to stamp events with the current system time, compare dates arithmetically using their millisecond values, or format timestamps for display and storage.

All string representations use the local timezone. The `includeDividerCharacters` parameter on the formatting methods controls whether the output includes dashes, colons, and the `T` separator (e.g. `2026-03-09T14:30:00+0100`) or omits them for a compact form (e.g. `20260309T143000+0100`).

> [!Tip:Returns wall-clock time, not DAW transport] These methods return the operating system's wall-clock time, not the DAW transport position. For transport-relative timing, use `TransportHandler` instead.
