# Forum Snippet Scraper

Scrapes HiseSnippet base64 strings from the HISE forum, decodes them to extract Interface scripts, and produces code examples for vector database ingestion. Parallel pipeline to `forum-code-scraper.md` — targets embedded project snapshots instead of code fences.

For HISEScript syntax rules, see `scripting-api/hisescript-rules.md`.
For general code example quality principles, see `scripting-api/code-examples.md`.

All paths are relative to `HISE/tools/api generator/`.

---

## Quick Start — Execute This Checklist

When asked to run this pipeline, follow these steps in order. Do not plan ahead — execute each step, confirm the output, then move to the next.

### 1. Run the crawl

```bash
cd "HISE/tools/api generator"

# TEST RUN FIRST (always start here)
python3 forum-search/forum-search.py crawl-snippets --categories 7 --max-topics 200

# FULL RUN in batches of 2000 (only after user approves test output)
# python3 forum-search/forum-search.py crawl-snippets --categories 7,2,5,15 --max-topics 2000
# For subsequent batches:
# python3 forum-search/forum-search.py crawl-snippets --categories 7,2,5,15 --max-topics 2000 --resume
```

Wait for it to finish. It will print a JSON summary with `snippet_scripts` count. Tell the user how many snippets were extracted.

### 2. Export the first batch

```bash
python3 forum-search/forum-search.py describe-code \
    --input forum-search/forum_cache/snippet_dataset_raw.json \
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
    --input forum-search/forum_cache/snippet_dataset_raw.json \
    --responses forum-search/forum_cache/responses.jsonl \
    --snippet
```

The `--snippet` flag outputs to `forum-search/code_examples/snippet_batch_NNN.json` (auto-incrementing). Tell the user the stats (kept/fixed/rejected).

---

## How HiseSnippets Differ from Code Fences

HiseSnippets are base64-encoded HISE project snapshots (JUCE ValueTree → zlib → custom base64). The crawl:
1. Finds `HiseSnippet \d+\.[A-Za-z0-9.+]+` strings in raw HTML posts
2. Decodes using JUCE's custom base64 alphabet (`.ABCDEFG...` not standard RFC 4648)
3. Decompresses with zlib
4. Extracts the Interface script from the binary ValueTree
5. **Strips all callback functions** (`onNoteOn`, `onNoteOff`, `onController`, `onTimer`, `onControl`)

The result is the `onInit` script content — what users paste into their Interface script.

---

## Triage Rules

Same REJECT/FIX/KEEP rules as `forum-code-scraper.md`, plus these snippet-specific rules:

### Additional REJECT rules:

1. **Module-only snippet** — The extracted script is just `Content.makeFrontInterface(w, h)` with no other meaningful code. This means the snippet demonstrates module configuration (routing, modulator setup), not scripting.
2. **Code in callbacks** — Already filtered by the crawl. If a snippet had actual code in `onNoteOn()`, `onNoteOff()`, `onController()`, `onTimer()`, or `onControl()`, it was rejected during extraction. This is because callback code can't be copy-pasted into an interface script.
3. **Decode failure** — Corrupt or truncated snippet string.

### Response format

Same as `forum-code-scraper.md`:

**For KEEP:**
```json
{"index": 0, "response": "VERDICT: KEEP\nFEATURED: yes\nTITLE: Custom Value Popup with Broadcaster\nDESCRIPTION: Engine.createBroadcaster with attachToComponentMouseEvents to show a positioned label as a custom value popup on knob drag."}
```

**For FIX:**
```json
{"index": 3, "response": "VERDICT: FIX\nFEATURED: no\nTITLE: Arc Drawing on Panel\nDESCRIPTION: Content.createPath with addArc to draw partial arcs on a panel paint routine.\nCODE:\nContent.makeFrontInterface(600, 600);\n\nconst var Panel1 = Content.getComponent(\"Panel1\");\n// ... fixed code ..."}
```

**For REJECT:**
```json
{"index": 5, "response": "VERDICT: REJECT\nREASON: Module-only snippet — no scripting beyond Content.makeFrontInterface."}
```

### Title, description, code fixing, and featured rules

Identical to `forum-code-scraper.md`. See that file for the full reference.

---

## Output Directory

Final batch files are stored in `forum-search/code_examples/` (tracked in git):
```
code_examples/
  batch_001.json            ← code fence examples
  batch_002.json
  snippet_batch_001.json    ← HiseSnippet examples
  snippet_batch_002.json
  ...
```

Each file is a standalone JSON array with the same schema:

```json
{
  "title": "LLM-generated descriptive title",
  "category": "Forum",
  "tags": ["forum", "Scripting", "trusted"],
  "description": "LLM-generated 2-3 sentence description",
  "code": "final code (original or fixed)",
  "url": "https://forum.hise.audio/topic/{tid}",
  "featured": true
}
```

---

## Incremental Updates

State is tracked separately from the code scraper in `crawl_state.json`:
- `snippet_processed_tids` — topic IDs processed by this scraper
- `categories.snip_{cid}` — per-category page progress

```bash
# Next batch (resumes from last position)
python3 forum-search/forum-search.py crawl-snippets --categories 7,2,5,15 --max-topics 2000 --resume
```
