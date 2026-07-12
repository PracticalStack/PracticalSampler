"""
HISE API Class Survey - Merge Script

Combines 6 survey fragment JSONs into a single class_survey_data.json.
Derives: createdBy (inverse of creates), fanIn/fanOut, validates seeAlso.

Usage:
    python survey_merge.py
"""

import json
from pathlib import Path
from collections import defaultdict

SCRIPT_DIR = Path(__file__).parent
FRAGMENTS_DIR = SCRIPT_DIR / "fragments"
OUTPUT_PATH = SCRIPT_DIR / "class_survey_data.json"

FRAGMENTS = [
    "survey_ui.json",
    "survey_module_tree.json",
    "survey_data.json",
    "survey_scriptnode.json",
    "survey_event.json",
    "survey_services.json",
]

GROUP_NAMES = {
    "survey_ui.json": "ui",
    "survey_module_tree.json": "module-tree",
    "survey_data.json": "data",
    "survey_scriptnode.json": "scriptnode",
    "survey_event.json": "event",
    "survey_services.json": "services",
}


def main():
    # --- Load and merge all fragments ---
    classes = {}
    class_to_group = {}

    for frag_name in FRAGMENTS:
        path = FRAGMENTS_DIR / frag_name
        if not path.exists():
            print(f"WARNING: {frag_name} not found, skipping")
            continue
        data = json.load(open(path, "r"))
        group = GROUP_NAMES[frag_name]
        for cls_name, cls_data in data.items():
            if cls_name in classes:
                print(f"WARNING: Duplicate class {cls_name} "
                      f"(in {class_to_group[cls_name]} and {group})")
            classes[cls_name] = cls_data
            class_to_group[cls_name] = group

    all_class_names = set(classes.keys())
    print(f"Merged {len(classes)} classes from {len(FRAGMENTS)} fragments")

    # --- Derive createdBy (inverse of creates) ---
    for cls_name, cls_data in classes.items():
        cls_data["createdBy"] = []

    for cls_name, cls_data in classes.items():
        for created in cls_data.get("creates", []):
            if created in classes:
                # Find the method that creates it
                classes[created]["createdBy"].append(cls_name)

    # Deduplicate createdBy
    for cls_data in classes.values():
        cls_data["createdBy"] = sorted(set(cls_data["createdBy"]))

    # --- Compute fanOut and fanIn ---
    # fanOut: number of distinct classes this one creates or references
    # fanIn: number of distinct classes that create or reference this one
    incoming = defaultdict(set)

    for cls_name, cls_data in classes.items():
        outgoing = set()
        for c in cls_data.get("creates", []):
            if c in all_class_names and c != cls_name:
                outgoing.add(c)
        for r in cls_data.get("references", []):
            if r in all_class_names and r != cls_name:
                outgoing.add(r)
        cls_data["_fanOutRaw"] = len(outgoing)

        for target in outgoing:
            incoming[target].add(cls_name)

    for cls_name, cls_data in classes.items():
        cls_data["_fanInRaw"] = len(incoming.get(cls_name, set()))

    # Normalize
    max_fan_out = max(
        (c["_fanOutRaw"] for c in classes.values()), default=1) or 1
    max_fan_in = max(
        (c["_fanInRaw"] for c in classes.values()), default=1) or 1
    max_api_surface = 1  # We'll compute from method counts if available

    for cls_data in classes.values():
        cls_data["fanOut"] = round(cls_data["_fanOutRaw"] / max_fan_out, 2)
        cls_data["fanIn"] = round(cls_data["_fanInRaw"] / max_fan_in, 2)
        del cls_data["_fanOutRaw"]
        del cls_data["_fanInRaw"]

    # --- Validate seeAlso reciprocity ---
    see_also_issues = []
    for cls_name, cls_data in classes.items():
        for entry in cls_data.get("seeAlso", []):
            target = entry.get("class", "")
            if target not in all_class_names:
                see_also_issues.append(
                    f"  {cls_name} -> {target}: target class not found")
                continue
            # Check if target mentions cls_name in its seeAlso
            target_see_also = [
                e.get("class", "")
                for e in classes[target].get("seeAlso", [])
            ]
            if cls_name not in target_see_also:
                see_also_issues.append(
                    f"  {cls_name} -> {target}: not reciprocal "
                    f"({target} does not list {cls_name})")

    if see_also_issues:
        print(f"\nseeAlso reciprocity issues ({len(see_also_issues)}):")
        for issue in see_also_issues[:30]:
            print(issue)
        if len(see_also_issues) > 30:
            print(f"  ... and {len(see_also_issues) - 30} more")

    # --- Validate creates references ---
    creates_issues = []
    for cls_name, cls_data in classes.items():
        for created in cls_data.get("creates", []):
            if created not in all_class_names:
                creates_issues.append(
                    f"  {cls_name} creates {created}: not a known class")

    if creates_issues:
        print(f"\ncreates reference issues ({len(creates_issues)}):")
        for issue in creates_issues:
            print(issue)

    # --- Build clusters from groups ---
    clusters = {}
    for group_name in set(GROUP_NAMES.values()):
        members = [
            cls for cls, grp in class_to_group.items()
            if grp == group_name
        ]
        clusters[group_name] = {
            "members": sorted(members),
            "description": ""
        }

    # --- Build output ---
    output = {
        "meta": {
            "generatedAt": "2026-02-27",
            "classCount": len(classes),
            "groups": list(set(GROUP_NAMES.values())),
            "normalization": {
                "maxFanOut": max_fan_out,
                "maxFanIn": max_fan_in,
            }
        },
        "classes": dict(sorted(classes.items())),
        "clusters": clusters,
    }

    with open(OUTPUT_PATH, "w") as f:
        json.dump(output, f, indent=2)

    print(f"\nOutput written to {OUTPUT_PATH}")
    print(f"  {len(classes)} classes")
    print(f"  {len(clusters)} clusters")

    # --- Print summary stats ---
    print("\n=== Per-group summary ===")
    for group_name in sorted(set(GROUP_NAMES.values())):
        members = clusters[group_name]["members"]
        creates_count = sum(
            len(classes[m].get("creates", [])) for m in members)
        see_also_count = sum(
            len(classes[m].get("seeAlso", [])) for m in members)
        print(f"  {group_name:15s}: {len(members):2d} classes, "
              f"{creates_count:3d} creates, {see_also_count:3d} seeAlso")

    # --- Print createdBy chains for prerequisite analysis ---
    print("\n=== createdBy relationships ===")
    for cls_name in sorted(classes.keys()):
        created_by = classes[cls_name].get("createdBy", [])
        if created_by:
            print(f"  {cls_name} <- {', '.join(created_by)}")

    # --- Print cross-group creates ---
    print("\n=== Cross-group creates (factory relationships) ===")
    for cls_name, cls_data in sorted(classes.items()):
        src_group = class_to_group[cls_name]
        for created in cls_data.get("creates", []):
            if created in class_to_group:
                dst_group = class_to_group[created]
                if src_group != dst_group:
                    print(f"  {cls_name} ({src_group}) "
                          f"-> {created} ({dst_group})")


if __name__ == "__main__":
    main()
