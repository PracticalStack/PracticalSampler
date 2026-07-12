#!/usr/bin/env python3
"""
Publish pipeline for HISE documentation.

Collects markdown from all enrichment pipeline outputs, resolves canonical
link tokens ($DOMAIN.Target$), applies MDC (Markdown Components) transforms
for Nuxt.js, validates links and images, and writes to a target directory.

Link syntax:  [display text]($DOMAIN.Target#fragment$)
Domains:      API, MODULES, UI, SN, DOC

Usage:
    python publish.py [output_dir]              # default: CWD
    python publish.py content/v2 --strict       # CI mode: fail on broken links
    python publish.py --dry-run                 # resolve + validate only, no file writes

Configuration lives in site_structure.json (same directory as this script).
"""

import argparse
import difflib
import json
import os
import re
import shutil
import sys
from pathlib import Path

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------

SCRIPT_DIR = Path(__file__).resolve().parent
SITE_STRUCTURE_PATH = SCRIPT_DIR / "site_structure.json"

# Enrichment pipeline directories (relative to SCRIPT_DIR)
ENRICHMENT_BASE_DIR = SCRIPT_DIR / "enrichment" / "base"
ENRICHMENT_WEBSITE_DIR = SCRIPT_DIR / "enrichment" / "output" / "website"
ENRICHMENT_PREVIEW_DIR = SCRIPT_DIR / "enrichment" / "output" / "preview"
MODULE_ENRICHMENT_OUTPUT_DIR = SCRIPT_DIR / "module_enrichment" / "output"
MODULE_LIST_PATH = SCRIPT_DIR / "module_enrichment" / "base" / "moduleList.json"


# ---------------------------------------------------------------------------
# Registry: builds the set of valid link targets for each domain
# ---------------------------------------------------------------------------

class LinkRegistry:
    """Holds all valid link targets, keyed by domain.

    Each domain maps canonical names (case-preserved) to URL paths.
    A parallel lowercase index enables case-insensitive lookup.
    """

    def __init__(self, site_structure: dict, script_dir: Path):
        self.structure = site_structure
        self.script_dir = script_dir
        # domain -> { canonical_key: url_path }
        self.targets = {}
        # domain -> { lowercase_key: canonical_key }
        self.lower_index = {}
        # domain -> { lowercase_key: canonical_key } (dash-stripped for DOC)
        self.normalized_index = {}
        # module_id -> { prettyName, type, subtype }
        self.module_info = {}

        self._build_api_registry()
        self._build_modules_registry()
        self._build_sn_registry()
        self._build_doc_registry()
        self._build_ui_registry()
        self._build_lang_registry()

    def _build_api_registry(self):
        """Build API domain registry from enrichment/base/*.json."""
        targets = {}
        base_path = self.structure["domains"]["API"]["basePath"]
        base_dir = self.script_dir / "enrichment" / "base"

        if not base_dir.is_dir():
            self.targets["API"] = {}
            self.lower_index["API"] = {}
            self.normalized_index["API"] = {}
            return

        for json_path in sorted(base_dir.glob("*.json")):
            with open(json_path, "r", encoding="utf-8") as f:
                data = json.load(f)
            class_name = data["className"]
            slug = class_name.lower()

            # Class-level target: $API.ClassName$
            targets[class_name] = f"{base_path}/{slug}"

            # Method-level targets: $API.ClassName.method$
            for method_name in data.get("methods", {}).keys():
                key = f"{class_name}.{method_name}"
                targets[key] = f"{base_path}/{slug}#{method_name.lower()}"

        self.targets["API"] = targets
        self._build_indexes("API")

    def _build_modules_registry(self):
        """Build MODULES domain registry from moduleList.json."""
        targets = {}
        domain_config = self.structure["domains"]["MODULES"]
        base_path = domain_config["basePath"]
        type_mapping = domain_config.get("typeMapping", {})

        if not MODULE_LIST_PATH.is_file():
            self.targets["MODULES"] = {}
            self.lower_index["MODULES"] = {}
            self.normalized_index["MODULES"] = {}
            return

        with open(MODULE_LIST_PATH, "r", encoding="utf-8") as f:
            module_list = json.load(f)

        for module in module_list.get("modules", []):
            module_id = module["id"]
            module_type = module.get("type", "")
            module_subtype = module.get("subtype", "")

            self.module_info[module_id] = {
                "prettyName": module.get("prettyName", module_id),
                "type": module_type,
                "subtype": module_subtype,
            }

            # Find the directory mapping
            # Try type.subtype first, then type alone
            type_key = f"{module_type}.{module_subtype}" if module_subtype else module_type
            dir_path = type_mapping.get(type_key)
            if dir_path is None:
                dir_path = type_mapping.get(module_type)
            if dir_path is None:
                continue  # Unknown type, skip

            # Nuxt strips number prefixes from URLs
            url_segments = []
            for segment in dir_path.split("/"):
                # Strip leading "NN." prefix
                clean = re.sub(r"^\d+\.", "", segment)
                url_segments.append(clean)
            url_dir = "/".join(url_segments)

            slug = module_id.lower()
            targets[module_id] = f"{base_path}/{url_dir}/{slug}"

            # Parameter-level targets: $MODULES.ModuleId.ParamId$
            for param in module.get("parameters", []):
                param_id = param.get("id", "")
                if param_id:
                    key = f"{module_id}.{param_id}"
                    targets[key] = f"{base_path}/{url_dir}/{slug}#{param_id.lower()}"

        self.targets["MODULES"] = targets
        self._build_indexes("MODULES")

    def _build_sn_registry(self):
        """Build SN domain registry from scriptnodeList.json."""
        targets = {}
        if "SN" not in self.structure.get("domains", {}):
            self.targets["SN"] = {}
            self.lower_index["SN"] = {}
            self.normalized_index["SN"] = {}
            return

        domain_config = self.structure["domains"]["SN"]
        base_path = domain_config["basePath"]
        registry_source = domain_config.get("registrySource", "")
        sn_list_path = self.script_dir / registry_source

        if not sn_list_path.is_file():
            self.targets["SN"] = {}
            self.lower_index["SN"] = {}
            self.normalized_index["SN"] = {}
            return

        with open(sn_list_path, "r", encoding="utf-8") as f:
            sn_data = json.load(f)

        # scriptnodeList.json is a dict keyed by "factory.node"
        for factory_path, node_data in sn_data.items():
            if "." not in factory_path:
                continue
            factory, node_id = factory_path.split(".", 1)
            # Node-level target: $SN.factory.node$
            targets[factory_path] = f"{base_path}/{factory}/{node_id}"
            # Factory-level target: $SN.factory$
            if factory not in targets:
                targets[factory] = f"{base_path}/{factory}"

        self.targets["SN"] = targets
        self._build_indexes("SN")

    def _build_doc_registry(self):
        """Build DOC domain registry by scanning the content directory.

        Since DOC pages use numbered prefixes (01.overview.md) that Nuxt
        strips, we scan the actual content dirs and register slug targets.
        """
        targets = {}
        domain_config = self.structure["domains"]["DOC"]
        base_path = domain_config["basePath"]
        subdomains = domain_config.get("subdomains", {})

        # Try to find the content root -- check common locations
        content_roots = [
            self.script_dir / ".." / ".." / ".." / "hise_website_v2" / "content" / "v2",
            Path("D:/Development/Projekte/hise_website_v2/content/v2"),
        ]

        content_root = None
        for root in content_roots:
            if root.is_dir():
                content_root = root.resolve()
                break

        if content_root is None:
            # Can't scan -- register empty
            self.targets["DOC"] = {}
            self.lower_index["DOC"] = {}
            self.normalized_index["DOC"] = {}
            return

        for subdomain_name, dir_name in subdomains.items():
            section_dir = content_root / dir_name
            if not section_dir.is_dir():
                continue

            for md_file in section_dir.rglob("*.md"):
                if md_file.name.startswith("_") or md_file.name.startswith("."):
                    continue
                if md_file.name.lower() == "readme.md":
                    continue

                # Build the slug by stripping number prefixes from each segment
                rel = md_file.relative_to(section_dir)
                slug_parts = []
                for part in rel.parts:
                    # Strip .md extension from filename
                    name = part
                    if name.endswith(".md"):
                        name = name[:-3]
                    # Strip leading "NN." prefix
                    name = re.sub(r"^\d+\.", "", name)
                    if name and name.lower() != "index":
                        slug_parts.append(name)

                if not slug_parts:
                    continue

                slug = "/".join(slug_parts)
                # Key format: SubdomainName.SlugPart1.SlugPart2
                # e.g., Architecture.data-model or Guide.building-a-sampler
                key_parts = [subdomain_name] + slug_parts
                key = ".".join(key_parts)
                targets[key] = f"{base_path}/{dir_name}/{slug}"

        self.targets["DOC"] = targets
        self._build_indexes("DOC")

    def _build_ui_registry(self):
        """Build UI domain registry from ui_enrichment/pages/ subdirectories."""
        targets = {}
        domain_config = self.structure.get("domains", {}).get("UI", {})
        base_path = domain_config.get("basePath", "/v2/reference/ui-components")
        pages_config = domain_config.get("sources", {}).get("pages", {})

        if not isinstance(pages_config, dict):
            self.targets["UI"] = {}
            self.lower_index["UI"] = {}
            self.normalized_index["UI"] = {}
            return

        for subdir_name, subdir_path in pages_config.items():
            pages_dir = self.script_dir / subdir_path
            if not pages_dir.is_dir():
                continue
            for md_file in sorted(pages_dir.glob("*.md")):
                if md_file.name.lower() == "readme.md":
                    continue
                # Read componentId or contentType from frontmatter
                with open(md_file, "r", encoding="utf-8") as f:
                    content = f.read()
                key = (_extract_frontmatter_field(content, "componentId") or
                       _extract_frontmatter_field(content, "contentType") or
                       md_file.stem)
                slug = md_file.stem.lower()
                targets[key] = f"{base_path}/{subdir_name}/{slug}"

        self.targets["UI"] = targets
        self._build_indexes("UI")

    def _build_lang_registry(self):
        """Build LANG domain registry from language_enrichment/output/*.md."""
        targets = {}
        domain_config = self.structure.get("domains", {}).get("LANG", {})
        if not domain_config:
            self.targets["LANG"] = {}
            self.lower_index["LANG"] = {}
            self.normalized_index["LANG"] = {}
            return

        base_path = domain_config["basePath"]
        pages_path = domain_config.get("sources", {}).get("pages", "")
        pages_dir = self.script_dir / pages_path

        if not pages_dir.is_dir():
            self.targets["LANG"] = {}
            self.lower_index["LANG"] = {}
            self.normalized_index["LANG"] = {}
            return

        for md_file in sorted(pages_dir.glob("*.md")):
            if md_file.name.lower() == "readme.md":
                continue
            stem = md_file.stem
            targets[stem] = f"{base_path}/{stem}"

        self.targets["LANG"] = targets
        self._build_indexes("LANG")

    def _build_indexes(self, domain: str):
        """Build case-insensitive and normalized indexes for a domain."""
        lower = {}
        normalized = {}
        for key in self.targets[domain]:
            lk = key.lower()
            lower[lk] = key
            # Normalized: lowercase + strip dashes (for DOC fuzzy matching)
            nk = lk.replace("-", "").replace("_", "")
            normalized[nk] = key
        self.lower_index[domain] = lower
        self.normalized_index[domain] = normalized

    def resolve(self, domain: str, target: str, fragment: str = None):
        """Resolve a link target within a domain.

        Returns (url, canonical_key, match_type) where match_type is one of:
        - "exact": exact match
        - "case": case-insensitive match
        - "normalized": dash/underscore-insensitive match
        - "fuzzy": difflib fuzzy match
        - None: no match found
        """
        if domain not in self.targets:
            return None, None, None

        targets = self.targets[domain]

        # 1. Exact match
        if target in targets:
            url = targets[target]
            if fragment:
                url = f"{url}#{fragment}" if "#" not in url else url
            return url, target, "exact"

        # 2. Case-insensitive exact match
        lower_idx = self.lower_index.get(domain, {})
        lower_target = target.lower()
        if lower_target in lower_idx:
            canonical = lower_idx[lower_target]
            url = targets[canonical]
            if fragment:
                url = f"{url}#{fragment}" if "#" not in url else url
            return url, canonical, "case"

        # 3. Normalized match (strip dashes, underscores)
        norm_idx = self.normalized_index.get(domain, {})
        norm_target = lower_target.replace("-", "").replace("_", "")
        if norm_target in norm_idx:
            canonical = norm_idx[norm_target]
            url = targets[canonical]
            if fragment:
                url = f"{url}#{fragment}" if "#" not in url else url
            return url, canonical, "normalized"

        # 4. Fuzzy match using difflib
        all_keys = list(targets.keys())
        # Compare lowercase for better fuzzy matching
        all_lower = [k.lower() for k in all_keys]
        matches = difflib.get_close_matches(lower_target, all_lower, n=1, cutoff=0.6)
        if matches:
            # Find the original-case key
            match_idx = all_lower.index(matches[0])
            canonical = all_keys[match_idx]
            url = targets[canonical]
            if fragment:
                url = f"{url}#{fragment}" if "#" not in url else url
            return url, canonical, "fuzzy"

        return None, None, None

    def resolve_compound(self, domain: str, parts: list, fragment: str = None):
        """Resolve a compound target like API.ClassName.method by trying
        progressively from full compound down to class-only.

        For API: try "ClassName.method" first, then "ClassName"
        For MODULES: try "ModuleId.ParamId" first, then "ModuleId"
        For DOC: try "Subdomain.slug1.slug2" as full key
        """
        # Try full compound key
        full_key = ".".join(parts)
        url, canonical, match_type = self.resolve(domain, full_key, fragment)
        if url:
            return url, canonical, match_type

        # For API/MODULES: if we have ClassName.method, try resolving
        # the class first (fuzzy), then the method within that class
        if domain in ("API", "MODULES") and len(parts) >= 2:
            # Resolve the class/module part
            class_target = parts[0]
            class_url, class_canonical, class_match = self.resolve(domain, class_target)

            if class_canonical:
                # Now try the full key with the corrected class name
                method_part = ".".join(parts[1:])
                corrected_key = f"{class_canonical}.{method_part}"
                url, canonical, match_type = self.resolve(domain, corrected_key, fragment)
                if url:
                    # The match type is the "worst" of the two resolutions
                    worst = _worst_match(class_match, match_type)
                    return url, canonical, worst

                # Try fuzzy on just the method within the class
                class_prefix = f"{class_canonical}."
                class_methods = {
                    k: v for k, v in self.targets[domain].items()
                    if k.startswith(class_prefix)
                }
                if class_methods:
                    method_keys = list(class_methods.keys())
                    method_lower = [k.lower() for k in method_keys]
                    target_lower = corrected_key.lower()
                    matches = difflib.get_close_matches(
                        target_lower, method_lower, n=1, cutoff=0.6
                    )
                    if matches:
                        match_idx = method_lower.index(matches[0])
                        canonical = method_keys[match_idx]
                        url = class_methods[canonical]
                        if fragment:
                            url = f"{url}#{fragment}" if "#" not in url else url
                        return url, canonical, "fuzzy"

        return None, None, None


