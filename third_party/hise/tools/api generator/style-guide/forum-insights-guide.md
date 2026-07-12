# Forum Insights Workflow

Enriches documentation pages with real-world usage patterns, gotchas, best practices, and gap analysis from the HISE forum. Supports three enrichment domains: modules, scriptnode nodes, and scripting API classes.

For output formatting (Warning/Tip/CommonMistake blocks), see `canonical-links.md` sections 2-5.

---

## Quick Start

The primary interface is the `/update-forum` agent:

```
/update-forum --modules LFO AHDSR Delay
/update-forum --scriptnode fx.reverb core.oscillator
/update-forum --api ScriptPanel Graphics
```

This handles search, diff, fetch, and extraction in one pass. See the agent definition at `.opencode/agents/update-forum.md` for full details.

---

## Prerequisites

| Component | Location | Purpose |
|-----------|----------|---------|
| `forum-search.py` | `forum-search/forum-search.py` | Search, filter, clean, and combine forum data |
| `config.json` | `forum-search/forum_search/config.json` | Trusted posters, scoring weights, filter rules |
| `forum_scanned.json` | `forum-search/forum_search/forum_scanned.json` | Diff tracker (which topics are already processed per scope) |
| `/update-forum` | `.opencode/agents/update-forum.md` | Top-level orchestrator agent |
| `@extract-forum-insights` | `.opencode/agents/extract-forum-insights.md` | Sonnet subagent (insights, questions, examples modes) |
| `@verify-forum-claim` | `.opencode/agents/verify-forum-claim.md` | Haiku subagent for C++ verification |

All paths relative to `HISE/tools/api generator/` unless noted.

---

## Phase 1: Search & Fetch

### Automated (preferred)

The `update` command handles search, diff, fetch, and combine in one pass:

```bash
python forum-search.py update "LFO" --also "lfo modulation" "lfo tempo sync" --scope modules:LFO
```

This produces a single combined file at `forum_cache/modules_LFO.json`. The diff tracker (`forum_search/forum_scanned.json`) ensures re-runs only fetch new topics.

### Manual (for exploration)

```bash
python forum-search.py search "PresetBrowser" --also "preset browser" "preset browser css"
```

Returns a JSON topic list with signal scores and briefs (first ~50 words of OP). No-reply threads, excluded categories (Feature Requests, AI Discussion), and duplicates are pre-filtered.

Select 5-8 topics for deep dive. Prefer topics with signal score > 0.5, trusted poster participation, or high reply count.

```bash
python forum-search.py fetch 12756 13420 13480 13296 6672 12893
```

Individual topic files are cached in `forum_cache/topic_{tid}.json`. Posts are cleaned: HiseSnippets stripped, HTML to markdown, quotes summarised, posts truncated to 500 words, trusted posters sorted first.

---

## Phase 2: Insight Extraction

Invoke the `@extract-forum-insights` subagent (Sonnet):

```
@extract-forum-insights
TOPIC: PresetBrowser
FILE: HISE/tools/api generator/forum-search/forum_cache/processed/pb_fetch_output.txt
EXISTING: (none)
```

Returns a structured list of 5-15 insights:

```
1. [TYPE] Title
   SOURCE: tid XXXX, username
   SUMMARY: 1-2 sentences
   VERIFY: yes | no
   VERIFY_HINT: grep terms (only if VERIFY=yes)
```

Types: `[BUG]`, `[GOTCHA]`, `[PATTERN]`, `[WORKAROUND]`, `[API]`, `[CONTRADICTION]`.

---

## Phase 3: C++ Verification

For each insight with `VERIFY: yes`, invoke `@verify-forum-claim` (Haiku):

```
@verify-forum-claim
CLAIM: "obj.handle is the correct property for scrollbar thumb bounds, not obj.area"
SEARCH HINTS: grep drawScrollbar in HiseLookAndFeel or PresetBrowser LAF dispatch
CONTEXT: oskarsh, November 2022, forum tid 6672
```

Returns:

```
CLAIM: ...
VERDICT: confirmed | fixed | inconclusive
EVIDENCE: file_path:line - key code line
NOTES: 1-2 sentences if needed
```

**Skip verification for:** usage patterns (`[PATTERN]`), API design rationale from Christoph Hart, performance tips, HISEScript-only workarounds.

**Verdict handling:**
- `confirmed` - include as Warning, Tip, or CommonMistake
- `fixed` - skip, or note as "Fixed in recent versions"
- `inconclusive` - include with softer language

---

## Gap Enrichment (questions mode)

Used in Step 2b of the module enrichment pipeline and equivalent steps in scriptnode/API pipelines. Extracts user confusion points to enrich C++ exploration gap lists.

