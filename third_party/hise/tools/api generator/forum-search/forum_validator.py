#!/usr/bin/env python3
"""
Forum Code Example Validator

3-pass pipeline for validating HISE forum code examples:
  Pass 1 (--analyze):  AI analysis via Claude to generate setup code + REPL checks
  Pass 2 (--validate): HISE runtime compilation + REPL verification
  Pass 3 (--retry-failures): Re-analyze failures with error context, re-test

Input:  forum-search/code_examples/batch_*.json, snippet_batch_*.json
Output: forum-search/code_examples/validated/*.json (enriched with validation metadata)

Usage:
    python forum_validator.py --analyze --batch batch_001.json
    python forum_validator.py --analyze --all
    python forum_validator.py --validate --batch batch_001.json --launch
    python forum_validator.py --validate --all --launch
    python forum_validator.py --retry-failures --all --launch
    python forum_validator.py --coverage [--all | --batch X]
    python forum_validator.py --dry-run --batch batch_001.json --index 5
"""

import argparse
import json
import os
import re
import sys
import time
import traceback
from datetime import datetime, timezone
from pathlib import Path

# Add parent dir to path so we can import from snippet_validator
SCRIPT_DIR = Path(__file__).parent
PARENT_DIR = SCRIPT_DIR.parent
sys.path.insert(0, str(PARENT_DIR))

from snippet_validator import HISEAPIClient, HISELauncher, HISEConnectionError, SnippetValidator, Colors

CODE_EXAMPLES_DIR = SCRIPT_DIR / "code_examples"
VALIDATED_DIR = CODE_EXAMPLES_DIR / "validated"
MAX_ATTEMPTS = 2


# ---------------------------------------------------------------------------
# Data helpers
# ---------------------------------------------------------------------------

def collect_batch_files():
    """Return sorted list of all batch JSON files."""
    files = sorted(CODE_EXAMPLES_DIR.glob("batch_*.json")) + \
            sorted(CODE_EXAMPLES_DIR.glob("snippet_batch_*.json"))
    return files


def load_batch(path: Path) -> list:
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)


def save_validated_batch(filename: str, entries: list):
    VALIDATED_DIR.mkdir(parents=True, exist_ok=True)
    out = VALIDATED_DIR / filename
    with open(out, "w", encoding="utf-8") as f:
        json.dump(entries, f, indent=2, ensure_ascii=False)
    return out


def load_validated_batch(filename: str) -> list:
    path = VALIDATED_DIR / filename
    if not path.exists():
        return []
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)


def topic_slug(url: str) -> str:
    """Extract a stable slug from forum URL: 'https://forum.hise.audio/topic/7486' -> 'topic-7486'"""
    m = re.search(r'/topic/(\d+)', url)
    return f"topic-{m.group(1)}" if m else url


# ---------------------------------------------------------------------------
# Tier classification (mechanical pre-filter before AI analysis)
# ---------------------------------------------------------------------------

# Patterns that indicate external resource dependencies (Tier 4 / skip)
EXTERNAL_PATTERNS = [
    r'\.loadImage\s*\(',
    r'Server\.\w+',
    r'Engine\.loadFontAs\w*\(',
]

# Patterns for module references (Tier 3)
MODULE_PATTERNS = [
    r'Synth\.getEffect\s*\(',
    r'Synth\.getSampler\s*\(',
    r'Synth\.getChildSynth\s*\(',
    r'Synth\.getMidiProcessor\s*\(',
    r'Synth\.getModulator\s*\(',
    r'Synth\.getMidiPlayer\s*\(',
    r'Synth\.getSlotFX\s*\(',
]

# Patterns for component references (Tier 2)
COMPONENT_PATTERNS = [
    r'Content\.getComponent\s*\(',
]


def classify_tier(code: str) -> int:
    """Classify example into testability tier (1-4) based on code patterns."""
    for pat in EXTERNAL_PATTERNS:
        if re.search(pat, code):
            return 4
    for pat in MODULE_PATTERNS:
        if re.search(pat, code):
            return 3
    for pat in COMPONENT_PATTERNS:
        if re.search(pat, code):
            return 2
    return 1


# ---------------------------------------------------------------------------
# AI Analysis prompt construction
# ---------------------------------------------------------------------------

