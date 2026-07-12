# Language Reference Authoring Guideline

Checklist for writing and reviewing language reference pages in the `language_enrichment/output/` directory. Derived from the HiseScript and SNEX authoring sessions.

---

## 1. Introduction structure

- **Lead with the key constraint** — the single most important thing about the language (e.g. realtime safety for HiseScript, JIT sandbox for SNEX). One paragraph, no lists of specific features or variable types.
- **Three bullet points** for primary use cases or entry points. Keep abstract — no details, no forward references to specific sections.
- **`**See also:**` box** linking to the applied/practical section of the same doc, using the canonical `**See also:**` format (converted to `::see-also` MDC by `publish.py`).

## 2. Section naming

| Section | Purpose |
|---------|---------|
| **The Language** / **Syntax** (or equivalent) | Pure language reference — syntax, types, control flow, functions |
| **Usage in [context]** / applied section | How the language connects to its host environment (callbacks, APIs, drawing) |
| **Differences from [parent language]** | Categorised comparison table |

The applied section gets a short transition paragraph acknowledging the shift from reference to applied context. Each subsection ends with a `**See also:**` box pointing to relevant API pages.

## 3. Differences-from-parent-language structure

Do **not** use flat tables. Categorise by rationale:

1. **Concept omissions** — features from the parent language that don't apply to this domain (e.g. web concepts not needed in audio scripting, STL not available in a JIT sandbox). Brief table, short rationale paragraph.
2. **Deliberate design decisions** — intentional differences that improve reliability or catch bugs. Four-column table: Feature | Parent Language | This Language | Rationale.
3. **[Domain]-specific additions** — features this language adds that don't exist in the parent. Only include genuine language additions, not shared API classes.
4. **Behavioural differences** — features that exist in both but behave differently (e.g. `replace()` replacing all occurrences). Add to the design decisions table with clear rationale.

### Triage policy

- Items with a clear design rationale → stay in the doc, in the appropriate category
- Items with no rationale beyond "not implemented" → move to `issues.md` sidecar
- Features identical in both languages → remove entirely (don't list non-differences)

### Verification

Before documenting any difference from the parent language, **verify via REPL or source** that the difference actually exists. Never trust assumptions — test first. We found multiple items that were wrong (e.g. `===` working fine in HiseScript, `typeof` existing, Array having 25 methods not 5).

## 4. Non-trivial features need context, not just syntax

Simple reference sections (math operators, bitwise operators, primitive type tables) can be bare syntax listings. But any non-trivial language concept — structs, polyphony, parameter handling, debugging tools, namespaces, closures — should follow this pattern:

- **Why** you need this feature in this language's domain (one sentence connecting it to a real use case)
- **How** it works (syntax + a realistic, motivated example — not a minimal `Foo`/`Bar` demo)
- **Best practice hint** connecting back to the main example or showing an idiomatic pattern (e.g. "wrap your voice state in a struct inside `PolyData`", "use `enum` for parameter IDs so `setParameter` reads as `if (P == Parameters::MyGain)`")
- **Rules and constraints** if any (e.g. forward declaration order in templates)

**The test:** if a reader could get the same information from a Doxygen extract or a syntax cheat sheet, the section needs more context. A language reference should help the reader understand *when* to reach for a feature and *how* it fits into the code they're already writing.

## 5. Warning and Tip boxes

**Budget:** 6-12 boxes per page. Enough to add value without breaking flow.

### Warning candidates (things that catch people out)

- Resource limits (e.g. 32 reg variables)
- Scope leaks or unexpected visibility
- Realtime safety violations
- Silent mutation (pass-by-reference)
- Missing safety nets (no watchdog for infinite loops)
- Required explicit steps that parent language handles implicitly (capture lists)

### Tip candidates (best practices, useful shortcuts)

- Default/preferred choices (e.g. `const var` as default, `for...in` as default loop)
- IDE shortcuts that generate boilerplate
- Debugging utilities not discoverable via autocomplete
- Scaling patterns (arrays of components, shared callbacks)

### Rules

- Use `> [!Warning:title]` and `> [!Tip:title]` format (see `style-guide/canonical-links.md`)
- Maximum one styled block per section — never stack adjacent boxes
- Merge related tips into a single box rather than placing two in a row
- Titles are 3-8 words, action-oriented

## 6. Removing tutorial content

Language references should not contain best-practice tutorials or step-by-step walkthroughs. If a section teaches a workflow pattern rather than documenting a language feature:

- **Extract** the useful bits into `> [!Tip:title]` boxes placed at the relevant reference section
- **Delete** the tutorial section entirely
- Update the frontmatter/guidance to reflect the removal

## 7. Reducing API duplication

If a section duplicates content that belongs in an API reference page (e.g. listing all Rectangle methods, all Colour helpers):

- Replace with a one-line summary of the concept
- Add a `**See also:**` box linking to the API reference using `$DOMAIN.Target$` tokens

## 8. Code examples: correctness over brevity

The main code example must be **correct and complete**. Verify:

- Polyphony models are sound (all per-voice state properly wrapped)
- Sample-rate-dependent parameters have update functions called from both `prepare` and `setParameter`
- Declaration order respects language constraints (e.g. SNEX forward declaration in templates)
- Best practices are demonstrated (nested state structs, `prepare`/`reset` on inner types)

## 9. MCP style guide cross-check

After completing the doc, fetch the MCP server's style guide for the same language and compare:

- Find items the MCP guide covers that the doc doesn't → candidates for addition
- Find items the MCP guide gets wrong → flag for maintainer to fix
- The MCP guide is an LLM-facing reference; the doc is a user-facing reference. They should cover the same ground but the doc is more narrative.

## 10. See-also boxes at section boundaries

Each major subsection under the applied section should end with a `**See also:**` box pointing readers to the relevant API pages for deeper reading. Use `$DOMAIN.Target$` tokens — the cross-reference script resolves and validates them.

## 11. Consistency across language docs

Once all language docs are written, a consistency pass should:

- Unify section naming across docs (e.g. all DSP language docs use the same callback section order)
- Deduplicate shared API content into companion reference pages
- Add the Rosetta Stone example to all DSP language docs (C++, SNEX, Faust)
- Verify all `$DOMAIN.Target$` tokens resolve correctly
