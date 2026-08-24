# Instrument Controls accessibility and host matrix checklist

This is the manual sign-off companion to the automated qualification. It is
intentionally explicit: a green unit test proves the component contract, but a
screen reader and physical MIDI device still need a human pass before release.

## Accessibility walkthrough

Run in both the VST3 editor and standalone shell with the Controls feature flag
enabled, first at the minimum supported size and then at 820×700.

1. Start on the **Mixer** surface and use Tab/Shift+Tab to traverse the surface
   buttons, filter, sort, assignments toggle, every value slider, CC selector,
   channel selector, Learn, clear, and restore actions.
2. Switch to **Parameters** and repeat the traversal for tuning, envelope,
   dynamics, and tone controls. Confirm the value is announced with its declared
   unit (dB, cents, seconds, percent, or generic normalized value).
3. Open **Assignments**. Confirm the drawer is full-height and one-column at
   compact size, and that every destination/source/status/action remains
   reachable without horizontal clipping.
4. Arm Learn, confirm the destination and cancellation instruction are
   announced, press Escape, and confirm focus returns to the Learn action.
5. Assign an occupied CC/channel source. Confirm the conflict is announced and
   both **Replace** and **Cancel** are reachable; verify Cancel restores the
   prior source and Replace commits one undoable transaction.
6. Repeat with a high-contrast theme and with reduced-motion preferences.

Automated evidence: `drs.instrument_controls.ui` explicitly requires keyboard
focus on every interactive child, creates JUCE accessibility handlers for every
interactive child, verifies titles/value interfaces, checks the 820×700,
1120×800, and compact layout captures, and exercises Learn, Escape,
reserved-CC rejection (64/120/123), Replace/Cancel behavior, and the visible
source-status transition from `Awaiting MIDI input` to the observed channel/CC
and value. Standalone uses JUCE's aggregate MIDI callback, so status is based on
observed source activity rather than an invented physical-device identity.

## Host/device matrix

Run the same imported/project fixture in the plugin and standalone shells.

| Host | Source | Expected result | Sign-off |
| --- | --- | --- | --- |
| VST3 | CC21 on channel 1 | Exact channel-2 binding is absent; audio remains at the unmodulated level | [ ] |
| VST3 | CC21 on channel 2 | Exact binding responds; CC0 reaches the minimum target | [ ] |
| Standalone | CC21 on channel 1 | Exact channel-2 binding is absent; source status remains meaningful | [ ] |
| Standalone | CC21 on channel 2 | Exact binding responds; CC0 reaches the minimum target | [ ] |
| Both | Unassigned/removed source | No stale assignment or unexpected audio change | [ ] |
| Both | CC64/120/123 | Reserved transport/safety semantics remain intact | [ ] |

Automated evidence: `drs.phase1.performance_package_host_validation` packages
an any-channel CC20 control and an exact-channel CC21 control, runs the matrix
in both plugin and standalone processors, verifies an unassigned CC22 and
reserved CC64/120/123 do not break playback, exercises 44.1→48→44.1 kHz
prepare/release cycles, and records channel-1 output `0.124907` versus
channel-2 matched output `0.0` with zero realtime violations and zero
blocking-lock attempts.

Manual sign-off owner: ____________________    Date: ____________________