ANALYSIS_SYSTEM_PROMPT = """\
You are analyzing HiseScript code examples from the HISE audio framework forum. \
For each example, you must generate test metadata that will allow automated \
validation via the HISE runtime REST API.

## Your Task

For each code example, produce a JSON object with these fields:

```json
{
  "tier": <int 1-4>,
  "testable": <bool>,
  "skipReason": <string or null>,
  "modules": [{"type": "SimpleGain", "name": "Simple Gain1"}],
  "components": [{"type": "ScriptSlider", "id": "Knob1"}],
  "verifyScript": <array of verification objects or null>,
  "notes": <string or null>
}
```

The validator creates modules and components via REST API before compiling \
the example code. The example code is compiled exactly as-is — no prepending.

## Tier Classification

- **Tier 1**: Self-contained (pure functions, LAF, code that creates its own components).
- **Tier 2**: Needs UI components. Code calls Content.getComponent("name").
- **Tier 3**: Needs audio modules + possibly components. Code calls Synth.getEffect etc.
- **Tier 4**: Truly untestable. Mark testable: false.

## Skip Policy — Default to Testable

Only skip for truly impossible cases:
- FileSystem.browse / browseForDirectory (modal dialogs)
- loadImage / loadFontAs with {PROJECT_FOLDER} paths (files missing)
- Server.* API calls (needs network)
- Non-HiseScript code (C++, FAUST)
- References to undefined project-specific variables (e.g. TBLLaf, ErrorHandler.errorLabel)

DO NOT skip: UserPresetHandler, Engine.saveUserPreset, CSS styling, \
typed inline functions, Synth.getSampler (use StreamingSampler type), \
MIDI callbacks, LAF paint routines. When in doubt, mark testable: true \
with verifyScript: null. Let the HISE runtime decide.

## Modules Array

Each entry: {"type": "<ModuleTypeID>", "name": "<instance name>"}.
Common types: SimpleGain, SimpleReverb, Delay, Dynamics, PolyphonicFilter, \
Chorus, Saturator, StereoFX, ShapeFX, Convolution, CurveEq, \
SineSynth, StreamingSampler, SynthGroup, Noise, \
Arpeggiator, Transposer, MidiPlayer, LFO, AHDSR, Velocity.

For Synth.getSampler("Sampler1") use: {"type": "StreamingSampler", "name": "Sampler1"}
For Synth.getMidiPlayer("MIDI Player1") use: {"type": "MidiPlayer", "name": "MIDI Player1"}

## Components Array

Each entry: {"type": "<ComponentType>", "id": "<component ID>"}.
Types: ScriptSlider, ScriptButton, ScriptPanel, ScriptLabel, ScriptComboBox, \
ScriptFloatingTile, ScriptTable, ScriptSliderPack, ScriptedViewport, \
ScriptAudioWaveform, ScriptImage.

Infer from name prefix (Knob* -> ScriptSlider, Button* -> ScriptButton, \
Panel* -> ScriptPanel) or method usage (.setPaintRoutine -> ScriptPanel, \
.addItem -> ScriptComboBox). Default to ScriptSlider if ambiguous.

Don't add components the example creates itself via Content.addKnob etc.

## Verification Script

### REPL
{"type": "REPL", "expression": "expr", "value": expected, "delay": 0}

### Callback trigger via set_component_value
{"type": "set_value", "component": "Knob1", "value": 0.5, "delay": 0}
This directly triggers the control callback. Place before REPL checks that \
verify callback side effects. Use 200ms delay on the subsequent REPL check.

### Log output
{"type": "log-output", "values": ["line1", "line2"]}

### Error expectation
{"type": "expect-error", "errorMessage": "substring"}

## Guidelines

1. Default to testable. Only skip for truly impossible cases.
2. LAF/paint routines: testable compile-only, verifyScript: null.
3. MIDI callbacks (onNoteOn, onController): compile-only.
4. const var and reg variables are accessible via REPL.
5. Don't duplicate components the example creates itself.
6. Trace code carefully for expected REPL values. Skip REPL if unpredictable.
7. Default knob range is 0.0-1.0 — integer indexing won't work without reconfiguring.
"""


def build_analysis_prompt(entries: list, batch_name: str) -> str:
    """Build the user prompt for AI analysis of a batch of examples."""
    lines = [
        f"Analyze the following {len(entries)} code examples from batch `{batch_name}`.",
        "For each example, output a JSON array with one validation object per example, ",
        "in the same order as the input. Use the HISE MCP tools (query_scripting_api, ",
        "list_module_types, list_ui_components, query_ui_property) to look up any ",
        "unfamiliar APIs before generating setup code.",
        "",
        "Output ONLY the JSON array, no markdown fences or commentary.",
        "",
        "Examples:",
        ""
    ]

    for i, entry in enumerate(entries):
        lines.append(f"### Example {i} — {entry['title']}")
        lines.append(f"URL: {entry.get('url', 'N/A')}")
        lines.append(f"Description: {entry.get('description', 'N/A')}")
        lines.append(f"Tags: {', '.join(entry.get('tags', []))}")
        lines.append(f"```javascript\n{entry['code']}\n```")
        lines.append("")

    return "\n".join(lines)


def build_retry_prompt(entry: dict, error_info: dict) -> str:
    """Build a retry prompt for a failed example."""
    v = entry.get("validation", {})
    lines = [
        "The following forum code example FAILED validation. Fix the test metadata.",
        "",
        f"### {entry['title']}",
        f"URL: {entry.get('url', 'N/A')}",
        f"```javascript\n{entry['code']}\n```",
        "",
        "### Previous test metadata:",
        f"```json\n{json.dumps(v, indent=2)}\n```",
        "",
        "### Failure details:",
        f"Stage: {error_info.get('stage', 'unknown')}",
        f"Error: {json.dumps(error_info.get('error', 'unknown'))}",
        "",
        "Analyze the error and produce a corrected validation JSON object. ",
        "Common issues:",
        "- Missing component in setup (add it)",
        "- Wrong component type (e.g., needs Panel not Knob for setPaintRoutine)",
        "- Wrong expected REPL value (recalculate)",
        "- Missing Content.makeFrontInterface",
        "- Module type string incorrect (use MCP tools to look up correct type)",
        "- Code references a variable from test-only code in REPL (move to main code area)",
        "",
        "Output ONLY the corrected JSON object, no markdown fences."
    ]
    return "\n".join(lines)


# ---------------------------------------------------------------------------
# Claude API integration
# ---------------------------------------------------------------------------

SUB_BATCH_SIZE = 10  # Examples per Claude API call