def _worst_match(*match_types):
    """Return the least precise match type from a set."""
    order = {"exact": 0, "case": 1, "normalized": 2, "fuzzy": 3}
    worst = max(match_types, key=lambda m: order.get(m, 99))
    return worst


# ---------------------------------------------------------------------------
# Link token resolution
# ---------------------------------------------------------------------------

# Pattern: $DOMAIN.target.parts#fragment$ inside markdown links or standalone
LINK_TOKEN_PATTERN = re.compile(
    r'\$([A-Z]+)\.([^$#]+?)(?:#([^$]+))?\$'
)


def resolve_tokens(content: str, registry: LinkRegistry, filepath: str,
                   messages: list) -> str:
    """Replace all $DOMAIN.target$ tokens in content with resolved URLs.

    For tokens inside markdown links [text]($...$), replaces the URL part.
    For standalone $...$ tokens, replaces with the URL.
    """

    def _replace_token(m):
        domain = m.group(1)
        target_str = m.group(2)
        fragment = m.group(3)  # may be None

        parts = target_str.split(".")
        url, canonical, match_type = registry.resolve_compound(
            domain, parts, fragment
        )

        if url is None:
            messages.append({
                "level": "ERROR",
                "file": filepath,
                "token": m.group(0),
                "message": f"Unresolved link: {m.group(0)}"
            })
            # Leave the token as-is so it's visible in output
            return m.group(0)

        original_token = f"${domain}.{target_str}$"
        canonical_token = f"${domain}.{canonical}$"

        if match_type == "case":
            messages.append({
                "level": "INFO",
                "file": filepath,
                "token": original_token,
                "message": f"Case fix: {original_token} -> {canonical_token}"
            })
        elif match_type == "normalized":
            messages.append({
                "level": "INFO",
                "file": filepath,
                "token": original_token,
                "message": f"Normalized: {original_token} -> {canonical_token}"
            })
        elif match_type == "fuzzy":
            messages.append({
                "level": "WARN",
                "file": filepath,
                "token": original_token,
                "message": f"Fuzzy match: {original_token} -> {canonical_token}"
            })

        return url

    # Process markdown links: [text]($DOMAIN.target$)
    def _replace_md_link(m):
        before = m.group(1)  # [text](
        token_match = LINK_TOKEN_PATTERN.search(m.group(2))
        after = m.group(3)   # )
        if token_match:
            resolved = _replace_token(token_match)
            return f"{before}{resolved}{after}"
        return m.group(0)

    # First pass: resolve tokens inside markdown links
    content = re.sub(
        r'(\[[^\]]*\]\()(\$[A-Z]+\.[^$]+\$)(\))',
        _replace_md_link,
        content
    )

    # Second pass: resolve any remaining standalone tokens
    # (e.g., in see-also lines, YAML blocks)
    content = LINK_TOKEN_PATTERN.sub(_replace_token, content)

    return content


