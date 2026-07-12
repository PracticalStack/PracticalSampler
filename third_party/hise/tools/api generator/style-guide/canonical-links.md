# Canonical Links, Warnings, Tips, and Common Mistakes

**Single source of truth** for cross-reference syntax, warning/tip formatting, and common mistake structure across all HISE documentation pipelines.

All markdown source files (scripting API enrichment, module reference pages, architecture docs) use the formats described here. The `publish.py` script resolves link tokens and converts everything to Nuxt.js MDC components.

---

## 1. Cross-Reference Links

### Link token syntax

Use `$DOMAIN.Target#fragment$` tokens. These are resolved by `publish.py` to final URLs with fuzzy matching and validation.

| Domain | Target format | Example |
|--------|--------------|---------|
| `$API.ClassName$` | Scripting API class | `$API.Effect$` |
| `$API.ClassName.method$` | Scripting API method | `$API.Effect.setBypassed$` |
| `$MODULES.ModuleId$` | Audio module (ID from moduleList.json) | `$MODULES.Convolution$` |
| `$MODULES.ModuleId.ParamId$` | Module parameter | `$MODULES.AHDSR.Attack$` |
| `$UI.Components.Name$` | UI component reference | `$UI.Components.ScriptSlider$` |
| `$UI.FloatingTiles.Name$` | FloatingTile reference | `$UI.FloatingTiles.PresetBrowser$` |
| `$SN.factory.node$` | Scriptnode node | `$SN.math.add$` |
| `$LANG.language$` | Language reference page | `$LANG.snex$`, `$LANG.hisescript$` |
| `$DOC.Section.Page$` | Manual/architecture docs | `$DOC.Architecture.ModuleTree$` |
| `$FORUM_REF.tid$` | Forum topic by topic ID (page-local) | `$FORUM_REF.2054$` |

**Note:** `$FORUM_REF$` is page-local — it resolves directly to `https://forum.hise.audio/topic/{tid}` via simple regex substitution, bypassing the registry cascade below. Used as inline citations: `[1]($FORUM_REF.2054$)`.

### Resolution cascade

The publish script resolves tokens in this order:

1. **Exact match** - silent
2. **Case-insensitive** - INFO message (e.g., `$API.transporthandler$` -> `TransportHandler`)
3. **Normalized** (strip dashes/underscores) - INFO message (e.g., `$DOC.Architecture.DataModel$` matches `data-model`)
4. **Fuzzy** (difflib, >0.6 similarity) - WARN message (e.g., `$MODULES.PhaseFX$` -> `Phaser`)
5. **No match** - ERROR (fails build in `--strict` mode)

### In-prose links

Use standard markdown link syntax with tokens as the URL:

```markdown
This module works with the [Global Envelope Modulator]($MODULES.GlobalEnvelopeModulator$) to provide...

Use [Effect.setBypassed]($API.Effect.setBypassed$) to toggle the effect from script.

See the [module tree architecture]($DOC.Architecture.ModuleTree$) for an overview.
```

### See-also references

Use the `**See also:**` format. Entries are comma-separated, with optional `--` descriptions:

```markdown
**See also:** $API.Effect.setBypassed$ -- scripting API for bypass, $MODULES.Convolution$ -- convolution reverb module
```

The publish script converts this to `::see-also` MDC components with proper URLs.

### Where to write cross-references

| Content domain | Source file | Format |
|---------------|-----------|--------|
| API method see-also | `enrichment/phase1/{ClassName}/methods.md` | `**Cross References:**` bullet list: `- \`$API.Target$\`` |
| API in-prose links | `enrichment/phase4/auto/{ClassName}/*.md` | `[text]($DOMAIN.Target$)` inline |
| Module see-also | `module_enrichment/pages/{ModuleId}.md` | `**See also:** $DOMAIN.Target$ -- desc` |
| Module in-prose links | `module_enrichment/pages/{ModuleId}.md` | `[text]($DOMAIN.Target$)` inline |
| Architecture/guide | `content/v2/architecture/*.md`, `content/v2/guide/*.md` | `[text]($DOMAIN.Target$)` inline |