def analyze_with_claude(entries: list, batch_name: str, index_filter=None,
                        model="claude-sonnet-4-20250514", force=False):
    """Call Claude API to generate validation metadata for examples.

    Processes in sub-batches of SUB_BATCH_SIZE. Returns list of entries
    with validation metadata added.
    """
    try:
        import anthropic
    except ImportError:
        print("Error: anthropic package not installed. Run: pip3 install anthropic",
              file=sys.stderr)
        sys.exit(1)

    client = anthropic.Anthropic()

    # Determine which entries to analyze
    if index_filter is not None:
        indices = [index_filter]
    else:
        # Skip entries that already have validation metadata (unless --force)
        indices = []
        for i, entry in enumerate(entries):
            if force or not entry.get("validation"):
                indices.append(i)

    if not indices:
        print("  All examples already analyzed. Use --force to re-analyze.")
        return entries

    print(f"  {len(indices)} examples to analyze (skipping {len(entries) - len(indices)} already done)")

    # Process in sub-batches
    for batch_start in range(0, len(indices), SUB_BATCH_SIZE):
        batch_indices = indices[batch_start:batch_start + SUB_BATCH_SIZE]
        batch_entries = [entries[i] for i in batch_indices]

        sub_num = batch_start // SUB_BATCH_SIZE + 1
        total_subs = (len(indices) + SUB_BATCH_SIZE - 1) // SUB_BATCH_SIZE
        print(f"\n  Sub-batch {sub_num}/{total_subs} — examples {batch_indices[0]}-{batch_indices[-1]}")

        prompt = build_analysis_prompt(batch_entries, batch_name)

        try:
            response = client.messages.create(
                model=model,
                max_tokens=8192,
                system=ANALYSIS_SYSTEM_PROMPT,
                messages=[{"role": "user", "content": prompt}]
            )

            # Extract text response
            text = ""
            for block in response.content:
                if block.type == "text":
                    text += block.text

            # Parse JSON from response (strip any markdown fences)
            text = text.strip()
            if text.startswith("```"):
                text = re.sub(r'^```\w*\n?', '', text)
                text = re.sub(r'\n?```$', '', text)
                text = text.strip()

            validations = json.loads(text)

            if len(validations) != len(batch_indices):
                print(f"    {Colors.yellow('WARNING')}: Expected {len(batch_indices)} results, got {len(validations)}")

            # Apply validations to entries
            for j, idx in enumerate(batch_indices):
                if j < len(validations):
                    entries[idx]["validation"] = validations[j]
                    status = "testable" if validations[j].get("testable") else "skip"
                    tier = validations[j].get("tier", "?")
                    has_repl = bool(validations[j].get("verifyScript"))
                    repl_str = f" +REPL({len(validations[j].get('verifyScript', []))})" if has_repl else ""
                    print(f"    [{idx}] T{tier} {status}{repl_str} — {entries[idx].get('title', '?')[:60]}")
                else:
                    print(f"    [{idx}] {Colors.red('MISSING')} — no result from Claude")

            # Token usage
            usage = response.usage
            print(f"    Tokens: {usage.input_tokens} in / {usage.output_tokens} out")

        except json.JSONDecodeError as e:
            print(f"    {Colors.red('JSON parse error')}: {e}")
            print(f"    Raw response (first 500 chars): {text[:500]}")
        except Exception as e:
            print(f"    {Colors.red('API error')}: {e}")
            traceback.print_exc()

    return entries


def retry_with_claude(entries: list, batch_name: str,
                      model="claude-sonnet-4-20250514"):
    """Re-analyze failed examples with error context."""
    try:
        import anthropic
    except ImportError:
        print("Error: anthropic package not installed", file=sys.stderr)
        sys.exit(1)

    client = anthropic.Anthropic()
    retried = 0

    for i, entry in enumerate(entries):
        v = entry.get("validation", {})
        r = v.get("result", {})

        if not (r.get("tested") and not r.get("passed") and r.get("attempts", 0) < MAX_ATTEMPTS):
            continue

        title = entry.get("title", f"Example {i}")
        print(f"\n  [{i}] Retrying: {title}")
        print(f"       Previous error ({r.get('stage', '?')}): {str(r.get('error', ''))[:100]}")

        prompt = build_retry_prompt(entry, r)

        try:
            response = client.messages.create(
                model=model,
                max_tokens=4096,
                system=ANALYSIS_SYSTEM_PROMPT,
                messages=[{"role": "user", "content": prompt}]
            )

            text = ""
            for block in response.content:
                if block.type == "text":
                    text += block.text

            text = text.strip()
            if text.startswith("```"):
                text = re.sub(r'^```\w*\n?', '', text)
                text = re.sub(r'\n?```$', '', text)
                text = text.strip()

            new_validation = json.loads(text)

            # Preserve the old result's attempt count
            old_attempts = r.get("attempts", 1)
            new_validation["result"] = {"attempts": old_attempts}  # will be updated on next validate

            entries[i]["validation"] = new_validation
            retried += 1
            print(f"       {Colors.cyan('Updated')} — T{new_validation.get('tier', '?')}")

        except Exception as e:
            print(f"       {Colors.red('Retry failed')}: {e}")

    print(f"\n  Retried {retried} examples")
    return entries


# ---------------------------------------------------------------------------
# HISE Validation runner
# ---------------------------------------------------------------------------

# Module chain index mapping for builder/apply
CHAIN_FX = 3
CHAIN_CHILDREN = -1
CHAIN_MIDI = 0

# Module categories that determine which chain to use
EFFECT_TYPES = {
    "SimpleGain", "SimpleReverb", "Convolution", "Delay", "Dynamics",
    "Saturator", "StereoFX", "ShapeFX", "PolyshapeFX", "PhaseFX",
    "PolyphonicFilter", "HarmonicFilter", "CurveEq", "Chorus",
    "HardcodedMasterFX", "SendFX", "SlotFX", "RouteFX", "EmptyFX",
    "Analyser",
}