# ---------------------------------------------------------------------------
# MDC transforms (absorbed from postprocess_md.py)
# ---------------------------------------------------------------------------

def get_class_name_from_heading(lines):
    """Extract class name from the first # heading."""
    for line in lines:
        if line.startswith("# "):
            return line[2:].strip()
    return ""


def convert_h1_to_frontmatter(content):
    """Convert '# ClassName\\n*category*\\n\\nbrief\\n' to YAML frontmatter."""
    m = re.match(r'^# (.+)\n\*(\w+)\*\n\n(.+)\n\n', content)
    if not m:
        return content
    title = m.group(1)
    category = m.group(2)
    description = m.group(3)
    frontmatter = (f"---\ntitle: {title}\ncategory: {category}\n"
                   f"description: \"{description}\"\n---\n\n")
    return frontmatter + content[m.end():]


def convert_warning_blockquotes(content, messages=None, filepath=""):
    """Convert warning blockquotes to ::warning components.

    Supports two formats:
    1. New titled format:  > [!Warning:Title here] text...
    2. Legacy format:      > **Warning:** text...

    The placeholder $WARNING_TO_BE_REPLACED$ is treated as "no title" and
    emits an INFO message so untitled warnings can be found.
    """
    PLACEHOLDER = "$WARNING_TO_BE_REPLACED$"

    # New format: > [!Warning:title] text
    def replace_titled_warning(m):
        title = m.group(1).strip()
        text = m.group(2).strip()
        if title == PLACEHOLDER:
            if messages is not None:
                messages.append({
                    "level": "INFO",
                    "file": filepath,
                    "token": PLACEHOLDER,
                    "message": f"Untitled warning (needs title): {text[:60]}..."
                })
            return f"::warning\n{text}\n::"
        return f'::warning{{title="{title}"}}\n{text}\n::'

    content = re.sub(
        r'^> \[!Warning:([^\]]+)\]\s*(.+?)$',
        replace_titled_warning, content, flags=re.MULTILINE
    )

    # Legacy format: > **Warning:** text
    pattern = r'^> \*\*Warning:\*\* (.+?)$'

    def replace_warning(m):
        text = m.group(1)
        return f"::warning\n{text}\n::"

    return re.sub(pattern, replace_warning, content, flags=re.MULTILINE)


def convert_tip_blockquotes(content, messages=None, filepath=""):
    """Convert tip blockquotes (before ## Methods) to ::tip.

    Supports two formats:
    1. New titled format:  > [!Tip:Title here] text...
    2. Legacy format:      > plain blockquote text (not a warning)

    The placeholder $TIP_TO_BE_REPLACED$ is treated as "no title".
    """
    PLACEHOLDER = "$TIP_TO_BE_REPLACED$"

    # First pass: handle new titled format anywhere in the document
    def replace_titled_tip(m):
        title = m.group(1).strip()
        text = m.group(2).strip()
        if title == PLACEHOLDER:
            if messages is not None:
                messages.append({
                    "level": "INFO",
                    "file": filepath,
                    "token": PLACEHOLDER,
                    "message": f"Untitled tip (needs title): {text[:60]}..."
                })
            return f"::tip\n{text}\n::"
        return f'::tip{{title="{title}"}}\n{text}\n::'

    content = re.sub(
        r'^> \[!Tip:([^\]]+)\]\s*(.+?)$',
        replace_titled_tip, content, flags=re.MULTILINE
    )

    # Second pass: legacy plain blockquotes (before ## Methods only)
    lines = content.split('\n')
    result = []
    in_methods = False
    i = 0
    while i < len(lines):
        line = lines[i]
        if line.startswith("## Methods"):
            in_methods = True
        if (not in_methods and line.startswith("> ")
                and not line.startswith("> **Warning:")
                and not line.startswith("> **Warning*")
                and not line.startswith("> [!Warning:")
                and not line.startswith("> [!Tip:")):
            bq_lines = []
            while i < len(lines) and lines[i].startswith("> "):
                bq_lines.append(lines[i][2:])
                i += 1
            text = "\n".join(bq_lines)
            result.append(f"::tip\n{text}\n::")
            continue
        result.append(line)
        i += 1
    return "\n".join(result)


