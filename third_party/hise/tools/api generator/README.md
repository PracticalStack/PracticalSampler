# HISE API Generator

Extraction tooling that produces structured JSON data from the HISE C++ source code. The output serves three deployment targets:

- **[HISE MCP server](https://github.com/christoph-hart/hise_mcp_server)** -- LLM coding assistance for HiseScript development
- **Documentation website** -- docs.hise.dev API reference pages
- **HISE internal autocomplete** -- In-editor code completion and parameter hints

> **To regenerate `XmlApi.h`/`XmlApi.cpp`, run `batchCreateMeta.bat`.**
> This supersedes the old `batchCreate.bat` by running the full enriched pipeline in one step:
> Doxygen XML generation, Phase 0 parsing, merge, filter, and C++ binary blob generation.
> Prerequisites: Python 3.x and Doxygen on PATH.

## Extraction Processes

| Process | Output | Status | Guide |
|---------|--------|--------|-------|
| **Scripting API enrichment** | `enrichment/output/api_reference.json` | Active | [scripting-api-enrichment.md](doc_builders/scripting-api-enrichment.md) |
| **LAF function extraction** | `laf_style_guide.json` | Active | [laf-extraction.md](doc_builders/laf-extraction.md) |
| **Component properties** | TBD | Planned | [component-properties.md](doc_builders/component-properties.md) |
| **Module enrichment** | `module_enrichment/pages/` + `module_enrichment/resources/static/` | Active | [module-enrichment.md](doc_builders/module-enrichment.md) |


## Directory Structure

```
├── batchCreateMeta.bat        # Full pipeline: Doxygen + enrich + binary blob
├── batchCreate.bat            # Step 1 only: Doxygen XML generation
├── api_enrich.py              # CLI tool: phase0, prepare, merge, preview, filter-binary
├── snippet_validator.py       # Validates HISEScript examples against HISE REST API
├── ApiValueTreeBuilder.exe    # Converts filtered JSON into C++ binary blob
├── ApiValueTreeBuilder/       # JUCE console app source for the above
├── xml.doxyfile               # Doxygen configuration
├── xml/                       # Doxygen XML output (gitignored, regenerated)
├── style-guide/
│   ├── general.md             # General writing style (all pipelines)
│   ├── canonical-links.md     # Cross-reference link tokens, warnings, tips
│   └── scripting-api/         # Scripting API-specific style guides
│       ├── userdocs-style.md  # userDocs prose rules
│       ├── code-examples.md   # Code example quality guidelines
│       ├── hisescript-rules.md # HISEScript syntax rules
│       ├── diagrams.md        # Diagram creation guidelines
│       ├── test-metadata.md   # Test metadata authoring
│       └── builder-reference.md # Builder API reference
├── enrichment/
│   ├── base/                  # Phase 0 output (gitignored, regenerated)
│   ├── phase1/                # Phase 1 agent output (tracked)
│   ├── phase2/                # Phase 2 project overrides (tracked)
│   ├── phase3/                # Phase 3 manual overrides (tracked)
│   ├── phase4/
│   │   ├── auto/              # Phase 4a LLM-generated userDocs (tracked)
│   │   └── manual/            # Phase 4a human-edited overrides (tracked, wins over auto)
│   ├── phase4b/               # Phase 4b LLM C++ reference entries (tracked)
│   ├── resources/             # Supporting data: explorations, survey
│   ├── issues.md              # Bugs discovered during C++ source analysis
│   ├── phase1_scanned.txt     # Diff manifest
│   └── output/                # Final merged JSON (gitignored, regenerated)
├── doc_builders/
│   ├── scripting-api-enrichment.md        # Orchestrator guide
│   ├── scripting-api-enrichment/          # Sub-phase details
│   │   ├── phase0.md
│   │   ├── phase1.md
│   │   ├── phase2.md
│   │   ├── phase3.md
│   │   ├── phase4.md
│   │   └── phase4b.md
│   ├── module-enrichment.md               # Module enrichment orchestrator guide
│   ├── module-enrichment/                 # Module enrichment sub-phase details
│   │   ├── intermediate-format.md         # Signal flow JSON specification
│   ├── laf-extraction.md
│   ├── component-properties.md
│   └── module-list.md
├── module_enrichment/                     # Module enrichment work product
│   ├── issues.md                          # Bugs discovered during C++ signal flow exploration
│   ├── base/                              # Phase 0 output (tracked)
│   │   └── moduleList.json
│   ├── phase1/                            # Phase 1 intermediate JSONs (tracked)
│   ├── phase2a/                           # Phase 2a usage summaries (tracked)
│   ├── phase2b/                           # Phase 2b documentation extracts (tracked)
│   ├── phase3/                            # Phase 3 enriched output (tracked)
│   ├── resources/                         # Supporting data and guidelines
│   └── output/                            # Final merged JSON (gitignored)
```

## Quick Start (Scripting API Enrichment)

Prerequisites: Python 3.x, Doxygen on PATH, HISE source tree at `../../` relative to this directory.

### Regenerate XmlApi.h / XmlApi.cpp (full pipeline)

```bash
batchCreateMeta.bat
```

### Individual steps (for partial runs or debugging)

```bash
# 1. Regenerate Doxygen XML + parse into base JSON
batchCreate.bat > NUL 2>&1
python api_enrich.py phase0

# 2. Check what still needs Phase 1 processing
python api_enrich.py prepare

# 3. (Phase 1 is agent-driven -- see Session Prompts below)

# 4. Merge all phases into final output
python api_enrich.py merge

# 5. Preview HTML pages (review + web if userDocs exist)
python api_enrich.py preview Console

# 6. Generate binary blob
python api_enrich.py filter-binary
ApiValueTreeBuilder.exe enrichment\output\filtered_api.json "..\..\hi_scripting\scripting\api" XmlApi
```

## Adding New API Classes

When a new class is added to the HISE scripting API:

1. **Add Doxygen comments** -- Add `/** */` doc comments to the public methods in the C++ class header
2. **Update `batchCreate.bat`** -- Add two lines:
   - An `xcopy` line to copy the Doxygen-generated XML file into `xml\selection` (follow the naming pattern: `classhise_1_1_...` for classes in the `hise` namespace, `structhise_1_1_...` for structs)
   - A `ren` line to rename it to the friendly class name -- this must exactly match the string returned by `Identifier Class::getObjectName()` (e.g., `Console.xml`)
3. **Update `api_enrich.py`** -- Add the class to the `CATEGORY_MAP` dict with the correct category: `namespace`, `object`, `component`, or `scriptnode`
4. **Run Phase 0** -- `batchCreate.bat > NUL 2>&1 && python api_enrich.py phase0`
5. **Run Phase 1** -- Agent-driven C++ source analysis (see Session Prompts below)
6. **Merge** -- `python api_enrich.py merge`

## Session Prompts

Copy-paste these into an agent session to run the enrichment pipeline:

### Single class (most common)

```
Follow tools/api generator/doc_builders/scripting-api-enrichment.md. Run phase0, then run phase1 for Console.
```

### Multiple classes

```
Follow tools/api generator/doc_builders/scripting-api-enrichment.md. Run phase0, then run phase1 for Console and Engine.
```

### Resume interrupted class

```
Follow tools/api generator/doc_builders/scripting-api-enrichment.md. Resume phase1 for Broadcaster.
```

### Full automation

```
Follow tools/api generator/doc_builders/scripting-api-enrichment.md. Run the full pipeline for all unscanned classes.
```

### Post-process only

```
Follow tools/api generator/doc_builders/scripting-api-enrichment.md. Run post-process for Broadcaster.
```

### Merge only

```
Follow tools/api generator/doc_builders/scripting-api-enrichment.md. Run merge.
```

### Phase 4 userDocs authoring (single class)

```
Follow tools/api generator/doc_builders/scripting-api-enrichment/phase4.md. Write userDocs for Console.
```

### Preview

```
Follow tools/api generator/doc_builders/scripting-api-enrichment.md. Run merge and preview Console.
```

## What Gets Tracked vs Regenerated

**Tracked** (human/agent work product):
- `enrichment/phase1/` -- Agent-produced class analyses and method docs
- `enrichment/phase2/` -- Project example overrides
- `enrichment/phase3/` -- Manual markdown overrides
- `enrichment/phase4/auto/` -- LLM-generated user-facing docs
- `enrichment/phase4/manual/` -- Human-edited user-facing doc overrides
- `enrichment/phase1_scanned.txt` -- Progress manifest

**Gitignored** (regenerated by tools):
- `xml/` -- Doxygen XML output
- `enrichment/base/` -- Phase 0 JSON (from `api_enrich.py phase0`)
- `enrichment/output/` -- Final `api_reference.json` (from `api_enrich.py merge`)
