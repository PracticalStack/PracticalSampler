# Structure Viewer authoring guidance

The Authoring mapping header contains peer `Map` and `Structure` views. Use Map for spatial range editing; use Structure when equal or overlapping ranges need independent rows.

Structure selection is same-type and stable-ID based. Select a layer to scope Groups, a group to scope Zones, and a zone to expose its context inspector. Control/Command-click toggles same-type peers; Shift extends a visible-order range. Double-click a zone row to reveal it in Map.

The inspector displays shared values for a multi-selection. `Mixed` means the selected objects differ; typing an absolute value applies it to every selected object in one transaction. Gain and pan nudge buttons apply relative deltas. Invalid or partially missing target sets are rejected atomically.

Diagnostics are descriptive, not validation errors. Use the diagnostic and context filters to find key overlaps, exact stacks, potential collisions, articulation, and performance-event variants. Read the tooltip/reason text; color is supplementary only.

Map/Structure mode, filters, sorting, column widths, and scroll position are workspace state. They do not dirty or serialize the authored instrument.
