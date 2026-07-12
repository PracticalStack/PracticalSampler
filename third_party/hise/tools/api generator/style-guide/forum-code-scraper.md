# Forum Code Scraper

Scrapes the HISE forum for code examples, triages them for quality, fixes/lints the code, and produces a dataset matching the `snippet_dataset.json` schema for vector database ingestion.

For HISEScript syntax rules (inline function, const var/local/reg, LAF callbacks), see `scripting-api/hisescript-rules.md`.
For general code example quality principles, see `scripting-api/code-examples.md`.

All paths are relative to `HISE/tools/api generator/`.

---

## Quick Start — Execute This Checklist

When asked to run this pipeline, follow these steps in order. Do not plan ahead — execute each step, confirm the output, then move to the next.

### 1. Run the crawl

Start with a small test run. Only scale up after the user confirms the output quality is good.

```bash
cd "HISE/tools/api generator"

# TEST RUN FIRST (always start here)
python3 forum-search/forum-search.py crawl-code --categories 7 --max-topics 200 \
    --output forum-search/forum_cache/forum_code_dataset.json

# FULL RUN in batches of 2000 (only after user approves test output)
# python3 forum-search/forum-search.py crawl-code --categories 7,2,5,15 --max-topics 2000
# For subsequent batches, add --resume to skip already-processed topics:
# python3 forum-search/forum-search.py crawl-code --categories 7,2,5,15 --max-topics 2000 --resume
```

Wait for it to finish. It will print a JSON summary with `code_blocks` count. Tell the user how many blocks were extracted.

### 2. Export the first batch

```bash
python3 forum-search/forum-search.py describe-code \
    --input forum-search/forum_cache/forum_code_dataset.json \
    --batch-size 20 --offset 0
```

This creates a JSONL file. Read it.

### 3. Triage each block

For each line in the batch JSONL, read the `code` and `context` fields. Apply the triage rules (see below). Write one response JSON line per block to `forum-search/forum_cache/responses.jsonl`.

**Process blocks one at a time.** After every 5 blocks, append the 5 response lines to the responses file. Do not wait until the batch is done.

### 4. Repeat for next batch

Run `describe-code` with `--offset 20`, `--offset 40`, etc. Process each batch the same way, appending to the same `responses.jsonl`. The script prints the next command.

### 5. Apply results

After all batches are processed:

```bash
python3 forum-search/forum-search.py apply-descriptions \
    --input forum-search/forum_cache/forum_code_dataset.json \
    --responses forum-search/forum_cache/responses.jsonl \
    --output forum-search/forum_cache/forum_code_final.json
```

Tell the user the stats (kept/fixed/rejected).

---

## Reference: Categories

| cid | Name | Notes |
|-----|------|-------|
| 7 | Scripting | Primary source for HISEScript examples |
| 2 | General Questions | Mixed, many code answers |
| 5 | Presets / Scripts / Ideas | Community shared scripts |
| 15 | ScriptNode | DSP/scriptnode examples |
| 8 | C++ Development | Third-party node examples |
| 14 | Faust Development | Faust DSP code |

## Reference: Mechanical Filters (applied by the crawl script)

- Post upvotes: author/expert role ≥ 1, everyone else ≥ 2
- Code block ≥ 3 lines
- No HiseSnippet base64 blobs
- Code block ≤ 5000 chars
- Must contain HISE API keywords OR have a `js`/`javascript` language hint
- Must not match non-HISE patterns (PHP, Python, raw C++ includes)

### Triage rules

#### REJECT if any of these apply:

1. **Stub code** — The code body is mostly placeholders: `// etc`, `// TODO`, `// Your ... goes here`, `{ ... }`, empty function bodies with no real logic.
2. **Trivially obvious** — The example is a single API call that adds nothing beyond what the API signature already says (e.g., `Table1.setSnapValues([0, 0.5, 1.0])`, `const x = Math.random() * 2 - 1`).
3. **Not HISE code** — PHP, Python, WordPress, generic JavaScript that has no HISE API usage, or C++ that isn't HISE third-party node / ScriptNode code.
4. **Bug reproduction** — The code exists to demonstrate a bug, not a useful pattern. Look for context clues like "this crashes", "is this a bug", "wrong behavior".
5. **Incomplete fragment** — Missing so much context that even with reasonable backfilling it can't become a coherent example (e.g., references 5+ undeclared variables with no way to infer their types).

#### FIX if any of these apply (but the code is otherwise useful):

1. **Missing declarations** — Code references variables that are obvious from context but undeclared. Add `const var` declarations at the top (e.g., `const var eh = Engine.createExpansionHandler();`).
2. **Bare paint routine** — Code is a paint routine body without the `setPaintRoutine(function(g) { ... })` wrapper. Wrap it.
3. **Bare LAF function** — Code is a LAF callback body without `laf.registerFunction("drawXxx", function(g, obj) { ... })`. Wrap it.
4. **Inconsistent indentation** — Mix of tabs and spaces, or inconsistent nesting depth. Normalize to tabs (HISE convention).
5. **Missing interface call** — Standalone snippet that should start with `Content.makeFrontInterface(w, h)` but doesn't. Add it.
6. **Syntax issues per hisescript-rules.md** — `var` inside inline functions (should be `local`), regular `function` used for control callbacks (should be `inline function`).

#### KEEP if:

- Code is self-contained, demonstrates a clear HISE pattern, uses consistent tab indentation, and all variables are declared or obtainable from the code itself.

### Response format

Write one JSON line per block to the response JSONL file.