def convert_see_also(content, class_name, registry=None):
    """Convert **See also:** lines to ::see-also MDC components.

    Parses $API.Class.method$ and $MODULES.ModuleId$ tokens directly
    (before link resolution).
    Same-class references become anchor-only links (#methodname).
    Cross-class references link to /v2/scripting-api/classname#methodname.
    Module references use registry for URL and prettyName for label.
    Annotated refs use: $DOMAIN.Target$ -- description text
    """
    slug = class_name.lower()
    pattern = r'^\*\*See also:\*\* (.+)$'

    def replace_see_also(m):
        line = m.group(1)
        # Split on comma followed by whitespace and a $ token or ClassName
        items = re.split(r',\s*(?=\$[A-Z]|\$[a-z]|[A-Z]\w*\.)', line)
        if not items:
            return m.group(0)

        yaml_links = []
        for item in items:
            item = item.strip().rstrip(',')
            if not item:
                continue

            # Parse $API.Class.method$ tokens (with optional annotation)
            token_ann = re.match(
                r'\$API\.([A-Za-z]\w*)\.(\w+)\$\s*--\s*(.+)', item, re.DOTALL
            )
            token_plain = re.match(
                r'\$API\.([A-Za-z]\w*)\.(\w+)\$', item
            )

            if token_ann:
                cls = token_ann.group(1)
                method = token_ann.group(2)
                desc = token_ann.group(3).strip().replace('"', '\\"')
                if cls.lower() == slug:
                    yaml_links.append(
                        f'  - {{ label: "{method}", to: "#{method.lower()}", desc: "{desc}" }}'
                    )
                else:
                    yaml_links.append(
                        f'  - {{ label: "{cls}.{method}", to: "/v2/scripting-api/{cls.lower()}#{method.lower()}", desc: "{desc}" }}'
                    )
            elif token_plain:
                cls = token_plain.group(1)
                method = token_plain.group(2)
                if cls.lower() == slug:
                    yaml_links.append(
                        f'  - {{ label: "{method}", to: "#{method.lower()}" }}'
                    )
                else:
                    yaml_links.append(
                        f'  - {{ label: "{cls}.{method}", to: "/v2/scripting-api/{cls.lower()}#{method.lower()}" }}'
                    )

            # Parse bare $API.ClassName$ tokens (no method, with optional annotation)
            elif re.match(r'\$API\.([A-Za-z]\w*)\$', item):
                api_cls_ann = re.match(
                    r'\$API\.([A-Za-z]\w*)\$\s*--\s*(.+)', item, re.DOTALL
                )
                api_cls_plain = re.match(r'\$API\.([A-Za-z]\w*)\$', item)
                api_cls_match = api_cls_ann or api_cls_plain
                cls = api_cls_match.group(1)
                desc = api_cls_ann.group(2).strip().replace('"', '\\"') if api_cls_ann else None

                url = None
                if registry:
                    url, _, _ = registry.resolve("API", cls)
                if not url:
                    url = f"/v2/scripting-api/{cls.lower()}"

                if desc:
                    yaml_links.append(
                        f'  - {{ label: "{cls}", to: "{url}", desc: "{desc}" }}'
                    )
                else:
                    yaml_links.append(
                        f'  - {{ label: "{cls}", to: "{url}" }}'
                    )

            else:
                # Parse $MODULES.ModuleId$ tokens (with optional annotation)
                mod_ann = re.match(
                    r'\$MODULES\.(\w+)\$\s*--\s*(.+)', item, re.DOTALL
                )
                mod_plain = re.match(r'\$MODULES\.(\w+)\$', item)

                if mod_ann or mod_plain:
                    mod_match = mod_ann or mod_plain
                    mod_id = mod_match.group(1)
                    desc = mod_ann.group(2).strip().replace('"', '\\"') if mod_ann else None

                    # Look up URL and pretty name from registry
                    url = None
                    if registry:
                        url, _, _ = registry.resolve("MODULES", mod_id)
                    label = mod_id
                    if registry and mod_id in registry.module_info:
                        label = registry.module_info[mod_id]["prettyName"]

                    if url:
                        if desc:
                            yaml_links.append(
                                f'  - {{ label: "{label}", to: "{url}", desc: "{desc}" }}'
                            )
                        else:
                            yaml_links.append(
                                f'  - {{ label: "{label}", to: "{url}" }}'
                            )
                    continue

                # Parse $SN.factory.node$ tokens (with optional annotation)
                sn_ann = re.match(
                    r'\$SN\.(\w+)\.(\w+)\$\s*--\s*(.+)', item, re.DOTALL
                )
                sn_plain = re.match(r'\$SN\.(\w+)\.(\w+)\$', item)

                if sn_ann or sn_plain:
                    sn_match = sn_ann or sn_plain
                    sn_factory = sn_match.group(1)
                    sn_node = sn_match.group(2)
                    desc = sn_ann.group(3).strip().replace('"', '\\"') if sn_ann else None
                    sn_key = f"{sn_factory}.{sn_node}"

                    # Look up URL from registry
                    url = None
                    if registry:
                        url, _, _ = registry.resolve("SN", sn_key)
                    if not url:
                        url = f"/v2/reference/scriptnodes/{sn_factory}/{sn_node}"

                    # Same-factory -> short label, cross-factory -> full label
                    if sn_factory == slug:
                        label = sn_node
                    else:
                        label = sn_key

                    if desc:
                        yaml_links.append(
                            f'  - {{ label: "{label}", to: "{url}", desc: "{desc}" }}'
                        )
                    else:
                        yaml_links.append(
                            f'  - {{ label: "{label}", to: "{url}" }}'
                        )
                    continue

                # Parse $UI.Components.Name$ tokens (with optional annotation)
                ui_comp_ann = re.match(
                    r'\$UI\.Components\.(\w+)\$\s*--\s*(.+)', item, re.DOTALL
                )
                ui_comp_plain = re.match(r'\$UI\.Components\.(\w+)\$', item)

                if ui_comp_ann or ui_comp_plain:
                    ui_match = ui_comp_ann or ui_comp_plain
                    comp_name = ui_match.group(1)
                    desc = ui_comp_ann.group(2).strip().replace('"', '\\"') if ui_comp_ann else None

                    # Look up URL from registry
                    url = None
                    if registry:
                        url, _, _ = registry.resolve("UI", f"Components.{comp_name}")
                    if not url:
                        url = f"/v2/reference/ui-components/{comp_name.lower()}"

                    label = comp_name
                    if desc:
                        yaml_links.append(
                            f'  - {{ label: "{label}", to: "{url}", desc: "{desc}" }}'
                        )
                    else:
                        yaml_links.append(
                            f'  - {{ label: "{label}", to: "{url}" }}'
                        )
                    continue

                # Parse $UI.FloatingTiles.Name$ tokens (with optional annotation)
                ui_ft_ann = re.match(
                    r'\$UI\.FloatingTiles\.(\w+)\$\s*--\s*(.+)', item, re.DOTALL
                )
                ui_ft_plain = re.match(r'\$UI\.FloatingTiles\.(\w+)\$', item)

                if ui_ft_ann or ui_ft_plain:
                    ft_match = ui_ft_ann or ui_ft_plain
                    ft_name = ft_match.group(1)
                    desc = ui_ft_ann.group(2).strip().replace('"', '\\"') if ui_ft_ann else None

                    # Look up URL from registry
                    url = None
                    if registry:
                        url, _, _ = registry.resolve("UI", f"FloatingTiles.{ft_name}")
                    if not url:
                        url = f"/v2/reference/ui-components/floating-tiles/{ft_name.lower()}"

                    label = ft_name
                    if desc:
                        yaml_links.append(
                            f'  - {{ label: "{label}", to: "{url}", desc: "{desc}" }}'
                        )
                    else:
                        yaml_links.append(
                            f'  - {{ label: "{label}", to: "{url}" }}'
                        )
                    continue

                # Fallback: plain ClassName.method (legacy format)
                ann_match = re.match(
                    r'([A-Z]\w*\.\w+)`?\s*--\s*(.+)', item, re.DOTALL
                )
                if ann_match:
                    ref = ann_match.group(1).strip()
                    desc = ann_match.group(2).strip().replace('"', '\\"')
                    cls, method = ref.split('.', 1)
                    if cls.lower() == slug:
                        yaml_links.append(
                            f'  - {{ label: "{method}", to: "#{method.lower()}", desc: "{desc}" }}'
                        )
                    else:
                        yaml_links.append(
                            f'  - {{ label: "{ref}", to: "/v2/scripting-api/{cls.lower()}#{method.lower()}", desc: "{desc}" }}'
                        )
                else:
                    ref = item.strip('`$ ')
                    if '.' in ref:
                        cls, method = ref.split('.', 1)
                        if cls.lower() == slug:
                            yaml_links.append(
                                f'  - {{ label: "{method}", to: "#{method.lower()}" }}'
                            )
                        else:
                            yaml_links.append(
                                f'  - {{ label: "{ref}", to: "/v2/scripting-api/{cls.lower()}#{method.lower()}" }}'
                            )
                    else:
                        yaml_links.append(
                            f'  - {{ label: "{ref}", to: "#{ref.lower()}" }}'
                        )

        if not yaml_links:
            return m.group(0)

        return "::see-also\n---\nlinks:\n" + "\n".join(yaml_links) + "\n---\n::"

    return re.sub(pattern, replace_see_also, content, flags=re.MULTILINE)


def convert_common_mistakes(content, messages=None, filepath=""):
    """Convert ## Common Mistakes section to ::common-mistakes component.

    Supports two formats:
    1. New titled format:
       - **Title here**
         **Wrong:** ...
         **Right:** ...
         *reason*

    2. Legacy format (no title):
       - **Wrong:** ...
         **Right:** ...
         *reason*
    """
    TITLE_PLACEHOLDER = "$COMMON_MISTAKE_TITLE_TO_BE_REPLACED$"

    # New format with title: - **Title**\n  **Wrong:** ...\n  **Right:** ...\n  *reason*
    titled_pattern = r'^## Common Mistakes\n\n((?:- \*\*[^W].*?\n(?:  .*?\n)*\n?)+)'

    def replace_titled_cm(m):
        block = m.group(1)
        entries = re.findall(
            r'- \*\*(.+?)\*\*\n  \*\*Wrong:\*\* (.+?)\n  \*\*Right:\*\* (.+?)\n  \*(.+?)\*',
            block
        )
        if not entries:
            return None  # Signal to try legacy format

        yaml_items = []
        for title, wrong, right, reason in entries:
            title = title.rstrip().replace('"', '\\"')
            wrong = wrong.rstrip().replace('\\', '\\\\').replace('"', '\\"')
            right = right.rstrip().replace('\\', '\\\\').replace('"', '\\"')
            reason = reason.rstrip().replace('\\', '\\\\').replace('"', '\\"')

            if title == TITLE_PLACEHOLDER:
                if messages is not None:
                    messages.append({
                        "level": "INFO",
                        "file": filepath,
                        "token": TITLE_PLACEHOLDER,
                        "message": f"Untitled common mistake (needs title): {wrong[:50]}..."
                    })
                yaml_items.append(
                    f'  - wrong: "{wrong}"\n'
                    f'    right: "{right}"\n'
                    f'    reason: "{reason}"'
                )
            else:
                yaml_items.append(
                    f'  - title: "{title}"\n'
                    f'    wrong: "{wrong}"\n'
                    f'    right: "{right}"\n'
                    f'    reason: "{reason}"'
                )

        return (
            "::common-mistakes\n---\nmistakes:\n"
            + "\n".join(yaml_items)
            + "\n---\n::\n"
        )

    # Try titled format first
    titled_match = re.search(titled_pattern, content, flags=re.MULTILINE)
    if titled_match:
        result = replace_titled_cm(titled_match)
        if result is not None:
            return content[:titled_match.start()] + result + content[titled_match.end():]

    # Legacy format: - **Wrong:** ...\n  **Right:** ...\n  *reason*
    legacy_pattern = r'^## Common Mistakes\n\n((?:- \*\*Wrong:\*\*.*?\n(?:  .*?\n)*\n?)+)'

    def replace_legacy_cm(m):
        block = m.group(1)
        entries = re.findall(
            r'- \*\*Wrong:\*\* (.+?)\n  \*\*Right:\*\* (.+?)\n  \*(.+?)\*',
            block
        )
        if not entries:
            return m.group(0)

        yaml_items = []
        for wrong, right, reason in entries:
            wrong = wrong.rstrip().replace('\\', '\\\\').replace('"', '\\"')
            right = right.rstrip().replace('\\', '\\\\').replace('"', '\\"')
            reason = reason.rstrip().replace('\\', '\\\\').replace('"', '\\"')
            yaml_items.append(
                f'  - wrong: "{wrong}"\n'
                f'    right: "{right}"\n'
                f'    reason: "{reason}"'
            )

        return (
            "::common-mistakes\n---\nmistakes:\n"
            + "\n".join(yaml_items)
            + "\n---\n::\n"
        )

    return re.sub(legacy_pattern, replace_legacy_cm, content, flags=re.MULTILINE)


