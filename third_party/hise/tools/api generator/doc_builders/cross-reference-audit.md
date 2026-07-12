# Cross-Reference Audit

**Purpose:** Guideline for an LLM-assisted audit that finds missing cross-references across the HISE documentation, proposes additions, and applies them to the correct source files.

**Usage:** Reference this file in your prompt, e.g.:
> "Please analyse the master effect modules for missing cross-references according to the guideline at `doc_builders/cross-reference-audit.md`"

---

## Canonical Link System

All cross-references use `$DOMAIN.Target$` tokens. See `style-guide/canonical-links.md` for the complete link token reference, warning/tip format, and common mistake format. Key domains:

`$API.ClassName.method$`, `$MODULES.ModuleId$`, `$UI.Components.Name$`, `$UI.FloatingTiles.Name$`, `$SN.factory.node$`, `$DOC.Section.Page$`

---

## Source Files by Domain

Each type of cross-reference has a specific source file where it must be written:

### API Scripting Reference

| Type | Source file | Format |
|------|-----------|--------|
| Method see-also | `enrichment/phase1/{ClassName}/methods.md` | `**Cross References:**` bullet list with backtick-wrapped tokens |
| In-prose links | `enrichment/phase4/auto/{ClassName}/*.md` or `phase4/manual/` | `[display text]($DOMAIN.Target$)` inline |
| Warnings | `enrichment/phase4/auto/{ClassName}/*.md` | `> [!Warning:Title] text` |
| Tips | `enrichment/phase4/auto/{ClassName}/Readme.md` | `> [!Tip:Title] text` |

### Module Reference

| Type | Source file | Format |
|------|-----------|--------|
| See-also | `module_enrichment/pages/{ModuleId}.md` | `**See also:** $DOMAIN.Target$ -- description` at end of file |
| In-prose links | `module_enrichment/pages/{ModuleId}.md` | `[display text]($DOMAIN.Target$)` inline in overview/notes |
| Warnings/Tips | `module_enrichment/pages/{ModuleId}.md` | `> [!Warning:Title] text` in Notes section |

### Architecture/Guide Docs

| Type | Source file | Format |
|------|-----------|--------|
| In-prose links | `content/v2/architecture/*.md`, `content/v2/guide/*.md` | `[display text]($DOMAIN.Target$)` inline |

---

## Audit Workflow

### Step 0: Generate Topology

Before scanning content, generate the cross-reference topology JSON:

```bash
python publish.py --topology cross_references.json
```

This scans all source files and produces a compact JSON with:
- Every page and its outgoing links (classified as `seeAlso` or `inlineLinks`)
- Bidirectional gaps (A links to B but B doesn't link back)
- Orphan pages (pages with no outgoing links)
- Statistics (total links, gaps, orphans)

Read this JSON first. It gives you the full graph without scanning hundreds of files. Focus your analysis on:
- **Bidirectional gaps** - the most actionable findings
- **Orphan pages** - pages that need outgoing links
- **Pages with only see-also but no inline links** - opportunities for in-prose enrichment

### Step 1: Scan Content

For the scope the user specified, read the actual source files to understand:
- What each page/method is about (its purpose and key concepts)
- What concepts it mentions but doesn't link to (the topology JSON shows existing links; you look for missing ones)

### Step 2: Propose

Present findings interactively, grouped by category:

#### A. Missing see-also links
A method or module mentions a related concept but has no cross-reference.

Example:
> `TransportHandler.setEnableGrid` discusses MidiPlayer sync but doesn't cross-reference `$API.MidiPlayer.setSyncToMasterClock$`.
> **Proposed:** Add `- \`$API.MidiPlayer.setSyncToMasterClock$\`` to the `**Cross References:**` section in `enrichment/phase1/TransportHandler/methods.md`.

#### B. Missing in-prose links
Body text mentions a class, method, or module by name without linking to it.

Example:
> Effect.md userDocs (phase4a) mentions "bypass state" in prose but doesn't link to `$API.Effect.isBypassed$`.
> **Proposed:** Change "the bypass state" to "[the bypass state]($API.Effect.isBypassed$)" in `enrichment/phase4/auto/Effect/Readme.md`.

#### C. Cross-domain opportunities
A page in one domain should reference content in another domain.

Example:
> The `ChildSynth` class overview discusses sound generators but doesn't link to the Sound Generators index page.
> **Proposed:** Add `[sound generator]($DOC.Architecture.ModuleTree$)` in the overview prose, and `$MODULES.SineSynth$` to the see-also.

#### D. Bidirectional gaps
Page A links to page B, but page B doesn't link back to page A.

Example:
> `GlobalModulatorContainer` links to `GlobalEnvelopeModulator`, but `GlobalEnvelopeModulator` doesn't link back.
> **Proposed:** Add `$MODULES.GlobalModulatorContainer$ -- source container for global modulation values` to the see-also in `module_enrichment/pages/GlobalEnvelopeModulator.md`.

#### E. Light authoring suggestions
The prose could be slightly improved to better explain a cross-domain connection.

Example:
> The Convolution module page mentions the AudioSampleProcessor interface but could briefly explain how to load impulse responses from script.
> **Proposed:** Add a sentence: "Use [AudioSampleProcessor.setFile]($API.AudioSampleProcessor.setFile$) to load impulse responses from script."

### Step 3: Review

Present each proposal to the user. Wait for approval before applying. The user may:
- **Accept** - apply the change
- **Reject** - skip it
- **Modify** - adjust the wording or target

### Step 4: Apply

For accepted proposals, edit the correct source file:
- Phase1 cross-refs: add/remove bullet items in `**Cross References:**` sections
- Phase4a prose: insert/modify `[text]($DOMAIN.Target$)` inline links
- Module pages: modify `**See also:**` line or insert prose links
- Architecture/guide: insert prose links

---

## Quality Criteria

- **Only propose non-obvious connections.** If a user reading the Effect class docs would naturally think "I should also check ChildSynth," that's a good cross-reference. If the connection requires domain expertise to see, it's an excellent cross-reference.
- **Don't pad.** An empty see-also is better than weak entries. Only propose links that genuinely help the reader.
- **Prefer bidirectional links.** If A references B, suggest B references A too.
- **Cross-domain links are highest value.** Connecting API methods to module pages to architecture docs is the whole point of this system.
- **Keep see-also entries to 3-6 per page.** More than that dilutes the signal.

---

## Validation

After applying changes, run the publish script to validate all new tokens:

```bash
python publish.py --dry-run
```

Any unresolved `$DOMAIN$` tokens will appear as ERROR messages. Fuzzy-matched tokens appear as WARN/INFO - these indicate typos in the canonical IDs that should be corrected in the source.
