#!/usr/bin/env python3
"""
One-time migration script: convert existing link formats to canonical $DOMAIN$ tokens.

Transforms:
1. API phase1 methods.md: **Cross References:** entries
   `ClassName.method` -> `$API.ClassName.method$`

2. API phase4a/4b userDocs: > **Warning:** blockquotes
   > **Warning:** text -> > [!Warning:$WARNING_TO_BE_REPLACED$] text

3. Module pages: ::see-also MDC blocks
   ::see-also with hardcoded URLs -> **See also:** $MODULES.xxx$ -- desc

4. Module pages: ::warning / ::tip blocks (if present as raw blockquotes)

Usage:
    python backport_links.py --dry-run          # preview changes
    python backport_links.py                    # apply changes
    python backport_links.py --target phase1    # only phase1 cross-refs
    python backport_links.py --target phase4    # only phase4 warnings
    python backport_links.py --target modules   # only module see-also
"""

import argparse
import json
import re
import sys
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
PHASE1_DIR = SCRIPT_DIR / "enrichment" / "phase1"
PHASE4_AUTO_DIR = SCRIPT_DIR / "enrichment" / "phase4" / "auto"
PHASE4_MANUAL_DIR = SCRIPT_DIR / "enrichment" / "phase4" / "manual"
PHASE4B_DIR = SCRIPT_DIR / "enrichment" / "phase4b"
MODULE_PAGES_DIR = SCRIPT_DIR / "module_enrichment" / "pages"
MODULE_STATIC_DIR = SCRIPT_DIR / "module_enrichment" / "resources" / "static"
MODULE_LIST_PATH = SCRIPT_DIR / "module_enrichment" / "base" / "moduleList.json"
ENRICHMENT_BASE_DIR = SCRIPT_DIR / "enrichment" / "base"

WARNING_PLACEHOLDER = "$WARNING_TO_BE_REPLACED$"
TIP_PLACEHOLDER = "$TIP_TO_BE_REPLACED$"


# ---------------------------------------------------------------------------
# Module ID lookup (for reverse-resolving URLs to module IDs)
# ---------------------------------------------------------------------------

def build_module_url_to_id(module_list_path: Path) -> dict:
    """Build a reverse mapping from URL slug fragments to module IDs."""
    if not module_list_path.is_file():
        return {}

    with open(module_list_path, "r", encoding="utf-8") as f:
        data = json.load(f)

    url_to_id = {}
    for module in data.get("modules", []):
        module_id = module["id"]
        slug = module_id.lower()
        url_to_id[slug] = module_id
    return url_to_id


# ---------------------------------------------------------------------------
# Phase 1: Cross References
# ---------------------------------------------------------------------------

def backport_phase1_crossrefs(dry_run: bool) -> int:
    """Convert `ClassName.method` to `$API.ClassName.method$` in phase1 methods.md files."""
    if not PHASE1_DIR.is_dir():
        print("  Phase 1 directory not found, skipping")
        return 0

    total_changes = 0
    for class_dir in sorted(PHASE1_DIR.iterdir()):
        if not class_dir.is_dir():
            continue
        methods_file = class_dir / "methods.md"
        if not methods_file.is_file():
            continue

        with open(methods_file, "r", encoding="utf-8") as f:
            content = f.read()

        original = content

        # Find cross-reference bullet items: - `ClassName.method`
        # Convert to: - `$API.ClassName.method$`
        # Pattern: backtick-wrapped ClassName.method (PascalCase class, camelCase method)
        def replace_xref(m):
            ref = m.group(1)
            # Only convert if it looks like a ClassName.method reference
            # (starts with uppercase, contains a dot)
            if '.' in ref and ref[0].isupper():
                return f'`$API.{ref}$`'
            # Standalone class reference
            if ref[0].isupper() and '.' not in ref:
                return f'`$API.{ref}$`'
            return m.group(0)

        # Match backtick-wrapped references in Cross References sections
        in_xref_section = False
        lines = content.split('\n')
        new_lines = []
        for line in lines:
            if line.startswith('**Cross References:**'):
                in_xref_section = True
                new_lines.append(line)
                continue
            if in_xref_section:
                if line.startswith('**') or (line.strip() == '' and new_lines and new_lines[-1].strip() == ''):
                    in_xref_section = False
                elif line.startswith('- `'):
                    # Replace backtick-wrapped reference
                    line = re.sub(r'`([A-Z]\w*(?:\.\w+)?)`', replace_xref, line)
            new_lines.append(line)

        content = '\n'.join(new_lines)

        if content != original:
            changes = sum(1 for a, b in zip(original.split('\n'), content.split('\n')) if a != b)
            total_changes += changes
            if dry_run:
                print(f"  [DRY] {methods_file.relative_to(SCRIPT_DIR)}: {changes} lines changed")
            else:
                with open(methods_file, "w", encoding="utf-8") as f:
                    f.write(content)
                print(f"  {methods_file.relative_to(SCRIPT_DIR)}: {changes} lines changed")

    return total_changes