def convert_method_headings(content):
    """Convert ### method + signature + param table to ::method-heading + code block."""
    lines = content.split('\n')
    result = []
    i = 0
    in_methods_section = False

    while i < len(lines):
        if lines[i].strip() == '## Methods':
            in_methods_section = True

        if (lines[i].startswith('### ')
                and not lines[i].startswith('### #')
                and i + 2 < len(lines)):
            raw_heading = lines[i][4:].strip()
            sig_match = re.match(r'`?(?:\w+\s+)?(?:\w+\.)?(\w+)\(', raw_heading)
            method_name = sig_match.group(1) if sig_match else raw_heading.strip('` ')

            j = i + 1
            while j < len(lines) and lines[j].strip() == '':
                j += 1

            has_signature = (j < len(lines)
                             and lines[j].startswith('`')
                             and lines[j].endswith('`')
                             and '```' not in lines[j])

            if has_signature:
                signature = lines[j][1:-1]
                k = j + 1
            else:
                signature = None
                k = j

            while k < len(lines) and lines[k].strip() == '':
                k += 1

            params = []
            table_end = k
            if (k < len(lines)
                    and '| Parameter' in lines[k]
                    and '| Type' in lines[k]
                    and '| Description' in lines[k]):
                k += 1
                if k < len(lines) and lines[k].startswith('|---'):
                    k += 1

                while k < len(lines) and lines[k].startswith('| `'):
                    row = lines[k]
                    cells = [c.strip() for c in row.split('|')[1:-1]]
                    if len(cells) >= 3:
                        pname = cells[0].strip('` ')
                        ptype = cells[1].strip('` ')
                        pdesc = cells[2].strip().replace('"', '\\"')
                        params.append(
                            f'  - {{ name: {pname}, type: {ptype}, desc: "{pdesc}" }}'
                        )
                    k += 1
                table_end = k

            if has_signature or params or in_methods_section:
                if params:
                    result.append("::method-heading")
                    result.append("---")
                    result.append(f"name: {method_name}")
                    result.append("params:")
                    result.extend(params)
                    result.append("---")
                    result.append("::")
                else:
                    result.append(f'::method-heading{{name="{method_name}"}}')
                    result.append("::")

                if signature:
                    result.append("")
                    result.append("```javascript")
                    if not signature.rstrip().endswith(';'):
                        signature = signature.rstrip() + ';'
                    result.append(signature)
                    result.append("```")
                    result.append("")

                i = table_end
                continue
            else:
                result.append(lines[i])
                i += 1
                continue
        else:
            result.append(lines[i])
            i += 1

    return "\n".join(result)


def rewrite_image_paths(content):
    """Rewrite relative image links (SVG, PNG) to absolute Nuxt paths."""
    return re.sub(
        r'!\[([^\]]*)\]\(([^)]*\.(?:svg|png))\)',
        r'![\1](/images/v2/scripting-api/\2)',
        content
    )


def fix_blank_lines_after_code_blocks(content):
    """Ensure a blank line exists after every code block closing fence."""
    return re.sub(r'(\n```)\n([^\n`])', r'\1\n\n\2', content)


# ---------------------------------------------------------------------------
# API category tags
# ---------------------------------------------------------------------------

_CLASS_TAGS_CACHE = None


def _load_class_tags() -> dict:
    """Load class_tags.json (cached after first call)."""
    global _CLASS_TAGS_CACHE
    if _CLASS_TAGS_CACHE is None:
        tags_path = SCRIPT_DIR / "enrichment" / "resources" / "survey" / "class_tags.json"
        if tags_path.is_file():
            with open(tags_path, "r", encoding="utf-8") as f:
                _CLASS_TAGS_CACHE = json.load(f)
        else:
            _CLASS_TAGS_CACHE = {}
    return _CLASS_TAGS_CACHE


def inject_api_category_tags(content: str, class_name: str) -> str:
    """Insert a ::category-tags MDC block after YAML frontmatter for API pages.

    Looks up the class in class_tags.json and generates tags for its domain
    group and role (skipping the role tag when it matches the group name).
    """
    tags_data = _load_class_tags()
    if not tags_data or not class_name:
        return content

    class_entry = tags_data.get("classes", {}).get(class_name)
    if not class_entry:
        return content

    groups = tags_data.get("groups", {})
    roles = tags_data.get("roles", {})

    group_key = class_entry.get("group", "")
    role_key = class_entry.get("role", "")

    tags = []
    if group_key and group_key in groups:
        tags.append(f'  - {{ name: {group_key}, desc: "{groups[group_key]}" }}')
    if role_key and role_key in roles and role_key != group_key:
        tags.append(f'  - {{ name: {role_key}, desc: "{roles[role_key]}" }}')

    if not tags:
        return content

    tag_block = "::category-tags\n---\ntags:\n" + "\n".join(tags) + "\n---\n::\n"

    # Insert after frontmatter closing ---
    fm_match = re.match(r'^(---\n.*?\n---\n)', content, re.DOTALL)
    if fm_match:
        insert_pos = fm_match.end()
        content = content[:insert_pos] + "\n" + tag_block + "\n" + content[insert_pos:]

    return content


# ---------------------------------------------------------------------------
# API class-level see-also
# ---------------------------------------------------------------------------

_CLASS_SURVEY_CACHE = None


def _load_class_survey() -> dict:
    """Load class_survey_data.json (cached after first call)."""
    global _CLASS_SURVEY_CACHE
    if _CLASS_SURVEY_CACHE is None:
        survey_path = SCRIPT_DIR / "enrichment" / "resources" / "survey" / "class_survey_data.json"
        if survey_path.is_file():
            with open(survey_path, "r", encoding="utf-8") as f:
                _CLASS_SURVEY_CACHE = json.load(f)
        else:
            _CLASS_SURVEY_CACHE = {}
    return _CLASS_SURVEY_CACHE


def inject_api_class_see_also(content: str, class_name: str) -> str:
    """Insert a class-level ::see-also MDC block and seeAlso frontmatter field.

    Reads the seeAlso array from class_survey_data.json and:
    1. Adds a seeAlso field to YAML frontmatter (class names + distinctions)
    2. Inserts a ::see-also MDC block after ::category-tags (or after frontmatter)
    3. For component-role classes with UI pages, prepends a cross-link to the UI reference
    """
    survey = _load_class_survey()
    if not survey:
        return content

    class_data = survey.get("classes", {}).get(class_name)
    if not class_data:
        return content

    see_also = list(class_data.get("seeAlso", []))

    # Prepend UI component cross-link for classes with role "component"
    tags_data = _load_class_tags()
    class_tags = tags_data.get("classes", {}).get(class_name, {})
    if class_tags.get("role") == "component":
        # Check if a UI component page exists for this class
        ui_pages_dir = SCRIPT_DIR / "ui_enrichment" / "pages" / "components"
        if ui_pages_dir.is_dir() and any(
            f.stem == class_name for f in ui_pages_dir.glob("*.md")
        ):
            see_also.insert(0, {
                "class": f"$UI.{class_name}$",
                "distinction": "CSS styling, properties reference, and visual customization",
                "_token": True
            })

    if not see_also:
        return content

    # Build ::see-also MDC block
    yaml_links = []
    for entry in see_also:
        cls = entry.get("class", "")
        distinction = entry.get("distinction", "").replace('"', '\\"')
        if not cls:
            continue
        if entry.get("_token"):
            # Token-based link (resolved by resolve_tokens later)
            yaml_links.append(
                f'  - {{ label: "{class_name} (UI Reference)", to: "{cls}", desc: "{distinction}" }}'
            )
        else:
            yaml_links.append(
                f'  - {{ label: "{cls}", to: "$API.{cls}$", desc: "{distinction}" }}'
            )

    if not yaml_links:
        return content

    see_also_block = "::see-also\n---\nlinks:\n" + "\n".join(yaml_links) + "\n---\n::\n"

    # Insert after ::category-tags block (if present), otherwise after frontmatter
    ct_end = content.find("::category-tags")
    if ct_end != -1:
        # Find the closing :: of the category-tags block
        closing = content.find("\n::\n", ct_end)
        if closing != -1:
            insert_pos = closing + 4  # after \n::\n
            content = content[:insert_pos] + "\n" + see_also_block + "\n" + content[insert_pos:]
    else:
        fm_match = re.match(r'^(---\n.*?\n---\n)', content, re.DOTALL)
        if fm_match:
            insert_pos = fm_match.end()
            content = content[:insert_pos] + "\n" + see_also_block + "\n" + content[insert_pos:]

    # Add seeAlso to frontmatter
    fm_match = re.match(r'^(---\n)(.*?)(\n---\n)', content, re.DOTALL)
    if fm_match:
        fm_body = fm_match.group(2)
        fm_lines = []
        fm_lines.append("seeAlso:")
        for entry in see_also:
            cls = entry.get("class", "")
            distinction = entry.get("distinction", "").replace('"', '\\"')
            if cls:
                fm_lines.append(f'  - class: "{cls}"')
                fm_lines.append(f'    distinction: "{distinction}"')
        fm_addition = "\n".join(fm_lines)
        content = fm_match.group(1) + fm_body + "\n" + fm_addition + fm_match.group(3) + content[fm_match.end():]

    return content