---

## 2. Warnings

Warnings highlight common pitfalls or non-obvious gotchas. Use the titled blockquote format:

```markdown
> [!Warning:Enable grid before registering callbacks] The grid must be enabled via setEnableGrid() before grid callbacks fire. Without it, the callback is registered but never triggered.
```

**Title guidelines:**
- 3-8 words, action-oriented
- Describes the problem or the correct action concisely
- Examples: "Prefetch component references", "Use inline functions for sync callbacks", "Stop clock before preset load"

The publish script converts this to:

```markdown
::warning{title="Enable grid before registering callbacks"}
The grid must be enabled via setEnableGrid() before grid callbacks fire.
::
```

**Legacy format** (`> **Warning:** text`) is still supported but produces untitled warnings.

**Placeholder:** During backport migration, untitled warnings use `> [!Warning:$WARNING_TO_BE_REPLACED$] text`. The publish script emits an INFO message for these so they can be found and titled.

---

## 3. Tips

Tips highlight best practices or useful shortcuts. Same syntax as warnings:

```markdown
> [!Tip:Store references in onInit] Effect references can only be obtained during onInit via Synth.getEffect(). Store them as const var at the top level.
```

Converts to `::tip{title="Store references in onInit"}`.

**Legacy format** (plain `> blockquote` before `## Methods`) is still supported but produces untitled tips.

**Placeholder:** `> [!Tip:$TIP_TO_BE_REPLACED$] text` for backport migration.

---

## 4. Common Mistakes

Common mistakes appear in class-level Readme.md files (phase4a for API, frontmatter for modules). Each entry has a title, wrong pattern, right pattern, and explanation:

```markdown
## Common Mistakes

- **Prefetch component references**
  **Wrong:** `const var fx = Synth.getEffect("MyFX");` in `onNoteOn`
  **Right:** `const var fx = Synth.getEffect("MyFX");` in `onInit`
  *Synth.getEffect() can only be called during initialisation. Store the reference as a top-level const.*

- **Use named constants for parameters**
  **Wrong:** `fx.setAttribute(0, 1000.0)` using raw integer indices
  **Right:** `fx.setAttribute(fx.Frequency, 1000.0)` using named constants
  *Named constants are generated per effect type. They make code self-documenting and survive parameter reordering.*
```

The publish script converts this to `::common-mistakes` MDC with a `title` field per entry.

**Placeholder:** During backport, untitled entries use `**$COMMON_MISTAKE_TITLE_TO_BE_REPLACED$**` as the title line.

---

## 5. Forum Community Insights

Forum insights use the same Warning, Tip, and CommonMistake formats defined in sections 2-4 above. The full workflow for gathering and verifying forum insights is in `style-guide/forum-insights-guide.md`.

### Output format

| Insight type | Block format |
|-------------|-------------|
| Pitfalls, gotchas | `> [!Warning:title]` (see section 2) |
| Best practices | `> [!Tip:title]` (see section 3) |
| Wrong/right patterns | `commonMistakes` YAML (see section 4) |

### Placement

- Scatter blocks throughout the page adjacent to the relevant property or code example
- Maximum one styled block per page section
- Never break a markdown table to insert a block. Place blocks before or after the complete table.
- Target 3-5 blocks per page, roughly even Warning/Tip balance

---

## Validation

Run the publish script to validate all tokens:

```bash
python publish.py --dry-run          # default: warn on broken links
python publish.py --dry-run --strict # CI: fail on broken links
```

- Unresolved `$DOMAIN$` tokens appear as ERROR
- Fuzzy-corrected tokens appear as WARN/INFO
- Untitled warnings/tips/mistakes appear as INFO