# ---------------------------------------------------------------------------
# Phase 4: Warnings
# ---------------------------------------------------------------------------

def backport_phase4_warnings(dry_run: bool) -> int:
    """Convert > **Warning:** to > [!Warning:$WARNING_TO_BE_REPLACED$] in phase4 files."""
    total_changes = 0

    for phase4_dir in (PHASE4_AUTO_DIR, PHASE4_MANUAL_DIR):
        if not phase4_dir.is_dir():
            continue
        for md_file in sorted(phase4_dir.rglob("*.md")):
            with open(md_file, "r", encoding="utf-8") as f:
                content = f.read()

            original = content

            # Convert > **Warning:** text to > [!Warning:$WARNING_TO_BE_REPLACED$] text
            content = re.sub(
                r'^> \*\*Warning:\*\* (.+)$',
                lambda m: f'> [!Warning:{WARNING_PLACEHOLDER}] {m.group(1)}',
                content,
                flags=re.MULTILINE
            )

            if content != original:
                changes = content.count(WARNING_PLACEHOLDER)
                total_changes += changes
                if dry_run:
                    print(f"  [DRY] {md_file.relative_to(SCRIPT_DIR)}: {changes} warnings")
                else:
                    with open(md_file, "w", encoding="utf-8") as f:
                        f.write(content)
                    print(f"  {md_file.relative_to(SCRIPT_DIR)}: {changes} warnings")

    return total_changes


# ---------------------------------------------------------------------------
# Module pages: ::see-also
# ---------------------------------------------------------------------------

def backport_module_see_also(dry_run: bool) -> int:
    """Convert ::see-also MDC blocks to **See also:** with $MODULES$ tokens."""
    if not MODULE_PAGES_DIR.is_dir():
        print("  Module pages directory not found, skipping")
        return 0

    url_to_id = build_module_url_to_id(MODULE_LIST_PATH)
    total_changes = 0

    for md_file in sorted(MODULE_PAGES_DIR.glob("*.md")):
        with open(md_file, "r", encoding="utf-8") as f:
            content = f.read()

        original = content

        # Find ::see-also blocks and convert them
        # Pattern: ::see-also\n---\nlinks:\n  - { label: "...", to: "...", desc: "..." }\n---\n::
        see_also_pattern = re.compile(
            r'::see-also\n---\nlinks:\n((?:\s+- \{[^}]+\}\n?)+)---\n::',
            re.MULTILINE
        )

        def replace_see_also_block(m):
            links_text = m.group(1)
            # Parse each link entry
            entries = re.findall(
                r'label:\s*"([^"]+)".*?to:\s*"([^"]+)"(?:.*?desc:\s*"([^"]*)")?',
                links_text
            )
            if not entries:
                return m.group(0)

            see_also_parts = []
            for label, url, desc in entries:
                # Try to resolve URL back to a module ID
                # URL format: /v2/reference/audio-modules/.../moduleid
                slug = url.rstrip('/').split('/')[-1]
                module_id = url_to_id.get(slug)

                if module_id:
                    ref = f"$MODULES.{module_id}$"
                elif '/scripting-api/' in url:
                    # API reference link
                    parts = url.split('/')
                    class_slug = parts[-1].split('#')[0] if '#' in parts[-1] else parts[-1]
                    method_anchor = parts[-1].split('#')[1] if '#' in parts[-1] else None
                    # We don't have reverse class lookup here, use slug as-is
                    ref = f"$API.{class_slug}$"
                    if method_anchor:
                        ref = f"$API.{class_slug}.{method_anchor}$"
                else:
                    # Unknown URL format, keep as markdown link
                    ref = f"[{label}]({url})"

                if desc:
                    see_also_parts.append(f"{ref} -- {desc}")
                else:
                    see_also_parts.append(ref)

            return "**See also:** " + ", ".join(see_also_parts)

        content = see_also_pattern.sub(replace_see_also_block, content)

        # Also remove the ## See Also heading if it now just contains a **See also:** line
        content = re.sub(
            r'^## See Also\n\n(\*\*See also:\*\*)',
            r'\1',
            content,
            flags=re.MULTILINE
        )

        if content != original:
            total_changes += 1
            if dry_run:
                print(f"  [DRY] {md_file.relative_to(SCRIPT_DIR)}: see-also converted")
            else:
                with open(md_file, "w", encoding="utf-8") as f:
                    f.write(content)
                print(f"  {md_file.relative_to(SCRIPT_DIR)}: see-also converted")

    return total_changes