def apply_mdc_transforms(content: str, class_name: str,
                         messages: list = None, filepath: str = "") -> str:
    """Apply all MDC transformations to API markdown content."""
    content = convert_h1_to_frontmatter(content)
    content = inject_api_category_tags(content, class_name)
    content = inject_api_class_see_also(content, class_name)
    content = convert_method_headings(content)
    content = convert_common_mistakes(content, messages, filepath)
    content = convert_warning_blockquotes(content, messages, filepath)
    content = convert_tip_blockquotes(content, messages, filepath)
    content = rewrite_image_paths(content)
    content = fix_blank_lines_after_code_blocks(content)
    return content


def apply_module_mdc_transforms(content: str, messages: list = None,
                                filepath: str = "") -> str:
    """Apply MDC transformations to module reference markdown.

    Module pages already contain most MDC components (::signal-path,
    ::parameter-table, ::modulation-table, ::category-tags) authored
    directly. We only need to convert:
    - $FORUM_REF.tid$ tokens -> forum topic URLs
    - **See also:** lines -> ::see-also
    - Warning/tip blockquotes -> ::warning / ::tip
    - Blank line fixes after code blocks
    """
    # Resolve $FORUM_REF.tid$ -> https://forum.hise.audio/topic/tid
    content = re.sub(
        r'\$FORUM_REF\.(\d+)\$',
        r'https://forum.hise.audio/topic/\1',
        content
    )
    content = convert_warning_blockquotes(content, messages, filepath)
    content = convert_tip_blockquotes(content, messages, filepath)
    content = _extract_frontmatter_components(content, messages, filepath)
    content = fix_blank_lines_after_code_blocks(content)
    return content


def _extract_frontmatter_components(content: str, messages: list = None,
                                    filepath: str = "") -> str:
    """Extract commonMistakes and cpuProfile from YAML frontmatter and
    append them as MDC body content.

    commonMistakes -> ::common-mistakes component (with titles)
    cpuProfile -> simple text block
    """
    # Find frontmatter boundaries
    fm_match = re.match(r'^---\n(.*?)\n---\n', content, re.DOTALL)
    if not fm_match:
        return content

    fm_text = fm_match.group(1)
    body = content[fm_match.end():]

    # --- Extract commonMistakes ---
    cm_block = re.search(
        r'^commonMistakes:\n((?:  .+\n)+)', fm_text, re.MULTILINE
    )
    if cm_block:
        # Note: entries without 'title' field are handled by the title-less regex below
        # Validate: every entry must have a title field
        entries_without_title = re.findall(r'  - wrong:', cm_block.group(0))
        if entries_without_title:
            if messages is not None:
                messages.append({
                    "level": "ERROR",
                    "file": filepath,
                    "message": f"commonMistakes: {len(entries_without_title)} entries missing 'title' field. "
                               f"Every entry must have a title for the ::common-mistakes component to render."
                })

        entries = re.findall(
            r'  - title:\s*"(.+?)"\n\s+wrong:\s*"(.+?)"\n\s+right:\s*"(.+?)"\n\s+explanation:\s*"(.+?)"',
            cm_block.group(0)
        )
        if entries:
            yaml_items = []
            for title, wrong, right, explanation in entries:
                wrong = wrong.replace('\\', '\\\\').replace('"', '\\"')
                right = right.replace('\\', '\\\\').replace('"', '\\"')
                explanation = explanation.replace('\\', '\\\\').replace('"', '\\"')
                title = title.replace('"', '\\"')
                yaml_items.append(
                    f'  - title: "{title}"\n'
                    f'    wrong: "{wrong}"\n'
                    f'    right: "{right}"\n'
                    f'    reason: "{explanation}"'
                )
            cm_component = (
                "\n## Common Mistakes\n\n"
                "::common-mistakes\n---\nmistakes:\n"
                + "\n".join(yaml_items)
                + "\n---\n::\n"
            )
            # Insert before last See Also or at end of body
            see_also_pos = body.rfind("::see-also")
            if see_also_pos != -1:
                body = body[:see_also_pos] + cm_component + "\n" + body[see_also_pos:]
            else:
                body = body.rstrip() + "\n" + cm_component

    # --- Extract cpuProfile ---
    cpu_match = re.search(
        r'^cpuProfile:\n\s+baseline:\s*(\S+)\n\s+polyphonic:\s*(\S+)',
        fm_text, re.MULTILINE
    )
    if cpu_match:
        baseline = cpu_match.group(1)
        poly = "polyphonic" if cpu_match.group(2) == "true" else "monophonic"

        # Check for scaling factors
        sf_match = re.search(
            r'scalingFactors:\n((?:\s+- .+\n)+)', fm_text
        )
        scaling_note = ""
        if sf_match:
            factors = re.findall(r'parameter:\s*"?([^",]+)"?', sf_match.group(1))
            if factors:
                scaling_note = f" Scales with {', '.join(factors)}."

        cpu_text = f"\n**CPU:** {baseline}, {poly}.{scaling_note}\n"

        # Insert after first paragraph (after screenshot if present)
        # Find the ## Signal Path or first ## heading
        heading_match = re.search(r'^## ', body, re.MULTILINE)
        if heading_match:
            body = body[:heading_match.start()] + cpu_text + "\n" + body[heading_match.start():]
        else:
            body = cpu_text + "\n" + body

    return content[:fm_match.end()] + body


def apply_scriptnode_mdc_transforms(content: str, messages: list = None,
                                     filepath: str = "") -> str:
    """Apply MDC transformations to scriptnode node reference markdown.

    Scriptnode pages have YAML frontmatter with structured data that needs
    to be extracted into body content:
    - commonMistakes -> ::common-mistakes MDC component
    - cpuProfile -> simple text block
    - Warning/tip blockquotes -> ::warning / ::tip
    - Blank line fixes after code blocks
    """
    content = _extract_frontmatter_components(content, messages, filepath)
    content = convert_warning_blockquotes(content, messages, filepath)
    content = convert_tip_blockquotes(content, messages, filepath)
    content = fix_blank_lines_after_code_blocks(content)
    return content


def inject_ui_api_cross_link(content: str) -> str:
    """Inject a ::see-also block on UI component pages linking to the scripting API.

    Reads componentId from frontmatter and adds a see-also link using $API$ tokens.
    """
    component_id = _extract_frontmatter_field(content, "componentId")
    if not component_id:
        return content

    see_also_block = (
        '::see-also\n---\nlinks:\n'
        f'  - {{ label: "{component_id} (Scripting API)", '
        f'to: "$API.{component_id}$", '
        f'desc: "Methods for creating and controlling this component" }}\n'
        '---\n::\n'
    )

    # Insert after frontmatter
    fm_match = re.match(r'^(---\n.*?\n---\n)', content, re.DOTALL)
    if fm_match:
        insert_pos = fm_match.end()
        content = content[:insert_pos] + "\n" + see_also_block + "\n" + content[insert_pos:]

    return content


def apply_ui_mdc_transforms(content: str, messages: list = None,
                             filepath: str = "") -> str:
    """Apply MDC transformations to UI component reference markdown.

    UI pages have YAML frontmatter with commonMistakes (same format as
    scriptnode) and use warning/tip blockquotes in the body.
    """
    content = _extract_frontmatter_components(content, messages, filepath)
    content = inject_ui_api_cross_link(content)
    content = convert_warning_blockquotes(content, messages, filepath)
    content = convert_tip_blockquotes(content, messages, filepath)
    content = fix_blank_lines_after_code_blocks(content)
    return content


def apply_language_mdc_transforms(content: str, messages: list = None,
                                   filepath: str = "") -> str:
    """Apply MDC transformations to language reference markdown.

    Language pages use warning/tip blockquotes and see-also lines.
    No method headings, common mistakes, or h1-to-frontmatter conversion.
    """
    content = convert_warning_blockquotes(content, messages, filepath)
    content = convert_tip_blockquotes(content, messages, filepath)
    content = fix_blank_lines_after_code_blocks(content)
    return content


# ---------------------------------------------------------------------------
# Image validation
# ---------------------------------------------------------------------------

def validate_images(content: str, filepath: str, output_dir: Path,
                    messages: list):
    """Check that all image references point to existing files."""
    # Match ![alt](path) patterns
    for m in re.finditer(r'!\[[^\]]*\]\(([^)]+)\)', content):
        img_path = m.group(1)
        # Skip external URLs
        if img_path.startswith("http://") or img_path.startswith("https://"):
            continue
        # Resolve relative to output directory or as absolute from content root
        if img_path.startswith("/"):
            # Absolute path from site root -- check in public/ dir
            # The images typically live in public/images/
            check_paths = [
                output_dir / ".." / "public" / img_path.lstrip("/"),
                output_dir / img_path.lstrip("/"),
            ]
        else:
            file_dir = Path(filepath).parent
            check_paths = [file_dir / img_path]

        found = any(p.is_file() for p in check_paths)
        if not found:
            messages.append({
                "level": "WARN",
                "file": filepath,
                "token": img_path,
                "message": f"Image not found: {img_path}"
            })