MIDI_PROCESSOR_TYPES = {
    "Arpeggiator", "Transposer", "MidiPlayer", "MidiMuter",
    "ReleaseTrigger", "CC2Note", "CCSwapper", "ChannelFilter",
    "ChannelSetter", "ChokeGroupProcessor", "MidiMetronome",
}


def _chain_for_module_type(module_type: str) -> int:
    """Determine the chain index for a module type."""
    if module_type in EFFECT_TYPES:
        return CHAIN_FX
    if module_type in MIDI_PROCESSOR_TYPES:
        return CHAIN_MIDI
    # Default to children (sound generators, containers)
    return CHAIN_CHILDREN


def run_validation(entries: list, launcher: HISELauncher, api: HISEAPIClient,
                   index_filter=None, verbose=True):
    """Run HISE validation on entries that have validation metadata.

    Uses the new REST API flow:
    1. builder/reset — clean module tree
    2. builder/apply — add required modules
    3. ui/apply — add required UI components
    4. set_script — compile the example code
    5. set_component_value + repl — verify behavior

    Modifies entries in-place, adding/updating validation.result.
    Returns (passed, failed, skipped) counts.
    """
    validator = SnippetValidator(api)
    passed = failed = skipped = 0

    for i, entry in enumerate(entries):
        if index_filter is not None and i != index_filter:
            continue

        v = entry.get("validation")
        if not v:
            skipped += 1
            continue

        title = entry.get("title", f"Example {i}")

        if not v.get("testable", False):
            if verbose:
                print(f"  {Colors.dim(f'[{i}]')} {Colors.yellow('SKIP')} {title} — {v.get('skipReason', 'not testable')}")
            skipped += 1
            continue

        timestamp = datetime.now(timezone.utc).isoformat().replace('+00:00', 'Z')
        code = entry.get("code", "")

        # --- Step 1: Reset module tree for clean state ---
        try:
            time.sleep(0.5)  # let async UI ops from previous example settle
            reset_result = api.builder_reset()
            if not reset_result.get("success"):
                result = {"tested": True, "passed": False, "stage": "setup",
                          "error": "builder/reset failed", "timestamp": timestamp}
                _store_result(v, result)
                failed += 1
                if verbose:
                    print(f"  {Colors.dim(f'[{i}]')} {Colors.red('FAIL')} {title} [setup] builder/reset failed")
                continue
            time.sleep(0.5)  # let module tree teardown complete before next step
        except HISEConnectionError as e:
            result = {"tested": True, "passed": False, "stage": "setup",
                      "error": str(e), "timestamp": timestamp}
            _store_result(v, result)
            failed += 1
            if verbose:
                print(f"  {Colors.dim(f'[{i}]')} {Colors.red('FAIL')} {title} [setup] {e}")
            # Check if HISE is still alive
            time.sleep(2)
            try:
                api.status()
            except Exception:
                if verbose:
                    print(f"  {Colors.yellow('WARNING')}: HISE unresponsive after builder/reset, stopping")
                break
            continue

        # --- Step 2: Add modules via builder/apply ---
        # Modules with a "parent" field referencing another module in the list
        # must be added in a separate call after the parent exists.
        modules = v.get("modules", [])
        if modules:
            # Split into batches: first batch = modules with default parent,
            # subsequent batches = modules whose parent was just created.
            master_ops = []
            child_ops = []  # list of (parent, op) for modules needing a non-Master parent
            for mod in modules:
                chain = mod.get("chain", _chain_for_module_type(mod["type"]))
                parent = mod.get("parent", "Master Chain")
                op = {
                    "op": "add",
                    "type": mod["type"],
                    "parent": parent,
                    "chain": chain,
                    "name": mod["name"]
                }
                if parent == "Master Chain":
                    master_ops.append(op)
                else:
                    child_ops.append(op)

            setup_failed = False
            for batch in ([master_ops] if not child_ops else [master_ops, child_ops]):
                if not batch:
                    continue
                try:
                    builder_result = api.builder_apply(batch)
                    if not builder_result.get("success"):
                        errors = builder_result.get("errors", [])
                        err_msg = errors[0].get("errorMessage", "unknown") if errors else "unknown"
                        result = {"tested": True, "passed": False, "stage": "setup",
                                  "error": f"builder/apply: {err_msg}", "timestamp": timestamp}
                        _store_result(v, result)
                        failed += 1
                        if verbose:
                            print(f"  {Colors.dim(f'[{i}]')} {Colors.red('FAIL')} {title} [setup] builder/apply: {err_msg[:100]}")
                        setup_failed = True
                        break
                    time.sleep(0.3)  # let module init settle before adding children
                except Exception as e:
                    result = {"tested": True, "passed": False, "stage": "setup",
                              "error": str(e), "timestamp": timestamp}
                    _store_result(v, result)
                    failed += 1
                    if verbose:
                        print(f"  {Colors.dim(f'[{i}]')} {Colors.red('FAIL')} {title} [setup] {e}")
                    setup_failed = True
                    break
            if setup_failed:
                continue

        # --- Step 3: Add UI components via ui/apply ---
        components = v.get("components", [])
        if components:
            ops = []
            for comp in components:
                ops.append({
                    "op": "add",
                    "componentType": comp["type"],
                    "id": comp["id"]
                })
            try:
                ui_result = api.ui_apply("Interface", ops)
                if not ui_result.get("success"):
                    errors = ui_result.get("errors", [])
                    err_msg = errors[0].get("errorMessage", "unknown") if errors else "unknown"
                    result = {"tested": True, "passed": False, "stage": "setup",
                              "error": f"ui/apply: {err_msg}", "timestamp": timestamp}
                    _store_result(v, result)
                    failed += 1
                    if verbose:
                        print(f"  {Colors.dim(f'[{i}]')} {Colors.red('FAIL')} {title} [setup] ui/apply: {err_msg[:100]}")
                    continue
            except Exception as e:
                result = {"tested": True, "passed": False, "stage": "setup",
                          "error": str(e), "timestamp": timestamp}
                _store_result(v, result)
                failed += 1
                if verbose:
                    print(f"  {Colors.dim(f'[{i}]')} {Colors.red('FAIL')} {title} [setup] {e}")
                continue

        # --- Steps 4-5: Compile + verify (wrapped in connection error handler) ---
        try:
            p, f, s = _run_example_test(api, validator, v, entry, i, title, timestamp, verbose)
            passed += p
            failed += f
            skipped += s
        except HISEConnectionError as e:
            result = {"tested": True, "passed": False, "stage": "execute",
                      "error": f"HISE connection lost: {e}", "timestamp": timestamp}
            _store_result(v, result)
            failed += 1
            if verbose:
                print(f"  {Colors.dim(f'[{i}]')} {Colors.red('FAIL')} {title} [execute] HISE connection lost")
            # Try to wait for HISE to recover
            time.sleep(2)
            try:
                api.status()
            except Exception:
                if verbose:
                    print(f"  {Colors.yellow('WARNING')}: HISE unresponsive, skipping remaining examples")
                break

    return passed, failed, skipped