```
@extract-forum-insights
TOPIC: LFO Modulator
FILE: forum_cache/modules_LFO.json
MODE: questions
```

Output is a structured list of questions with `gap_type` tags (behaviour, edge_case, interaction, performance, workflow). The `/update-forum` agent writes these to `{domain}_enrichment/forum/gaps/{TargetId}.json`.

For unenriched targets, questions with `maps_to_existing_gap: null` become new gaps for C++ exploration. Questions mapping to existing gaps enrich the gap's context. Forum insights with `verify: true` are verified during the full C++ exploration pass - no separate verification needed.

---

## Code Example Extraction (API domain only)

For scripting API classes, forum posts contain HiseScript code examples. The pipeline:

1. Python extracts code fences from posts with upvotes > 0:
   ```bash
   python forum-search.py extract-code --scope api:ScriptPanel
   ```
2. LLM tags methods, rates quality, writes descriptions:
   ```
   @extract-forum-insights
   TOPIC: ScriptPanel
   FILE: forum_cache/api_ScriptPanel_code.json
   MODE: examples
   ```
3. Output is written to `enrichment/forum/examples/{ClassName}.json`

Forum examples go through the same validation pipeline as Phase 2 project-extracted examples.

---

## Backport Triage (Step 4b)

For already-enriched modules (~46 modules with completed exploration output), the backport workflow avoids re-running the full pipeline:

1. Run `/update-forum --modules {ModuleId}` to get gaps and insights
2. Read the existing reference page and exploration output
3. For each gap/insight, classify as:
   - **Covered** - already answered in the existing page
   - **Answerable** - answered by exploration output but not in page (add to page)
   - **Needs verification** - VERIFY=yes, not covered anywhere (run `@verify-forum-claim`)
4. Generate targeted edit instructions (not a full rewrite)

The triage step is a Sonnet task that reads the existing page and forum data, producing a list of specific additions. This is reusable - it can be re-run whenever new forum content is available.

---

## Query Strategies

| Domain | Primary term | Alternative terms | Feature-specific examples |
|--------|-------------|-------------------|--------------------------|
| UI components | `ScriptSlider` | `knob`, `slider LAF` | `slider filmstrip`, `knob value popup` |
| Floating tiles | `PresetBrowser` | `preset browser` | `preset browser tags`, `preset browser css` |
| Modules | `AHDSR` | `envelope attack` | `AHDSR retrigger`, `envelope curve` |
| Scriptnode | `control.pma` | `pma modulation` | `pma bipolar`, `pma multiply add` |
| Scripting API | `Engine.setUserPresetTagList` | `preset tags` | `tag list custom browser` |

For scriptnode, also search by DSP operation description (e.g., `filter cutoff modulation`).

---

## Worked Example: PresetBrowser

```bash
# Phase 1: 45 topics found, 8 filtered out
python forum-search.py search "PresetBrowser" --also "preset browser" "preset browser css"
# Selected 8 high-signal topics, fetched
python forum-search.py fetch 12756 13420 13480 13296 6672 12893 12894 12067
```

Phase 2: `@extract-forum-insights` returned 13 insights, 6 with `VERIFY: yes`.

Phase 3 example - verifying LAF+CSS coexistence claim:

```
@verify-forum-claim
CLAIM: "LAF and CSS can coexist on the same component with per-function granularity"
SEARCH HINTS: grep CALL_LAF or CombinedLaf in ScriptingGraphics.h
CONTEXT: Christoph Hart, June 2025, tid 12893
```

Verdict: **confirmed**. `CombinedLaf` dispatches per-function - script LAF takes precedence, CSS is the fallback.

| Step | Tokens |
|------|--------|
| Triage (read search JSON) | ~6k |
| Insight extraction (Sonnet) | ~18k |
| Verification, 6 claims (Haiku) | ~42k |
| **Total** | **~66k** |

---

## Output Locations

| Domain | Gaps | Insights | Examples |
|--------|------|----------|----------|
| Modules | `module_enrichment/forum/gaps/` | `module_enrichment/forum/insights/` | - |
| Scriptnode | `scriptnode_enrichment/forum/gaps/` | `scriptnode_enrichment/forum/insights/` | - |
| Scripting API | `enrichment/forum/gaps/` | `enrichment/forum/insights/` | `enrichment/forum/examples/` |

---

## Maintenance

- **Refresh trusted posters:** `python forum-search.py refresh-users --min-reputation 100`
- **Cache:** Delete `forum_cache/` to force fresh fetches. The diff tracker (`forum_scanned.json`) is separate and preserved.
- **Reset diff tracker:** Delete or edit `forum_search/forum_scanned.json` to re-process topics for a scope.
- **Config:** Edit `forum_search/config.json` for scoring weights, category exclusions, or default limits.