# ---------------------------------------------------------------------------
# Source collection
# ---------------------------------------------------------------------------

def _slugify(title: str) -> str:
    """Convert a title to a URL slug."""
    slug = title.lower().strip()
    slug = re.sub(r'[^a-z0-9\s-]', '', slug)
    slug = re.sub(r'[\s]+', '-', slug)
    return slug


def _extract_frontmatter_field(content: str, field: str) -> str:
    """Extract a field value from YAML frontmatter."""
    m = re.search(rf'^{field}:\s*(.+)$', content, re.MULTILINE)
    if m:
        return m.group(1).strip().strip('"').strip("'")
    return ""


def _get_module_output_subdir(module_id: str, module_list_path: Path,
                               type_mapping: dict) -> str:
    """Look up a module's output subdirectory from moduleList.json + typeMapping."""
    if not module_list_path.is_file():
        return ""

    with open(module_list_path, "r", encoding="utf-8") as f:
        module_list = json.load(f)

    for module in module_list.get("modules", []):
        if module["id"] == module_id:
            module_type = module.get("type", "")
            module_subtype = module.get("subtype", "")
            type_key = f"{module_type}.{module_subtype}" if module_subtype else module_type
            dir_path = type_mapping.get(type_key) or type_mapping.get(module_type)
            if dir_path:
                return dir_path
            break
    return ""


# Cache for module list lookups
_module_list_cache = None


def _get_module_list(module_list_path: Path) -> list:
    """Load and cache moduleList.json modules array."""
    global _module_list_cache
    if _module_list_cache is not None:
        return _module_list_cache

    if not module_list_path.is_file():
        _module_list_cache = []
        return _module_list_cache

    with open(module_list_path, "r", encoding="utf-8") as f:
        data = json.load(f)
    _module_list_cache = data.get("modules", [])
    return _module_list_cache


def _resolve_module_subdir(module_id: str, module_list_path: Path,
                            type_mapping: dict) -> str:
    """Look up a module's output subdirectory using cached module list."""
    modules = _get_module_list(module_list_path)
    for module in modules:
        if module["id"] == module_id:
            module_type = module.get("type", "")
            module_subtype = module.get("subtype", "")
            type_key = f"{module_type}.{module_subtype}" if module_subtype else module_type
            return type_mapping.get(type_key) or type_mapping.get(module_type, "")
    return ""


def collect_sources(script_dir: Path, site_structure: dict) -> list:
    """Collect all markdown files from pipeline sources defined in site_structure.json.

    Returns list of (source_path, domain, relative_output_path) tuples.
    """
    sources = []

    for domain_name, domain_config in site_structure.get("domains", {}).items():
        domain_sources = domain_config.get("sources", {})
        content_dir = domain_config.get("contentDir", "")
        type_mapping = domain_config.get("typeMapping", {})

        # Pages: individual content files (API classes, module pages, SN nodes)
        pages_path = domain_sources.get("pages")

        # UI domain uses a dict of subdirectory -> source path mappings
        if isinstance(pages_path, dict):
            for subdir_name, subdir_path in pages_path.items():
                subdir_dir = script_dir / subdir_path
                if not subdir_dir.is_dir():
                    continue
                for md_file in sorted(subdir_dir.glob("*.md")):
                    if md_file.name.lower() == "readme.md":
                        continue
                    out_rel = Path(content_dir) / subdir_name / md_file.name.lower()
                    sources.append((md_file, domain_name, out_rel, "pages"))
            pages_path = None  # Skip the standard pages processing below

        if pages_path:
            pages_dir = script_dir / pages_path
            if pages_dir.is_dir():
                # SN uses nested {factory}/{node}.md structure
                if domain_name == "SN":
                    glob_pattern = "**/*.md"
                else:
                    glob_pattern = "*.md"

                for md_file in sorted(pages_dir.glob(glob_pattern)):
                    # Skip non-content files
                    if "_review" in md_file.name or "_llm" in md_file.name:
                        continue

                    if domain_name == "API":
                        # API: lowercase filename
                        out_rel = Path(content_dir) / md_file.name.lower()
                    elif domain_name == "SN":
                        # SN: preserve factory/node structure
                        rel = md_file.relative_to(pages_dir)
                        if rel.name.lower() == "readme.md":
                            # Factory Readme -> factory/index.md
                            out_rel = Path(content_dir) / rel.parent / "index.md"
                        else:
                            out_rel = Path(content_dir) / rel
                        source_type = "static" if rel.name.lower() == "readme.md" else "pages"
                        sources.append((md_file, domain_name, out_rel, source_type))
                        continue
                    elif type_mapping:
                        # Modules: use typeMapping to determine subdirectory
                        with open(md_file, "r", encoding="utf-8") as mf:
                            module_id = _extract_frontmatter_field(
                                mf.read(), "moduleId"
                            ) or md_file.stem
                        registry_source = domain_config.get("registrySource", "")
                        module_list_path = script_dir / registry_source
                        subdir = _resolve_module_subdir(
                            module_id, module_list_path, type_mapping
                        )
                        if subdir:
                            out_rel = Path(content_dir) / subdir / md_file.name
                        else:
                            # Unknown module type, place in root
                            out_rel = Path(content_dir) / md_file.name
                    else:
                        # Default: preserve filename
                        out_rel = Path(content_dir) / md_file.name.lower()

                    sources.append((md_file, domain_name, out_rel, "pages"))

        # Static: index/overview pages placed by title-based slug
        static_path = domain_sources.get("static")
        if static_path:
            static_dir = script_dir / static_path
            if static_dir.is_dir():
                for md_file in sorted(static_dir.rglob("*.md")):
                    with open(md_file, "r", encoding="utf-8") as f:
                        file_content = f.read()

                    # Resolve placement from staticPages in site_structure
                    static_pages_map = domain_config.get("staticPages", {})
                    title = _extract_frontmatter_field(file_content, "title")
                    if title and title in static_pages_map:
                        subdir = static_pages_map[title]
                        if subdir:
                            out_rel = Path(content_dir) / subdir / "index.md"
                        else:
                            out_rel = Path(content_dir) / "index.md"
                    else:
                        # Fallback: slug from title or filename
                        if title:
                            slug = _slugify(title)
                            out_rel = Path(content_dir) / slug / "index.md"
                        else:
                            stem = md_file.stem
                            if stem == "index":
                                out_rel = Path(content_dir) / "index.md"
                            else:
                                out_rel = Path(content_dir) / stem / "index.md"

                    sources.append((md_file, domain_name, out_rel, "static"))

    return sources


# ---------------------------------------------------------------------------
# Main pipeline
# ---------------------------------------------------------------------------

def collect_topology(content: str, registry: LinkRegistry, page_key: str,
                     source_path: str) -> dict:
    """Scan a file for all $DOMAIN$ tokens and classify as see-also or inline.

    Returns a dict with seeAlso and inlineLinks lists of canonical refs.
    """
    see_also = []
    inline_links = []

    # Check if we're in a see-also line context
    for line in content.split('\n'):
        is_see_also_line = (
            line.startswith("**See also:**") or
            line.startswith("**Cross References:**")
        )

        for m in LINK_TOKEN_PATTERN.finditer(line):
            domain = m.group(1)
            target_str = m.group(2)
            fragment = m.group(3)

            parts = target_str.split(".")
            url, canonical, match_type = registry.resolve_compound(
                domain, parts, fragment
            )

            if canonical:
                ref = f"{domain}.{canonical}"
            else:
                ref = f"{domain}.{target_str}"

            if is_see_also_line:
                if ref not in see_also:
                    see_also.append(ref)
            else:
                if ref not in inline_links:
                    inline_links.append(ref)

    # Also check markdown links [text]($DOMAIN.Target$) on non-see-also lines
    for line in content.split('\n'):
        is_see_also_line = (
            line.startswith("**See also:**") or
            line.startswith("**Cross References:**")
        )
        if is_see_also_line:
            continue

        for m in re.finditer(r'\[[^\]]+\]\(\$([A-Z]+)\.([^$#]+?)(?:#[^$]+)?\$\)', line):
            domain = m.group(1)
            target_str = m.group(2)
            parts = target_str.split(".")
            url, canonical, match_type = registry.resolve_compound(domain, parts)
            ref = f"{domain}.{canonical}" if canonical else f"{domain}.{target_str}"
            if ref not in inline_links and ref not in see_also:
                inline_links.append(ref)

    return {
        "source": source_path,
        "seeAlso": see_also,
        "inlineLinks": inline_links,
    }