All KEEP and FIX responses must include a `FEATURED:` line (yes/no). Mark as featured if the example is **exceptionally useful** — a complete, production-quality pattern that someone would want to browse without a specific search query. Use these heuristics:
- Upvotes ≥ 3 is a strong signal (but not automatic — trivial code with upvotes is still not featured)
- Complete self-contained systems (tooltip handler, zoom handler, preset browser styling) are featured
- Novel API usage that isn't covered by official snippets is featured
- Short utility one-offs and basic patterns are NOT featured even if well-written

**For KEEP:**
```json
{"index": 0, "response": "VERDICT: KEEP\nFEATURED: yes\nTITLE: Save User Preset with Validation\nDESCRIPTION: FileSystem.browse and Engine.saveUserPreset used to implement a save-as dialog for user presets. Validates the chosen path is within the UserPresets folder and inside a subfolder."}
```

**For FIX:**
```json
{"index": 10, "response": "VERDICT: FIX\nFEATURED: no\nTITLE: Flipped FFT Spectrum Display\nDESCRIPTION: Custom FFT visualization using Graphics.flip to mirror the spectrum path vertically with gradient fills and layered rendering.\nCODE:\nconst var Panel1 = Content.getComponent(\"Panel1\");\n\nPanel1.setPaintRoutine(function(g)\n{\n\tvar a = this.getLocalBounds(0);\n\tvar path2 = Content.createPath();\n\t// ... rest of fixed code with tabs\n});"}
```

**For REJECT:**
```json
{"index": 33, "response": "VERDICT: REJECT\nREASON: Trivial one-liner — basic Math.random arithmetic with no HISE-specific content."}
```

### Title rules

- 3-6 words naming the technique or feature, not the forum thread topic
- Match the style of existing snippets: "Knob with Modulation Scaling", "Horizontal Linear Slider LAF", "MIDI File Sort by Tag"
- Lead with the most specific noun (the API class or concept), not a generic verb

### Description rules

- 2-3 sentences explaining what the code does and when to use it
- Target audience: an embedding model doing cosine similarity search
- Front-load distinctive HISE API names and keywords in the first sentence
- Reference the specific API methods used (e.g., "FileSystem.browse", "Engine.createBroadcaster")
- No generic filler ("This is an example of...", "This code demonstrates...")
- Be direct: start with the API name or action, not the context

### Code fixing rules

When fixing code, follow `scripting-api/hisescript-rules.md` and `scripting-api/code-examples.md`:

- **Indentation**: tabs, not spaces. One tab per nesting level.
- **Declarations**: `const var` for component references and constants. `local` inside inline functions. `reg` for audio-thread variables.
- **Callbacks**: `inline function` for control callbacks, `setControlCallback`, `setMouseCallback`, `setKeyPressCallback`, etc. Plain `function` only for LAF callbacks and `setPaintRoutine`.
- **Completeness**: Add `Content.makeFrontInterface(600, 400)` if the example is a standalone snippet. Add component acquisition chains (`Content.getComponent(...)`) for referenced components.
- **Do not over-backfill**: Only add what's needed to make the example runnable. Don't add empty callback stubs (`function onNoteOn() {}` etc.) unless the example specifically uses them.

---

## Step 3: Apply Results

```bash
python3 forum-search/forum-search.py apply-descriptions \
    --input forum-search/forum_cache/forum_code_dataset.json \
    --responses forum-search/forum_cache/responses.jsonl
```

Output goes to `forum-search/code_examples/batch_NNN.json` automatically (auto-incrementing batch number). Each crawl session produces its own batch file. Override with `--output` if needed.

This:
- Parses each response line
- For KEEP: updates title and description
- For FIX: updates title, description, AND replaces code with the fixed version
- For REJECT: drops the block entirely
- Strips the `_meta` field from all remaining blocks
- Reports stats: `{kept: N, fixed: N, rejected: N, failed: N}`

### Output directory

Final batch files are stored in `forum-search/code_examples/` (tracked in git):
```
code_examples/
  batch_001.json   ← first 2000 topics
  batch_002.json   ← topics 2001-4000
  batch_003.json   ← topics 4001-6000
  ...
```

Each batch file is a standalone JSON array matching the snippet_dataset.json schema:

```json
{
  "title": "LLM-generated descriptive title",
  "category": "Forum",
  "tags": ["forum", "Scripting", "expert"],
  "description": "LLM-generated 2-3 sentence description",
  "code": "final code (original or fixed)",
  "url": "https://forum.hise.audio/topic/{tid}",
  "featured": true
}
```

The `featured` flag marks exceptionally useful examples for browsing without a search query.

---

## Incremental Updates and Batched Crawling

The crawl tracks state in `forum-search/forum_search/crawl_state.json`:
- `last_crawl` — date of the most recent crawl
- `categories` — per-category page progress (`last_page`, `next_page`)
- `processed_tids` — all topic IDs ever processed (used to skip on resume)

### Batched crawling through the full forum

The forum has ~10,000+ topics. Process in batches of 2000:

```bash
# First batch (pages 1-N until 2000 topics collected)
python3 forum-search/forum-search.py crawl-code --categories 7,2,5,15 --max-topics 2000

# Second batch (resumes from where the first left off, skips processed topics)
python3 forum-search/forum-search.py crawl-code --categories 7,2,5,15 --max-topics 2000 --resume

# Continue until crawl reports 0 new topics
```

Each batch: crawl → triage (describe-code + LLM) → apply-descriptions. Then resume.

### Periodic refresh (every few months)

NodeBB lists topics newest-first, so a fresh crawl (without `--resume`) naturally grabs recent topics. Already-processed topics are still skipped via `processed_tids`.

```bash
# Grab new topics since last crawl
python3 forum-search/forum-search.py crawl-code --categories 7,2,5,15 --max-topics 2000
```

The script saves the crawl date in `crawl_state.json`. Topics from the previous crawl won't be re-processed. Only genuinely new posts get triaged.