def _run_example_test(api, validator, v, entry, i, title, timestamp, verbose):
    """Run compile + verify for a single example. Returns (passed, failed, skipped) tuple."""
    code = entry.get("code", "")
    modules = v.get("modules", [])

    # --- Handle legacy setupCode (backward compat with batch_001) ---
    setup_code = v.get("setupCode")
    if setup_code:
        module_ops = []
        component_lines = []
        for line in setup_code.split("\n"):
            stripped = line.strip()
            if stripped.startswith("// MODULE_SETUP: "):
                m = re.search(r'Synth\.addEffect\(\s*"([^"]+)"\s*,\s*"([^"]+)"', stripped)
                if m:
                    mod_type, mod_name = m.group(1), m.group(2)
                    chain = _chain_for_module_type(mod_type)
                    module_ops.append({
                        "op": "add", "type": mod_type,
                        "parent": "Master Chain", "chain": chain,
                        "name": mod_name
                    })
            else:
                component_lines.append(line)

        if module_ops and not modules:
            builder_result = api.builder_apply(module_ops)
            if not builder_result.get("success"):
                errors = builder_result.get("errors", [])
                err_msg = errors[0].get("errorMessage", "unknown") if errors else "unknown"
                result = {"tested": True, "passed": False, "stage": "setup",
                          "error": f"legacy builder/apply: {err_msg}", "timestamp": timestamp}
                _store_result(v, result)
                if verbose:
                    print(f"  {Colors.dim(f'[{i}]')} {Colors.red('FAIL')} {title} [setup] legacy builder: {err_msg[:100]}")
                return 0, 1, 0

        clean_setup = "\n".join(component_lines).strip()
        if clean_setup:
            code = clean_setup + "\n\n" + code

    test_only = v.get("testOnlyCode")
    verify_scripts = v.get("verifyScript")

    # Separate set_value steps from REPL/log steps
    set_value_steps = []
    repl_verify_steps = []
    if verify_scripts:
        for vs in (verify_scripts if isinstance(verify_scripts, list) else [verify_scripts]):
            if vs.get("type") == "set_value":
                set_value_steps.append(vs)
            else:
                repl_verify_steps.append(vs)

    if set_value_steps:
        # --- Path A: Has set_value triggers (compile, trigger, verify separately) ---
        compile_result = api.set_script("Interface", {"onInit": code})
        if not compile_result.get("success") or compile_result.get("errors"):
            err = compile_result.get("errors", ["unknown"])
            err_str = err[0] if isinstance(err, list) and err else str(err)
            if isinstance(err_str, dict):
                err_str = err_str.get("errorMessage", str(err_str))
            result = {"tested": True, "passed": False, "stage": "execute",
                      "error": compile_result.get("errors", []), "timestamp": timestamp}
            _store_result(v, result)
            if verbose:
                print(f"  {Colors.dim(f'[{i}]')} {Colors.red('FAIL')} {title} [execute] {str(err_str)[:120]}")
            return 0, 1, 0

        time.sleep(0.2)

        # Fire set_value triggers
        for sv in set_value_steps:
            delay_ms = sv.get("delay", 0)
            if delay_ms > 0:
                time.sleep(delay_ms / 1000.0)
            api.set_component_value("Interface", sv["component"], sv["value"])

        # Run REPL verification
        if repl_verify_steps:
            verifications = []
            for j, vs in enumerate(repl_verify_steps):
                delay_ms = vs.get("delay", 100 if j > 0 else 0)
                delay_ms = min(delay_ms, 1000)
                if delay_ms > 0:
                    time.sleep(delay_ms / 1000.0)

                if vs.get("type") == "REPL":
                    expr = vs.get("expression", "")
                    expected = vs.get("value")
                    repl_result = api.repl("Interface", expr)
                    actual = repl_result.get("value")
                    verifications.append({"type": "REPL", "expression": expr,
                                          "expected": expected, "actual": actual, "delay": delay_ms})
                    if not validator._values_match(expected, actual):
                        result = {"tested": True, "passed": False, "stage": "verify",
                                  "error": f"Expected {expr} -> {expected}, got {actual}",
                                  "verifications": verifications, "timestamp": timestamp}
                        _store_result(v, result)
                        if verbose:
                            print(f"  {Colors.dim(f'[{i}]')} {Colors.red('FAIL')} {title} [verify] {expr}: expected {expected}, got {actual}")
                        return 0, 1, 0

                elif vs.get("type") == "log-output":
                    expected_logs = vs.get("values", [])
                    actual_logs = validator._filter_test_noise(compile_result.get("logs", []))
                    verifications.append({"type": "log-output",
                                          "expected": expected_logs, "actual": list(actual_logs), "delay": delay_ms})
                    if not validator._logs_match(expected_logs, actual_logs):
                        result = {"tested": True, "passed": False, "stage": "verify",
                                  "error": f"Log mismatch", "verifications": verifications, "timestamp": timestamp}
                        _store_result(v, result)
                        if verbose:
                            print(f"  {Colors.dim(f'[{i}]')} {Colors.red('FAIL')} {title} [verify] log mismatch")
                        return 0, 1, 0

            result = {"tested": True, "passed": True,
                      "verifications": verifications, "timestamp": timestamp}
            _store_result(v, result)
            if verbose:
                print(f"  {Colors.dim(f'[{i}]')} {Colors.green('PASS')} {title} ({len(verifications)} checks)")
            return 1, 0, 0
        else:
            result = {"tested": True, "passed": True, "verifications": [], "timestamp": timestamp}
            _store_result(v, result)
            if verbose:
                print(f"  {Colors.dim(f'[{i}]')} {Colors.green('PASS')} {title}")
            return 1, 0, 0

    else:
        # --- Path B: No set_value steps — use SnippetValidator directly ---
        test_dict = {
            "code": code,
            "testMetadata": {
                "testable": True,
                "verifyScript": repl_verify_steps if repl_verify_steps else None,
            }
        }
        if test_only:
            test_dict["testMetadata"]["testOnly"] = test_only
        test_dict["testMetadata"] = {k: val for k, val in test_dict["testMetadata"].items() if val is not None}

        result = validator.test_example(test_dict)
        _store_result(v, result)

        if result.get("passed"):
            if verbose:
                vc = len(result.get("verifications", []))
                vs = f" ({vc} checks)" if vc else ""
                print(f"  {Colors.dim(f'[{i}]')} {Colors.green('PASS')} {title}{vs}")
            return 1, 0, 0
        elif result.get("skipped"):
            if verbose:
                print(f"  {Colors.dim(f'[{i}]')} {Colors.yellow('SKIP')} {title} — {result.get('reason', '')}")
            return 0, 0, 1
        else:
            if verbose:
                stage = result.get("stage", "?")
                error = result.get("error", "")
                if isinstance(error, list):
                    error = "; ".join(str(e) for e in error[:2])
                print(f"  {Colors.dim(f'[{i}]')} {Colors.red('FAIL')} {title} [{stage}] {str(error)[:120]}")
            return 0, 1, 0

    return passed, failed, skipped