def run_topology(topology_path: Path):
    """Scan all sources and emit a cross-reference topology JSON."""

    if not SITE_STRUCTURE_PATH.is_file():
        print(f"ERROR: {SITE_STRUCTURE_PATH} not found.")
        sys.exit(1)

    with open(SITE_STRUCTURE_PATH, "r", encoding="utf-8") as f:
        site_structure = json.load(f)

    print("Building link registry...")
    registry = LinkRegistry(site_structure, SCRIPT_DIR)
    for domain, targets in registry.targets.items():
        print(f"  {domain}: {len(targets)} targets")

    sources = collect_sources(SCRIPT_DIR, site_structure)
    if not sources:
        print("No source files found.")
        return

    print(f"\nScanning {len(sources)} files for cross-references...")

    pages = {}
    all_refs = {}  # page_key -> set of outgoing refs
    static_pages = set()  # pages from static sources (index/overview pages)

    for source_path, domain, out_rel, source_type in sources:
        with open(source_path, "r", encoding="utf-8") as f:
            content = f.read()

        # Derive page key from domain + identifier
        lines = content.split('\n')
        if domain == "API":
            name = get_class_name_from_heading(lines)
            page_key = f"API.{name}" if name else f"API.{out_rel.stem}"
        elif domain == "MODULES":
            if source_type == "static":
                # Static pages: use title for key
                title = _extract_frontmatter_field(content, "title")
                page_key = f"MODULES._static.{_slugify(title)}" if title else f"MODULES._static.{out_rel.stem}"
            else:
                # Module pages: use moduleId from frontmatter or filename
                module_id = _extract_frontmatter_field(content, "moduleId")
                if not module_id:
                    module_id = out_rel.stem
                page_key = f"MODULES.{module_id}"
        else:
            page_key = f"{domain}.{out_rel.stem}"

        rel_source = str(source_path.relative_to(SCRIPT_DIR)).replace("\\", "/")
        topo = collect_topology(content, registry, page_key, rel_source)
        topo["sourceType"] = source_type
        pages[page_key] = topo
        all_refs[page_key] = set(topo["seeAlso"] + topo["inlineLinks"])
        if source_type == "static":
            static_pages.add(page_key)

    # Detect bidirectional gaps (exclude static pages - they link to all
    # modules in their category as a directory listing, modules don't
    # need to link back to the index)
    gaps = []
    for page_key, outgoing in all_refs.items():
        if page_key in static_pages:
            continue  # Skip static -> content gaps
        for ref in outgoing:
            if ref in static_pages:
                continue  # Skip content -> static gaps
            # Check if the target page links back
            if ref in all_refs and page_key not in all_refs[ref]:
                gaps.append({
                    "from": page_key,
                    "to": ref,
                    "reverseExists": False,
                })

    # Statistics
    total_see_also = sum(len(p["seeAlso"]) for p in pages.values())
    total_inline = sum(len(p["inlineLinks"]) for p in pages.values())
    orphan_pages = [k for k, v in all_refs.items() if not v]

    topology = {
        "generated": __import__("datetime").datetime.now(
            __import__("datetime").timezone.utc
        ).isoformat(),
        "stats": {
            "totalPages": len(pages),
            "totalSeeAlso": total_see_also,
            "totalInlineLinks": total_inline,
            "bidirectionalGaps": len(gaps),
            "orphanPages": len(orphan_pages),
        },
        "pages": pages,
        "bidirectionalGaps": gaps,
        "orphanPages": orphan_pages,
    }

    with open(topology_path, "w", encoding="utf-8") as f:
        json.dump(topology, f, indent=2, ensure_ascii=False)

    print(f"\n{'=' * 60}")
    print(f"Topology written to {topology_path}")
    print(f"  Pages: {len(pages)}")
    print(f"  See-also links: {total_see_also}")
    print(f"  Inline links: {total_inline}")
    print(f"  Bidirectional gaps: {len(gaps)}")
    print(f"  Orphan pages (no outgoing links): {len(orphan_pages)}")


def run(output_dir: Path, strict: bool = False, dry_run: bool = False):
    """Main publish pipeline."""

    # Load site structure
    if not SITE_STRUCTURE_PATH.is_file():
        print(f"ERROR: {SITE_STRUCTURE_PATH} not found.")
        sys.exit(1)

    with open(SITE_STRUCTURE_PATH, "r", encoding="utf-8") as f:
        site_structure = json.load(f)

    # Build link registry
    print("Building link registry...")
    registry = LinkRegistry(site_structure, SCRIPT_DIR)
    for domain, targets in registry.targets.items():
        print(f"  {domain}: {len(targets)} targets")

    # Collect source files
    sources = collect_sources(SCRIPT_DIR, site_structure)
    if not sources:
        print("No source files found.")
        return

    print(f"\nProcessing {len(sources)} files...")

    messages = []
    files_written = 0

    for source_path, domain, out_rel, source_type in sources:
        with open(source_path, "r", encoding="utf-8") as f:
            content = f.read()

        # Extract class name before transforms modify the heading
        lines = content.split('\n')
        class_name = get_class_name_from_heading(lines)

        # Step 1: Convert see-also BEFORE token resolution (uses $API$/$MODULES$ tokens)
        mdc_transform = site_structure.get("domains", {}).get(domain, {}).get("mdcTransform", "")
        if mdc_transform == "api":
            content = convert_see_also(content, class_name, registry)
        elif mdc_transform == "module":
            module_id = ""
            id_match = re.search(r'^moduleId:\s*(\S+)', content, re.MULTILINE)
            if id_match:
                module_id = id_match.group(1)
            content = convert_see_also(content, module_id, registry)
        elif mdc_transform == "scriptnode":
            factory = _extract_frontmatter_field(content, "factory") or ""
            content = convert_see_also(content, factory, registry)
        elif mdc_transform == "ui":
            component_id = _extract_frontmatter_field(content, "componentId") or \
                           _extract_frontmatter_field(content, "contentType") or ""
            content = convert_see_also(content, component_id, registry)
        elif mdc_transform == "language":
            lang_title = _extract_frontmatter_field(content, "title") or ""
            content = convert_see_also(content, lang_title, registry)

        # Step 2: Apply MDC transforms (may inject $DOMAIN$ tokens)
        if mdc_transform == "api":
            content = apply_mdc_transforms(content, class_name, messages,
                                           str(source_path))
        elif mdc_transform == "module":
            content = apply_module_mdc_transforms(content, messages,
                                                    str(source_path))
        elif mdc_transform == "scriptnode":
            content = apply_scriptnode_mdc_transforms(content, messages,
                                                       str(source_path))
        elif mdc_transform == "ui":
            content = apply_ui_mdc_transforms(content, messages,
                                               str(source_path))
        elif mdc_transform == "language":
            content = apply_language_mdc_transforms(content, messages,
                                                     str(source_path))

        # Step 3: Resolve $DOMAIN.Target$ tokens (after MDC transforms)
        content = resolve_tokens(content, registry, str(source_path), messages)

        # Step 3: Validate images
        dest = output_dir / out_rel
        validate_images(content, str(source_path), output_dir, messages)

        # Step 4: Write output
        if not dry_run:
            dest.parent.mkdir(parents=True, exist_ok=True)
            with open(dest, "w", encoding="utf-8") as f:
                f.write(content)
            files_written += 1

    # Print summary
    print(f"\n{'=' * 60}")
    if dry_run:
        print(f"DRY RUN: {len(sources)} files processed, no files written")
    else:
        print(f"Published {files_written} files to {output_dir}")

    # Group messages by level
    errors = [m for m in messages if m["level"] == "ERROR"]
    warns = [m for m in messages if m["level"] == "WARN"]
    infos = [m for m in messages if m["level"] == "INFO"]

    if infos:
        print(f"\nINFO ({len(infos)}):")
        for m in infos:
            print(f"  {m['message']}")
            print(f"    in {m['file']}")

    if warns:
        print(f"\nWARN ({len(warns)}):")
        for m in warns:
            print(f"  {m['message']}")
            print(f"    in {m['file']}")

    if errors:
        print(f"\nERROR ({len(errors)}):")
        for m in errors:
            print(f"  {m['message']}")
            print(f"    in {m['file']}")

    print(f"\nSummary: {len(errors)} errors, {len(warns)} warnings, {len(infos)} info")

    if strict and errors:
        print("\n--strict mode: failing due to unresolved links")
        sys.exit(1)


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description="Publish HISE documentation with link resolution and MDC transforms",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument(
        "output_dir", nargs="?", default=".",
        help="Output directory for published files (default: current directory)",
    )
    parser.add_argument(
        "--strict", action="store_true",
        help="Exit with error code on any unresolved links (for CI)",
    )
    parser.add_argument(
        "--dry-run", action="store_true",
        help="Resolve and validate only, do not write files",
    )
    parser.add_argument(
        "--topology", metavar="FILE",
        help="Emit cross-reference topology JSON and exit (no publish)",
    )

    args = parser.parse_args()

    if args.topology:
        run_topology(Path(args.topology).resolve())
    else:
        output_dir = Path(args.output_dir).resolve()
        run(output_dir, strict=args.strict, dry_run=args.dry_run)


if __name__ == "__main__":
    main()
