#!/usr/bin/env python3
"""
Collect enrichment pipeline output for the HISE MCP server.

Parses YAML-frontmatter markdown files from the enrichment pipeline and
produces JSON data files consumed by the MCP server's data-loader.

Usage:
    python collect_mcp.py <output_dir>          # write JSON to output_dir
    python collect_mcp.py --dry-run             # parse + validate, no file writes

Reads from the enrichment directories (module_enrichment/, ui_enrichment/,
scriptnode_enrichment/) that are siblings of this script.

Outputs:
    processors.json     - enriched module documentation
    ui_components.json  - enriched UI component docs
    scriptnode.json     - scriptnode node documentation
"""

import argparse
import json
import re
import sys
from pathlib import Path

import yaml

SCRIPT_DIR = Path(__file__).resolve().parent


# ---------------------------------------------------------------------------
# YAML frontmatter parser
# ---------------------------------------------------------------------------

def parse_frontmatter(filepath: Path) -> tuple[dict, str]:
    """Parse a markdown file with YAML frontmatter. Returns (metadata, body)."""
    text = filepath.read_text(encoding="utf-8")
    if not text.startswith("---"):
        return {}, text

    end = text.find("\n---", 3)
    if end == -1:
        return {}, text

    fm_text = text[3:end].strip()
    body = text[end + 4:].strip()

    try:
        metadata = yaml.safe_load(fm_text) or {}
    except yaml.YAMLError as e:
        print(f"  [warn] YAML error in {filepath.name}: {e}", file=sys.stderr)
        metadata = {}

    return metadata, body


# ---------------------------------------------------------------------------
# Parameter extraction from signal-path glossary in markdown body
# ---------------------------------------------------------------------------

def extract_parameters_from_body(body: str) -> dict:
    """Extract parameters from ::signal-path glossary block."""
    params = {}

    # Find ::signal-path block
    match = re.search(r"::signal-path\s*\n---\s*\n(.*?)\n---", body, re.DOTALL)
    if not match:
        return params

    try:
        glossary_data = yaml.safe_load(match.group(1))
    except yaml.YAMLError:
        return params

    if not glossary_data or "glossary" not in glossary_data:
        return params

    raw_params = glossary_data["glossary"].get("parameters", {})
    for name, info in raw_params.items():
        params[name] = {
            "description": info.get("desc", ""),
            "range": info.get("range", ""),
            "default": info.get("default", ""),
        }

    return params


# ---------------------------------------------------------------------------
# Module enrichment -> processors.json
# ---------------------------------------------------------------------------

def load_module_base_params(pipeline_dir: Path) -> dict:
    """Load numeric parameter data from moduleList.json, keyed by module id."""
    path = pipeline_dir / "module_enrichment" / "base" / "moduleList.json"
    if not path.is_file():
        print(f"  [warn] {path} not found, parameters will lack numeric ranges", file=sys.stderr)
        return {}

    with open(path, "r", encoding="utf-8") as f:
        data = json.load(f)

    # Build {moduleId: {paramId: {min, max, step, defaultValue, ...}}}
    index = {}
    for mod in data.get("modules", []):
        mod_id = mod.get("id", "")
        params = {}
        for p in mod.get("parameters", []):
            pid = p.get("id", "")
            r = p.get("range", {})
            dv = p.get("defaultValue", 0)
            params[pid] = {
                "min": r.get("min", 0),
                "max": r.get("max", 0),
                "step": r.get("stepSize", 0),
                "defaultValue": dv if isinstance(dv, (int, float)) else 0,
            }
            if p.get("items"):
                params[pid]["items"] = p["items"]
        index[mod_id] = params
    return index


