# SFZ Region Authoring in the Waveform Workspace

## What this feature does

Practical Sampler imports the supported SFZ region and loop settings into its native project model. The Waveform workspace can then edit those settings without changing, trimming, normalizing, or replacing the source audio file.

This is a one-way SFZ conversion workflow. Practical Sampler projects and playable packages may contain DSP, macros, routing, and native loop crossfades that SFZ cannot represent, so there is no **Export as SFZ** command.

## Creator workflow

1. Select a zone and open **Waveform** in the Authoring workspace.
2. Drag across the waveform to make a temporary selection. Use **Use for Playback**, **Use for Loop**, or **Play Selection** to act on it.
3. Choose **Loop Off**, **While Held**, or **Always**, then set the loop start/end. One-shot behavior remains a Trigger setting rather than a loop choice. Imported zones may have a loop start before playback start; playback begins at the authored playback start, reaches loop end, then wraps to the earlier loop start.
4. Optionally set a native loop crossfade. This is a Practical Sampler enhancement, not a portable SFZ opcode.
5. Press Return or leave a numeric field to commit it. Boundary drags commit on pointer release. One completed action creates one undo step; transient selections and canceled gestures do not dirty the project.
6. Use **Play Loop** to begin shortly before the loop end, so the authored crossfade and wrap are heard immediately. The action changes to **Release Loop** while the note is held. For **While Held** loops, releasing exits the loop and continues through the authored post-loop tail; **Always** loops continue wrapping while their release envelope fades. **Stop** remains available for an immediate Preview stop. Then Publish when the draft is ready. Saving, publishing, and package export preserve the source file unchanged.

Numeric fields accept source frames or seconds using an `s` suffix. End values shown by Practical Sampler are exclusive; they identify the first frame after the playable range.

## Final loop invariant

Enabled forward loops use the native half-open range `[loopStartFrame, loopEndFrame)` and must satisfy:

```text
loopStartFrame < loopEndFrame
loopStartFrame <= playbackStartFrame < loopEndFrame
loopEndFrame <= playbackEndFrame
```

The loop head may therefore precede playback start. Playback still begins at the authored `offset`/`playbackStartFrame`; it does not jump backward until playback reaches `loopEndFrame`. The waveform editor, Preview, Publish, package loader, prepared playback, render model, and voice kernel preserve this relationship.

## Keyboard reference

| Input | Action |
|---|---|
| Drag | Create a temporary waveform selection. |
| Space-drag or middle-drag | Pan the visible waveform. |
| Tab / Shift+Tab | Move through playback, loop, selection, snap, and audition controls. |
| Return or leave field | Apply the value in a focused playback, loop, or crossfade field. |
| Left / Right on a selected boundary | Move that boundary by one source frame and commit one undoable edit. |
| Left / Right with no boundary selected | Pan the visible waveform by one tenth of the viewport. |
| `+` or `=` / `-` | Zoom in / out around the viewport center. |
| Home or `0` | Fit the whole source. |
| Escape | Cancel an active pointer gesture without committing it. |

All boundary editors expose accessibility titles, descriptions, focus order, enabled state, and values independently of the painted waveform.

## SFZ region coverage

| SFZ input | Conversion status | Practical Sampler behavior |
|---|---|---|
| `offset` | Exact | Becomes `sampleStartFrame`. |
| `end` | Normalized | SFZ's inclusive final frame is converted once to native exclusive `sampleEndFrame`; omission retains physical source end. |
| `loop_mode=no_loop` | Exact | Loop disabled. |
| `loop_mode=one_shot` | Exact | One-shot trigger behavior with no repeating loop. |
| `loop_mode=loop_continuous` | Exact | Continuous forward loop. |
| `loop_mode=loop_sustain` | Exact | Forward loop released by note-off. |
| `loop_start` | Exact | Becomes the native loop start frame, including when it precedes playback start. |
| `loop_end` | Normalized | SFZ's inclusive loop end is converted once to the native exclusive loop end. |
| Embedded WAV `smpl` loop | Fallback | Supplies omitted loop boundaries; explicit SFZ values win. |
| `<global>` / `<master>` / `<group>` inheritance | Resolved | Effective values are resolved before the region is projected. |
| Missing source frame count | Partial | Import retains a finding when an inclusive end cannot be safely normalized. |
| Malformed or unsupported loop mode | Unsupported | Import records an explicit finding instead of silently claiming parity. |
| Alternate/reverse loop types, finite loop counts, modulated offsets, implementation-specific SFZ v2 region behavior | Unsupported | Not converted by the portable v1 region baseline. |
| Native `loopCrossfadeFrames` | Practical Sampler only | Persisted in native projects/packages and rendered with an equal-power loop blend; never represented as SFZ. |

## Migration and compatibility

- Projects predating playback ends continue to use the physical source end through the historical zero sentinel.
- Playback-region projects migrate to project/authoring schema 8/7 without changing their host binding when all new fields retain legacy defaults.
- Authoring a native loop crossfade promotes the project to schema 9/8. Compiled instruments use schema 7 for the complete SFZ region contract and schema 8 when a native crossfade is present.
- Older readable package and project schemas remain supported. Invalid, collapsed, or reversed ranges; loops that end at or before playback start; loop ends beyond playback end; unresolved active-loop boundaries; unsupported loop modes; and crossfades larger than half the loop are rejected or reported as unsupported rather than repaired silently at playback time.
- Host state, save/reopen, Preview, Publish, and playable-package open all use the same typed region fields.

## Large sources

The waveform viewer requests visible-range peak tiles rather than decoding a whole long file. Tile memory is bounded and stale zoom/source generations cannot replace the newest result. Playback preparation retains bounded source heads and now schedules deduplicated page prewarm for deep playback starts, loop heads, crossfade tails, and loop-wrap frames off the message and audio threads.

If a source is replaced at the same path, its size and modification identity invalidate stale waveform tiles and its fingerprint invalidates stale prepared playback. Practical Sampler never writes region edits back into that source.