# ---------------------------------------------------------------------------
# API class lookup (for reverse-resolving /v2/scripting-api/ URLs)
# ---------------------------------------------------------------------------

def build_api_slug_to_class(base_dir: Path) -> dict:
    """Build a reverse mapping from URL slugs to canonical API class names."""
    if not base_dir.is_dir():
        return {}

    slug_to_class = {}
    for json_path in sorted(base_dir.glob("*.json")):
        with open(json_path, "r", encoding="utf-8") as f:
            data = json.load(f)
        class_name = data["className"]
        slug_to_class[class_name.lower()] = class_name
    return slug_to_class


# ---------------------------------------------------------------------------
# Static module files: hardcoded URL links
# ---------------------------------------------------------------------------

def backport_static_links(dry_run: bool) -> int:
    """Convert hardcoded Nuxt URLs to $DOMAIN$ tokens in static module files.

    Converts:
      [Label](/v2/reference/audio-modules/.../slug)  ->  [Label]($MODULES.ModuleId$)
      [Label](/v2/scripting-api/classname#method)     ->  [Label]($API.ClassName.method$)
      [Label](/v2/scripting-api/classname)            ->  [Label]($API.ClassName$)
    """
    if not MODULE_STATIC_DIR.is_dir():
        print("  Static module directory not found, skipping")
        return 0

    url_to_id = build_module_url_to_id(MODULE_LIST_PATH)
    slug_to_class = build_api_slug_to_class(ENRICHMENT_BASE_DIR)
    total_changes = 0

    for md_file in sorted(MODULE_STATIC_DIR.rglob("*.md")):
        with open(md_file, "r", encoding="utf-8") as f:
            content = f.read()

        original = content

        def replace_link(m):
            label = m.group(1)
            url = m.group(2)

            # Module links: /v2/reference/audio-modules/.../slug
            if "/v2/reference/audio-modules/" in url:
                slug = url.rstrip("/").split("/")[-1]
                module_id = url_to_id.get(slug)
                if module_id:
                    return f"[{label}]($MODULES.{module_id}$)"
                # Unknown module slug, leave unchanged
                return m.group(0)

            # API links: /v2/scripting-api/classname#method or /v2/scripting-api/classname
            if "/v2/scripting-api/" in url:
                path_part = url.split("/v2/scripting-api/")[-1]
                if "#" in path_part:
                    class_slug, method = path_part.split("#", 1)
                    class_name = slug_to_class.get(class_slug, class_slug)
                    return f"[{label}]($API.{class_name}.{method}$)"
                else:
                    class_slug = path_part.rstrip("/")
                    class_name = slug_to_class.get(class_slug, class_slug)
                    return f"[{label}]($API.{class_name}$)"

            # Other URLs, leave unchanged
            return m.group(0)

        # Match markdown links [text](url) but not image links ![text](url)
        content = re.sub(
            r'(?<!!)\[([^\]]+)\]\((/v2/[^)]+)\)',
            replace_link,
            content
        )

        if content != original:
            changes = sum(1 for a, b in zip(
                original.split('\n'), content.split('\n')
            ) if a != b)
            total_changes += changes
            if dry_run:
                print(f"  [DRY] {md_file.relative_to(SCRIPT_DIR)}: {changes} links converted")
            else:
                with open(md_file, "w", encoding="utf-8") as f:
                    f.write(content)
                print(f"  {md_file.relative_to(SCRIPT_DIR)}: {changes} links converted")

    return total_changes