def collect_modules(pipeline_dir: Path) -> dict:
    """Parse module_enrichment/pages/*.md + base moduleList.json -> processors dict."""
    pages_dir = pipeline_dir / "module_enrichment" / "pages"
    if not pages_dir.is_dir():
        print(f"  [skip] {pages_dir} not found", file=sys.stderr)
        return {}

    base_params = load_module_base_params(pipeline_dir)

    modules = {}
    for md_file in sorted(pages_dir.glob("*.md")):
        fm, body = parse_frontmatter(md_file)
        if not fm:
            print(f"  [skip] {md_file.name}: no frontmatter", file=sys.stderr)
            continue

        module_id = fm.get("moduleId", md_file.stem)
        enriched_params = extract_parameters_from_body(body)
        base = base_params.get(module_id, {})

        # Merge: enriched description + base numeric values
        params = {}
        all_param_ids = set(list(enriched_params.keys()) + list(base.keys()))
        for pid in all_param_ids:
            ep = enriched_params.get(pid, {})
            bp = base.get(pid, {})
            param = {
                "description": ep.get("description", ""),
                "min": bp.get("min", 0),
                "max": bp.get("max", 0),
                "step": bp.get("step", 0),
                "defaultValue": bp.get("defaultValue", 0),
            }
            if bp.get("items"):
                param["items"] = bp["items"]
            params[pid] = param

        entry = {
            "id": module_id,
            "type": fm.get("type", ""),
            "subtype": fm.get("subtype", ""),
        }

        if fm.get("builderPath"):
            entry["builderPath"] = fm["builderPath"]
        if fm.get("tags"):
            entry["tags"] = fm["tags"]
        if fm.get("llmRef"):
            entry["llmRef"] = fm["llmRef"].rstrip()
        if fm.get("cpuProfile"):
            entry["cpuProfile"] = fm["cpuProfile"]
        if fm.get("commonMistakes"):
            entry["commonMistakes"] = fm["commonMistakes"]
        if fm.get("seeAlso"):
            entry["seeAlso"] = fm["seeAlso"]
        if fm.get("customEquivalent"):
            entry["customEquivalent"] = fm["customEquivalent"]
        if params:
            entry["parameters"] = params

        modules[module_id] = entry

    return modules


# ---------------------------------------------------------------------------
# UI enrichment -> ui_components.json
# ---------------------------------------------------------------------------

def load_existing_ui_properties(data_dir: Path | None) -> dict:
    """Load existing ui_component_properties.json for property data."""
    if data_dir is None:
        return {}
    path = data_dir / "ui_component_properties.json"
    if not path.exists():
        return {}
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)


def collect_ui_components(pipeline_dir: Path, data_dir: Path | None) -> dict:
    """Parse ui_enrichment/pages/components/*.md + existing properties -> ui dict."""
    components_dir = pipeline_dir / "ui_enrichment" / "pages" / "components"
    floating_dir = pipeline_dir / "ui_enrichment" / "pages" / "floating-tiles"

    existing = load_existing_ui_properties(data_dir)
    components = {}

    # Process enriched component pages
    for source_dir in [components_dir, floating_dir]:
        if not source_dir.is_dir():
            print(f"  [skip] {source_dir} not found", file=sys.stderr)
            continue

        for md_file in sorted(source_dir.glob("*.md")):
            fm, body = parse_frontmatter(md_file)
            if not fm:
                print(f"  [skip] {md_file.name}: no frontmatter", file=sys.stderr)
                continue

            comp_id = fm.get("componentId", md_file.stem)

            entry = {
                "id": comp_id,
                "type": fm.get("componentType", ""),
            }

            if fm.get("llmRef"):
                entry["llmRef"] = fm["llmRef"].rstrip()
            if fm.get("commonMistakes"):
                entry["commonMistakes"] = fm["commonMistakes"]
            if fm.get("seeAlso"):
                entry["seeAlso"] = fm["seeAlso"]

            # Merge existing property data
            if comp_id in existing:
                entry["properties"] = existing[comp_id]

            components[comp_id] = entry

    # Include components from existing data that have no enrichment page yet
    for comp_id, props in existing.items():
        if comp_id not in components:
            components[comp_id] = {
                "id": comp_id,
                "type": "",
                "properties": props,
            }

    return components


# ---------------------------------------------------------------------------
# Scriptnode enrichment -> scriptnode.json
# ---------------------------------------------------------------------------