def _store_result(validation: dict, result: dict):
    """Store test result, preserving attempt count."""
    attempts = validation.get("result", {}).get("attempts", 0) + 1
    result["attempts"] = attempts
    validation["result"] = result


# ---------------------------------------------------------------------------
# Coverage report
# ---------------------------------------------------------------------------

def print_coverage(entries: list, batch_name: str):
    """Print a coverage summary for a batch."""
    tier_counts = {1: 0, 2: 0, 3: 0, 4: 0}
    tested = passed = failed = skipped = 0
    no_metadata = 0

    for entry in entries:
        v = entry.get("validation")
        if not v:
            no_metadata += 1
            tier = classify_tier(entry.get("code", ""))
            tier_counts[tier] += 1
            continue

        tier_counts[v.get("tier", 1)] += 1
        r = v.get("result")
        if r:
            if r.get("tested"):
                tested += 1
                if r.get("passed"):
                    passed += 1
                else:
                    failed += 1
            elif r.get("skipped"):
                skipped += 1

    total = len(entries)
    print(f"\n{Colors.bold(batch_name)} — {total} examples")
    print(f"  Tiers: T1={tier_counts[1]}  T2={tier_counts[2]}  T3={tier_counts[3]}  T4={tier_counts[4]}")

    if no_metadata == total:
        print(f"  Status: {Colors.yellow('No validation metadata')} (run --analyze first)")
    else:
        analyzed = total - no_metadata
        print(f"  Analyzed: {analyzed}/{total}")
        if tested or skipped:
            print(f"  Results: {Colors.green(f'{passed} passed')}  {Colors.red(f'{failed} failed')}  {Colors.yellow(f'{skipped} skipped')}  {tested} tested")


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def resolve_batches(args) -> list:
    """Return list of (filename, path) tuples based on CLI args."""
    if args.batch:
        path = CODE_EXAMPLES_DIR / args.batch
        if not path.exists():
            print(f"Batch file not found: {path}", file=sys.stderr)
            sys.exit(1)
        return [(args.batch, path)]
    elif args.all:
        return [(f.name, f) for f in collect_batch_files()]
    else:
        print("Specify --batch <filename> or --all", file=sys.stderr)
        sys.exit(1)