# ---------------------------------------------------------------------------
# Phase 4: Common Mistake Titles
# ---------------------------------------------------------------------------

COMMON_MISTAKE_PLACEHOLDER = "$COMMON_MISTAKE_TITLE_TO_BE_REPLACED$"


def backport_phase4_common_mistakes(dry_run: bool) -> int:
    """Add title placeholders to common mistake entries in phase4 Readme.md files.

    Converts:
      - **Wrong:** text        ->  - **$COMMON_MISTAKE_TITLE_TO_BE_REPLACED$**
                                     **Wrong:** text
    """
    total_changes = 0

    for phase4_dir in (PHASE4_AUTO_DIR, PHASE4_MANUAL_DIR):
        if not phase4_dir.is_dir():
            continue
        for md_file in sorted(phase4_dir.rglob("Readme.md")):
            with open(md_file, "r", encoding="utf-8") as f:
                content = f.read()

            original = content

            # Convert: - **Wrong:** -> - **$PLACEHOLDER$**\n  **Wrong:**
            content = re.sub(
                r'^- \*\*Wrong:\*\*',
                f'- **{COMMON_MISTAKE_PLACEHOLDER}**\n  **Wrong:**',
                content,
                flags=re.MULTILINE
            )

            if content != original:
                changes = content.count(COMMON_MISTAKE_PLACEHOLDER)
                total_changes += changes
                if dry_run:
                    print(f"  [DRY] {md_file.relative_to(SCRIPT_DIR)}: {changes} mistakes titled")
                else:
                    with open(md_file, "w", encoding="utf-8") as f:
                        f.write(content)
                    print(f"  {md_file.relative_to(SCRIPT_DIR)}: {changes} mistakes titled")

    return total_changes


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description="Backport existing link formats to canonical $DOMAIN$ tokens",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument(
        "--dry-run", action="store_true",
        help="Preview changes without writing files",
    )
    parser.add_argument(
        "--target", choices=["phase1", "phase4", "modules", "mistakes", "static", "all"],
        default="all",
        help="Which source files to process (default: all)",
    )
    args = parser.parse_args()

    print("Backporting links to canonical $DOMAIN$ format...\n")

    total = 0

    if args.target in ("all", "phase1"):
        print("=== Phase 1: Cross References ===")
        n = backport_phase1_crossrefs(args.dry_run)
        print(f"  Total: {n} lines changed\n")
        total += n

    if args.target in ("all", "phase4"):
        print("=== Phase 4: Warnings ===")
        n = backport_phase4_warnings(args.dry_run)
        print(f"  Total: {n} warnings converted\n")
        total += n

    if args.target in ("all", "mistakes"):
        print("=== Phase 4: Common Mistake Titles ===")
        n = backport_phase4_common_mistakes(args.dry_run)
        print(f"  Total: {n} mistakes titled\n")
        total += n

    if args.target in ("all", "static"):
        print("=== Static Module Files: URL Links ===")
        n = backport_static_links(args.dry_run)
        print(f"  Total: {n} lines converted\n")
        total += n

    if args.target in ("all", "modules"):
        print("=== Module Pages: See Also ===")
        n = backport_module_see_also(args.dry_run)
        print(f"  Total: {n} files converted\n")
        total += n

    if args.dry_run:
        print(f"DRY RUN complete. {total} changes would be made.")
    else:
        print(f"Backport complete. {total} changes applied.")
        print("Review with: git diff")


if __name__ == "__main__":
    main()