def collect_scriptnode(pipeline_dir: Path) -> dict:
    """Parse scriptnode_enrichment/output/**/*.md -> scriptnode dict."""
    output_dir = pipeline_dir / "scriptnode_enrichment" / "output"
    if not output_dir.is_dir():
        print(f"  [skip] {output_dir} not found", file=sys.stderr)
        return {"nodes": {}}

    nodes = {}
    for md_file in sorted(output_dir.rglob("*.md")):
        # Skip Readme files
        if md_file.stem.lower() == "readme":
            continue

        fm, body = parse_frontmatter(md_file)
        if not fm:
            print(f"  [skip] {md_file.name}: no frontmatter", file=sys.stderr)
            continue

        factory_path = fm.get("factoryPath", f"{md_file.parent.name}.{md_file.stem}")
        params = extract_parameters_from_body(body)

        entry = {
            "factoryPath": factory_path,
            "factory": fm.get("factory", md_file.parent.name),
            "title": fm.get("title", md_file.stem),
            "description": fm.get("description", ""),
        }

        if fm.get("polyphonic") is not None:
            entry["polyphonic"] = fm["polyphonic"]
        if fm.get("tags"):
            entry["tags"] = fm["tags"]
        if fm.get("llmRef"):
            entry["llmRef"] = fm["llmRef"].rstrip()
        if fm.get("cpuProfile"):
            entry["cpuProfile"] = fm["cpuProfile"]
        if fm.get("commonMistakes"):
            entry["commonMistakes"] = fm["commonMistakes"]
        if fm.get("seeAlso"):
            entry["seeAlso"] = fm["seeAlso"]
        if params:
            entry["parameters"] = params

        nodes[factory_path] = entry

    return {"nodes": nodes}


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description="Collect enrichment pipeline output for the HISE MCP server"
    )
    parser.add_argument(
        "output_dir",
        nargs="?",
        help="Path to MCP server data directory (required unless --dry-run)",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Parse and validate only, don't write files",
    )

    args = parser.parse_args()
    pipeline_dir = SCRIPT_DIR
    output_dir = Path(args.output_dir).expanduser() if args.output_dir else None

    if not args.dry_run and output_dir is None:
        parser.error("output_dir is required unless --dry-run is set")

    print(f"Pipeline: {pipeline_dir}", file=sys.stderr)
    if output_dir:
        print(f"Output:   {output_dir}", file=sys.stderr)
    print(file=sys.stderr)

    # --- Modules ---
    print("[1/3] Collecting module enrichment...", file=sys.stderr)
    modules = collect_modules(pipeline_dir)
    enriched_count = sum(1 for m in modules.values() if m.get("llmRef"))
    print(f"      {len(modules)} modules ({enriched_count} with llmRef)", file=sys.stderr)

    # --- UI Components ---
    print("[2/3] Collecting UI component enrichment...", file=sys.stderr)
    ui = collect_ui_components(pipeline_dir, output_dir)
    enriched_count = sum(1 for c in ui.values() if c.get("llmRef"))
    props_count = sum(len(c.get("properties", {})) for c in ui.values())
    print(f"      {len(ui)} components ({enriched_count} with llmRef, {props_count} properties)",
          file=sys.stderr)

    # --- Scriptnode ---
    print("[3/3] Collecting scriptnode enrichment...", file=sys.stderr)
    sn = collect_scriptnode(pipeline_dir)
    node_count = len(sn["nodes"])
    enriched_count = sum(1 for n in sn["nodes"].values() if n.get("llmRef"))
    print(f"      {node_count} nodes ({enriched_count} with llmRef)", file=sys.stderr)

    if args.dry_run:
        print("\n[dry-run] No files written.", file=sys.stderr)
        return

    # --- Write output ---
    output_dir.mkdir(parents=True, exist_ok=True)

    def write_json(filename, data):
        path = output_dir / filename
        with open(path, "w", encoding="utf-8") as f:
            json.dump(data, f, indent=2, ensure_ascii=False)
        size = path.stat().st_size
        print(f"  {filename}: {size // 1024}KB", file=sys.stderr)

    print("\nWriting output files:", file=sys.stderr)
    write_json("processors.json", modules)
    write_json("ui_components.json", ui)
    write_json("scriptnode.json", sn)

    print("\nDone.", file=sys.stderr)


if __name__ == "__main__":
    main()
