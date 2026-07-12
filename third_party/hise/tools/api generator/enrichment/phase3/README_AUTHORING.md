# Phase 3 Authoring Guide

## Purpose

Phase 3 is your "no-filters diary" for API classes. Write high-level ideas, integration patterns, and domain insights without worrying about prose style or technical completeness. The Phase 4a authoring agent reads your notes (injected into its prompt) and incorporates the substance into polished documentation.

## What to Write

**Integration Patterns:**
```
GlobalCable: if the cable ID starts with `/`, it becomes an OSC address when you enable the OSC server
```

**Cross-System Interop:**
```
GlobalCable is the preferred way to send values from C++ scriptnode nodes back to the script
```

**Workflow Patterns:**
```
TransportHandler: always stop the internal clock before loading a preset, then call sendGridSyncOnNextCallback() and restart
```

**Domain Conventions:**
```
Broadcaster metadata: if you use /** comment instead of /*, it auto-populates the comment field from your code
```

**Design Rationale:**
```
The range conversion system exists because cables need to work with both 0..1 macro controls and arbitrary user-facing ranges
```

**Use Case Narratives:**
```
Most common use case: sync multiple MIDI players across different script processors. Create a cable, have each player send its playback position, listeners update UI.
```

## What NOT to Worry About

- **Prose style** - write conversationally, the agent tightens it
- **Completeness** - partial thoughts are fine, the agent fills gaps from Phase 1/2
- **Redundancy** - if you repeat Phase 1 facts, the agent deduplicates
- **Filler words** - "just", "in order to", "super helpful" are fine, agent strips them

## Length Guideline

**Soft limit: 500 lines**

If you write more, the agent will note it in the decision log: "Phase 3 Readme is 650 lines - skimmed for patterns, incorporated 3 unique insights."

This isn't a hard limit - write as much as you need. But if you're writing a novel, the agent may ask "did you really need all of this?" in the decision log.

## File Naming

**Class-level notes:**
```
enrichment/phase3/ClassName/Readme.md
```

**Method-level notes:**
```
enrichment/phase3/ClassName/methodName.md
```

(Use exact method name from Phase 1, case-insensitive matching)

## Code Examples

Hand-written code examples in Phase 3 **replace** auto-extracted ones from Phase 1/2. Write minimal, focused examples showing your intended usage:

```javascript
// GlobalCable as OSC address
const var grm = Engine.getGlobalRoutingManager();
const var oscCable = grm.getCable("/some_osc_id");  // `/` prefix makes it OSC

oscCable.registerCallback(function(value) {
    Console.print("Received from OSC: " + value);
});
```

The agent may supplement with Phase 2 real-world examples if they show additional complexity your example doesn't cover. Triage decisions are documented in the decision log.

## Markdown Links

If you write markdown links to other API methods, they become cross-references:

```markdown
See [MidiPlayer.setSyncToMasterClock](/scripting/scripting-api/midiplayer#synctomastertclock) for sync setup.
```

The agent extracts `MidiPlayer.setSyncToMasterClock` as a cross-reference and converts the link to a backtick reference in the prose.

## Examples of Good Phase 3 Entries

**Good (high-level pattern):**
```
ScriptPanel.setMouseCallback() - you can attach this to ANY component, not just panels. 
Useful for custom drag behavior on sliders without overriding their default UI.
```

**Good (integration detail):**
```
Server.callWithGET() - the callback is async, don't try to return values directly. 
Use a Broadcaster to notify other parts of your code when the response arrives.
```

**Good (workflow sequence):**
```
UserPresetHandler custom automation:
1. setUseCustomUserPresetModel(true)
2. attachAutomationCallback() with save/load logic  
3. setCustomAutomation() to populate initial state
Order matters - call them in this sequence.
```

**Less useful (just restates Phase 1):**
```
Synth.addNoteOn(noteNumber, velocity) sends a note-on event
```
(Phase 1 already extracted this from C++ - no unique insight)

## Editing Existing Phase 3 Files

The 201 imported files from docs.hise.dev are conversational prose. You can:
- Leave as-is (agent extracts useful bits)
- Add notes with additional patterns you've discovered
- Replace with cleaner diary entries if the prose is too verbose

Don't worry about cleaning up the conversational tone - that's the agent's job. If you see valuable content buried in fluff, just add a note: "Key insight: OSC addresses use `/` prefix" and the agent will find both.

## What Happens to Your Notes

1. **Phase 4a agent reads your file** (injected into its prompt when it runs)
2. **Agent extracts unique insights** not in Phase 1/2 (OSC patterns, C++ interop, workflows)
3. **Agent rewrites in tight technical style** (strips "in order to", "just", etc.)
4. **Agent documents decisions** in `output/decisions/{ClassName}_phase4a.md`:
   - "Incorporated OSC address pattern (/ prefix) from Phase 3 class diary"
   - "Omitted Phase 3 obtaining-the-object prose - redundant with Phase 1"
5. **Result becomes `userDocs`** in the JSON (published to docs.hise.dev)

You can review the decision log after each class to see what the agent did with your notes.
