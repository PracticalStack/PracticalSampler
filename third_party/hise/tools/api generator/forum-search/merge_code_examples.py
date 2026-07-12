#!/usr/bin/env python3
"""
Merge, deduplicate, and format all forum code example batch files.

Collects batch_*.json and snippet_batch_*.json from code_examples/,
deduplicates by normalized code hash, formats each code block with
astyle, and writes a single output file.

Usage:
    python3 merge_code_examples.py [--output code_examples/forum_examples.json]
"""

import argparse
import hashlib
import json
import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path

SCRIPT_DIR = Path(__file__).parent
CODE_EXAMPLES_DIR = SCRIPT_DIR / "code_examples"
VALIDATED_DIR = CODE_EXAMPLES_DIR / "validated"
ASTYLE_BIN = Path.home() / "HISE" / "tools" / "astyle" / "astyle_macos"
ASTYLE_RC = Path.home() / "HISE" / "tools" / "astyle" / "astylerc.sh"


def collect_batches(from_validated=False):
    """Collect all batch and snippet_batch JSON files (exclude metadata/fix files)."""
    source_dir = VALIDATED_DIR if from_validated else CODE_EXAMPLES_DIR
    import re
    pattern = re.compile(r'^(batch_\d+|snippet_batch_\d+)\.json$')
    files = sorted(f for f in source_dir.glob("*.json") if pattern.match(f.name))
    return files


def format_with_astyle(code):
    """Format a code string with astyle, returning the formatted version."""
    if not ASTYLE_BIN.exists():
        return code

    tmp = tempfile.NamedTemporaryFile(
        suffix='.js', mode='w', encoding='utf-8', delete=False
    )
    try:
        tmp.write(code)
        tmp.close()
        subprocess.run(
            [str(ASTYLE_BIN), f"--options={ASTYLE_RC}", tmp.name],
            capture_output=True, timeout=10
        )
        with open(tmp.name, "r", encoding="utf-8") as f:
            formatted = f.read()
        # astyle creates a .orig backup
        orig = Path(tmp.name + ".orig")
        if orig.exists():
            orig.unlink()
        return formatted.rstrip()
    except Exception as e:
        print(f"[warn] astyle failed: {e}", file=sys.stderr)
        return code
    finally:
        try:
            os.unlink(tmp.name)
        except OSError:
            pass


def code_hash(code):
    """Normalize whitespace and hash for deduplication."""
    normalized = re.sub(r'\s+', ' ', code).strip()
    return hashlib.md5(normalized.encode('utf-8')).hexdigest()


def main():
    parser = argparse.ArgumentParser(
        description="Merge, deduplicate, and format forum code examples"
    )
    parser.add_argument(
        "--output",
        default=str(SCRIPT_DIR / "output" / "forum_examples.json"),
        help="Output file path (default: forum-search/output/forum_examples.json)"
    )
    parser.add_argument(
        "--no-format", action="store_true",
        help="Skip astyle formatting (faster, for testing)"
    )
    parser.add_argument(
        "--from-validated", action="store_true",
        help="Read from validated/ dir, filter failures, add validated flag"
    )
    args = parser.parse_args()

    # Collect
    batch_files = collect_batches(from_validated=args.from_validated)
    if not batch_files:
        print("No batch files found in code_examples/", file=sys.stderr)
        sys.exit(1)

    print(f"Found {len(batch_files)} batch files:", file=sys.stderr)
    for f in batch_files:
        print(f"  {f.name}", file=sys.stderr)

    # Merge
    all_blocks = []
    for f in batch_files:
        with open(f, "r", encoding="utf-8") as fh:
            blocks = json.load(fh)
            all_blocks.extend(blocks)

    total_input = len(all_blocks)
    print(f"Total examples: {total_input}", file=sys.stderr)

    # Deduplicate
    seen_hashes = set()
    unique_blocks = []
    for block in all_blocks:
        h = code_hash(block["code"])
        if h in seen_hashes:
            continue
        seen_hashes.add(h)
        unique_blocks.append(block)

    duplicates = total_input - len(unique_blocks)
    print(f"Duplicates removed: {duplicates}", file=sys.stderr)

    # Filter by validation results (if --from-validated)
    discarded = 0
    if args.from_validated:
        filtered_blocks = []
        for block in unique_blocks:
            v = block.get("validation", {})
            r = v.get("result", {})

            # Discard if tested, failed, and exhausted retries (>= 2 attempts)
            if r.get("tested") and not r.get("passed") and r.get("attempts", 0) >= 2:
                discarded += 1
                continue

            # Add validated flag
            if r.get("tested") and r.get("passed"):
                block["validated"] = True
            else:
                block["validated"] = False

            # Strip internal validation metadata from output
            block.pop("validation", None)
            filtered_blocks.append(block)

        unique_blocks = filtered_blocks
        print(f"Validation failures discarded: {discarded}", file=sys.stderr)
        print(f"After filtering: {len(unique_blocks)} examples", file=sys.stderr)

    # Format with astyle
    formatted_count = 0
    if not args.no_format:
        if not ASTYLE_BIN.exists():
            print(f"[warn] astyle not found at {ASTYLE_BIN}, skipping formatting",
                  file=sys.stderr)
        else:
            print(f"Formatting {len(unique_blocks)} code blocks with astyle...",
                  file=sys.stderr)
            for i, block in enumerate(unique_blocks):
                original = block["code"]
                block["code"] = format_with_astyle(original)
                if block["code"] != original:
                    formatted_count += 1
                if (i + 1) % 50 == 0:
                    print(f"  Formatted {i + 1}/{len(unique_blocks)}...",
                          file=sys.stderr)

    # Write output
    output_path = Path(args.output)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    with open(output_path, "w", encoding="utf-8") as f:
        json.dump(unique_blocks, f, indent=2, ensure_ascii=False)

    file_size = output_path.stat().st_size

    summary = {
        "status": "ok",
        "batch_files": len(batch_files),
        "total_input": total_input,
        "duplicates_removed": duplicates,
        "validation_discarded": discarded if args.from_validated else 0,
        "formatted": formatted_count,
        "output_count": len(unique_blocks),
        "output_file": str(output_path),
        "file_size_bytes": file_size
    }
    print(json.dumps(summary, indent=2, ensure_ascii=False))


if __name__ == "__main__":
    main()