def main():
    parser = argparse.ArgumentParser(description="Forum code example validator")

    # Modes
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--analyze", action="store_true",
                      help="Pass 1: AI analysis to generate validation metadata")
    mode.add_argument("--validate", action="store_true",
                      help="Pass 2: Run HISE validation on analyzed examples")
    mode.add_argument("--retry-failures", action="store_true",
                      help="Pass 3: Re-analyze and re-test failures")
    mode.add_argument("--coverage", action="store_true",
                      help="Print coverage report")
    mode.add_argument("--dry-run", action="store_true",
                      help="Show what would be tested without running")
    mode.add_argument("--apply-metadata", nargs=2, metavar=("BATCH", "METADATA"),
                      help="Apply validation metadata JSON to a batch file")

    # Batch selection
    parser.add_argument("--batch", type=str, help="Single batch filename")
    parser.add_argument("--all", action="store_true", help="Process all batches")
    parser.add_argument("--index", type=int, default=None,
                        help="Process single example by index within batch")

    # HISE options
    parser.add_argument("--launch", action="store_true",
                        help="Auto-launch HISE runtime")
    parser.add_argument("--keep-alive", action="store_true",
                        help="Keep HISE running after validation")
    parser.add_argument("--port", type=int, default=1900,
                        help="HISE REST API port (default: 1900)")

    # Claude options
    parser.add_argument("--model", type=str, default="claude-sonnet-4-20250514",
                        help="Claude model for AI analysis (default: claude-sonnet-4-20250514)")
    parser.add_argument("--force", action="store_true",
                        help="Re-analyze even if validation metadata already exists")
    parser.add_argument("--use-api", action="store_true",
                        help="Use Anthropic API directly instead of generating Claude Code instructions")

    args = parser.parse_args()

    # --apply-metadata mode
    if args.apply_metadata:
        batch_filename, metadata_path = args.apply_metadata

        # Load the base batch (validated if exists, else raw)
        entries = load_validated_batch(batch_filename)
        if not entries:
            raw_path = CODE_EXAMPLES_DIR / batch_filename
            if not raw_path.exists():
                print(f"Batch file not found: {raw_path}", file=sys.stderr)
                sys.exit(1)
            entries = load_batch(raw_path)

        # Load metadata
        with open(metadata_path, "r", encoding="utf-8") as f:
            metadata = json.load(f)

        # metadata can be:
        # - A list (one per entry, indexed by position)
        # - A dict keyed by index (string keys)
        if isinstance(metadata, list):
            for i, m in enumerate(metadata):
                if m and i < len(entries):
                    entries[i]["validation"] = m
            applied = len([m for m in metadata if m])
        elif isinstance(metadata, dict):
            for idx_str, m in metadata.items():
                idx = int(idx_str)
                if idx < len(entries) and m:
                    entries[idx]["validation"] = m
            applied = len(metadata)
        else:
            print("Metadata must be a JSON array or object keyed by index", file=sys.stderr)
            sys.exit(1)

        out = save_validated_batch(batch_filename, entries)
        print(f"Applied {applied} validation entries to {batch_filename}")
        print(f"Saved to: {out}")
        return

    # --coverage mode
    if args.coverage:
        batches = resolve_batches(args)
        totals = {"t": 0, "p": 0, "f": 0, "s": 0}
        for filename, path in batches:
            # Try validated version first, fall back to raw
            entries = load_validated_batch(filename)
            if not entries:
                entries = load_batch(path)
            print_coverage(entries, filename)
            for e in entries:
                totals["t"] += 1
                r = e.get("validation", {}).get("result", {})
                if r.get("passed"): totals["p"] += 1
                elif r.get("tested") and not r.get("passed"): totals["f"] += 1
        if len(batches) > 1:
            p_count = totals["p"]
            f_count = totals["f"]
            print(f"\n{Colors.bold('TOTAL')}: {totals['t']} examples, "
                  f"{Colors.green(f'{p_count} passed')}, "
                  f"{Colors.red(f'{f_count} failed')}")
        return

    # --dry-run mode
    if args.dry_run:
        batches = resolve_batches(args)
        for filename, path in batches:
            entries = load_validated_batch(filename)
            if not entries:
                entries = load_batch(path)
            for i, entry in enumerate(entries):
                if args.index is not None and i != args.index:
                    continue
                v = entry.get("validation")
                print(f"\n{'='*60}")
                print(f"[{i}] {entry.get('title', '?')}")
                print(f"URL: {entry.get('url', '?')}")
                print(f"Tier: {classify_tier(entry.get('code', ''))}")
                if v:
                    print(f"\nSetup code:\n{v.get('setupCode', '(none)')}")
                    print(f"\nTest-only code:\n{v.get('testOnlyCode', '(none)')}")
                    print(f"\nVerify script:\n{json.dumps(v.get('verifyScript'), indent=2)}")
                    r = v.get("result")
                    if r:
                        status = "PASS" if r.get("passed") else "FAIL" if r.get("tested") else "SKIP"
                        print(f"\nResult: {status}")
                        if r.get("error"):
                            print(f"Error: {r['error']}")
                else:
                    print("No validation metadata (run --analyze)")
                print(f"\nCode:\n{entry.get('code', '(empty)')}")
        return

    # --analyze mode
    if args.analyze:
        batches = resolve_batches(args)
        for filename, path in batches:
            # Load existing validated data or start fresh from raw batch
            entries = load_validated_batch(filename)
            if not entries:
                entries = load_batch(path)

            # Determine which examples need analysis
            if args.index is not None:
                pending = [args.index]
            elif args.force:
                pending = list(range(len(entries)))
            else:
                pending = [i for i, e in enumerate(entries) if not e.get("validation")]

            print(f"\n{Colors.bold(f'Analyzing {filename}')} — {len(entries)} examples, {len(pending)} pending")

            if not pending:
                print("  All examples already analyzed. Use --force to re-analyze.")
                continue

            if args.use_api:
                # Direct Claude API mode
                entries = analyze_with_claude(entries, filename,
                                             index_filter=args.index,
                                             model=args.model,
                                             force=args.force)
                out = save_validated_batch(filename, entries)
                print(f"\n  Saved to: {out}")
            else:
                # Claude Code mode: generate instruction file
                VALIDATED_DIR.mkdir(parents=True, exist_ok=True)
                instructions_file = VALIDATED_DIR / f"{filename.replace('.json', '')}_instructions.md"

                lines = [
                    f"# Forum Example Analysis: {filename}",
                    "",
                    f"Analyze {len(pending)} examples from `{path}` and generate validation metadata.",
                    "",
                    "## Instructions",
                    "",
                    "For each example listed below, generate a validation JSON object and apply it.",
                    "Use MCP tools (`query_scripting_api`, `list_module_types`, `list_ui_components`, ",
                    "`query_ui_property`) to look up unfamiliar APIs before generating setup code.",
                    "",
                    "After analyzing all examples, write the enriched JSON to:",
                    f"`{VALIDATED_DIR / filename}`",
                    "",
                    "## System Prompt (analysis guidelines)",
                    "",
                    ANALYSIS_SYSTEM_PROMPT,
                    "",
                    f"## Examples to Analyze (indices: {pending[0]}-{pending[-1]})",
                    "",
                ]

                for i in pending:
                    entry = entries[i]
                    lines.append(f"### [{i}] {entry.get('title', 'Untitled')}")
                    lines.append(f"URL: {entry.get('url', 'N/A')}")
                    lines.append(f"Tags: {', '.join(entry.get('tags', []))}")
                    lines.append(f"Mechanical tier: {classify_tier(entry.get('code', ''))}")
                    lines.append(f"Description: {entry.get('description', 'N/A')}")
                    lines.append(f"```javascript\n{entry['code']}\n```")
                    lines.append("")

                with open(instructions_file, "w", encoding="utf-8") as f:
                    f.write("\n".join(lines))

                print(f"  Instructions written to: {instructions_file}")
                print(f"  Run a Claude Code session to analyze, then use:")
                print(f"    python forum_validator.py --apply-metadata {filename} <metadata.json>")
        return

    # --validate mode
    if args.validate:
        batches = resolve_batches(args)
        launcher = None
        api = HISEAPIClient(f"http://127.0.0.1:{args.port}")

        try:
            if args.launch:
                launcher = HISELauncher(port=args.port)
                if not launcher.is_running():
                    print("Launching HISE runtime...")
                    if not launcher.launch():
                        print(Colors.red("Failed to launch HISE"), file=sys.stderr)
                        sys.exit(1)
                else:
                    print("HISE already running")

            total_p = total_f = total_s = 0

            for filename, path in batches:
                entries = load_validated_batch(filename)
                if not entries:
                    print(f"{Colors.yellow(f'No validated data for {filename}')} — run --analyze first")
                    continue

                print(f"\n{Colors.bold(f'Validating {filename}')} — {len(entries)} examples")
                p, f_count, s = run_validation(entries, launcher, api,
                                               index_filter=args.index,
                                               verbose=True)
                total_p += p
                total_f += f_count
                total_s += s

                # Save results back
                save_validated_batch(filename, entries)
                print(f"  Results saved to {VALIDATED_DIR / filename}")

            print(f"\n{Colors.bold('Summary')}: {Colors.green(f'{total_p} passed')}  "
                  f"{Colors.red(f'{total_f} failed')}  {Colors.yellow(f'{total_s} skipped')}")

        finally:
            if launcher and not args.keep_alive:
                launcher.cleanup()

        return

    # --retry-failures mode
    if args.retry_failures:
        batches = resolve_batches(args)
        launcher = None
        api = None

        try:
            # Set up HISE if --launch specified (for re-validation after retry)
            if args.launch:
                api = HISEAPIClient(f"http://127.0.0.1:{args.port}")
                launcher = HISELauncher(port=args.port)
                if not launcher.is_running():
                    print("Launching HISE runtime...")
                    if not launcher.launch():
                        print(Colors.red("Failed to launch HISE"), file=sys.stderr)
                        sys.exit(1)

            for filename, path in batches:
                entries = load_validated_batch(filename)
                if not entries:
                    print(f"No validated data for {filename}")
                    continue

                # Count retryable failures
                retryable = sum(1 for e in entries
                                if e.get("validation", {}).get("result", {}).get("tested")
                                and not e.get("validation", {}).get("result", {}).get("passed")
                                and e.get("validation", {}).get("result", {}).get("attempts", 0) < MAX_ATTEMPTS)

                if retryable == 0:
                    print(f"{filename}: no retryable failures")
                    continue

                print(f"\n{Colors.bold(filename)}: {retryable} failures to retry")

                # Pass 3a: Re-analyze with Claude
                entries = retry_with_claude(entries, filename, model=args.model)

                # Pass 3b: Re-validate with HISE (if --launch)
                if api and launcher:
                    print(f"\n  Re-validating retried examples...")
                    # Only validate entries that were retried (no result yet or cleared result)
                    p, f_count, s = run_validation(entries, launcher, api, verbose=True)
                    print(f"  Re-validation: {Colors.green(f'{p} passed')}  {Colors.red(f'{f_count} failed')}  {Colors.yellow(f'{s} skipped')}")

                out = save_validated_batch(filename, entries)
                print(f"  Saved to: {out}")

        finally:
            if launcher and not args.keep_alive:
                launcher.cleanup()
        return


if __name__ == "__main__":
    main()
