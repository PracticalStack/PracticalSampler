# Phase 3: Author's Diary

Phase 3 is the author's diary - a place to capture high-level ideas, integration patterns, domain insights, and real-world conventions in free-form prose. Phase 4a authoring agents read these files (injected into their prompt) and incorporate unique insights into the final user-facing documentation.

Phase 3 content is written from a purpose-driven perspective ("what is this for, how do you use it") rather than the implementation perspective of Phase 1/2. This makes it especially valuable as source material for user-facing docs.

---

## Source Directory

```
enrichment/phase3/ClassName/Readme.md        # Class-level diary
enrichment/phase3/ClassName/methodName.md    # Method-level diary
```

---

## What Goes In

Free-form content written by the author. No structured format required:

- Conversational prose ("this is useful when...", "just create a reference...")
- Bullet points or incomplete thoughts
- Integration patterns ("prepend `/` for OSC addresses")
- Design rationale and historical context
- Workflow sequences ("stop clock before preset load")
- Code examples showing intended usage

The current 201 files were imported from the legacy HISE docs repository (docs.hise.dev) to preserve valuable explanations, integration patterns, and hand-written examples.

---

## What the Merge Script Extracts

The merge script (`api_enrich.py`) reads Phase 3 files and extracts two things mechanically:

### Code examples

Fenced code blocks are extracted and tagged `"source": "manual"`. They **replace** Phase 1/2 auto-extracted examples for the same method (hand-written examples are higher quality).

### Cross-references

Inline links matching the docs.hise.dev URL pattern are converted to canonical method references and merged with Phase 1/2 cross-references (deduplicated).

### Link conversion

| Input | Cross-reference added |
|---|---|
| `[Array.push](/scripting/scripting-api/array#push)` | `"Array.push"` |
| `[Buffer](/scripting/scripting-api/buffer)` | -- (class-level, no method) |
| `[text](/scriptnode/...)` | -- (non-API link) |
| `![](/images/...)` | -- (image reference) |

Link-to-class/method name resolution uses a case-insensitive lookup against the known classes and methods from the base JSON.

### Prose is NOT extracted

Phase 3 prose does not become `userDocs` or override any structured fields (brief, purpose, description, parameters, etc.). It is source material for Phase 4a agents, who read it, extract unique insights, and rewrite in tight technical style.

---

## Preserved and Stripped Elements

**Preserved** (in extracted content):
- Blockquotes (`> Note that...`) - contain valuable caveats
- Tables (`| Column | ... |`) - e.g. value mode tables
- Bold/italic and other inline formatting

**Stripped:**
- `#### Example` headings (code blocks are extracted separately)
- Image references (`![alt](/path)`)
- Non-API links (link markup removed, link text preserved)
- YAML frontmatter

---

## Soft Length Limit

Files exceeding 500 lines are flagged. The Phase 4a agent documents length handling in its decision log (e.g. "Phase 3 Readme is 650 lines - skimmed for patterns, found 3 insights incorporated").
