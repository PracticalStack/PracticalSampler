#!/usr/bin/env python3
"""
HISE Forum Search Tool

Pre-filters and cleans HISE forum search results for LLM consumption.
Reduces token usage by ~90% compared to raw WebFetch of the NodeBB API.

Usage:
    # Search - returns filtered topic list with signal scores
    python forum-search.py search "ScriptSlider" --also "knob filmstrip"

    # Fetch - returns cleaned post content for selected topics
    python forum-search.py fetch 3630 5460

    # Update - search + diff + fetch + combine in one pass (primary command)
    python forum-search.py update "LFO" --also "lfo modulation" --scope modules:LFO

    # Extract code - extract code fences from combined topic file (upvote > 0)
    python forum-search.py extract-code --scope api:ScriptPanel

    # Refresh trusted posters from forum reputation API
    python forum-search.py refresh-users --min-reputation 100
"""

import argparse
import html
import io
import json
import os
import re
import sys
import time
import urllib.error
import urllib.parse
import urllib.request
from datetime import datetime, timezone
from pathlib import Path

# Force UTF-8 output on Windows (avoids cp1252 encoding errors)
if sys.stdout.encoding != "utf-8":
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding="utf-8", errors="replace")
if sys.stderr.encoding != "utf-8":
    sys.stderr = io.TextIOWrapper(sys.stderr.buffer, encoding="utf-8", errors="replace")

SCRIPT_DIR = Path(__file__).parent
CONFIG_PATH = SCRIPT_DIR / "forum_search" / "config.json"
SCANNED_PATH = SCRIPT_DIR / "forum_search" / "forum_scanned.json"
CACHE_DIR = SCRIPT_DIR / "forum_cache"

FORUM_BASE = "https://forum.hise.audio"
API_BASE = f"{FORUM_BASE}/api"

# ---------------------------------------------------------------------------
# Config
# ---------------------------------------------------------------------------

def load_config():
    if CONFIG_PATH.exists():
        with open(CONFIG_PATH, "r", encoding="utf-8") as f:
            return json.load(f)
    return default_config()


def default_config():
    return {
        "trusted_posters": {},
        "excluded_categories": [4, 20],
        "category_weights": {
            "7": 0.15, "2": 0.10, "3": 0.10, "15": 0.10,
            "5": 0.05, "17": 0.05, "8": 0.05
        },
        "scoring": {
            "trusted_poster_reply": 0.30,
            "high_reply_count_threshold": 10,
            "high_reply_count_weight": 0.20,
            "solved_weight": 0.10,
            "recency_weight": 0.10,
            "recency_months": 12,
            "upvote_weight": 0.05
        },
        "defaults": {
            "max_age_years": 10,
            "max_search_pages": 3,
            "max_posts_per_topic": 30,
            "max_post_words": 500
        }
    }


def save_config(config):
    CONFIG_PATH.parent.mkdir(parents=True, exist_ok=True)
    with open(CONFIG_PATH, "w", encoding="utf-8") as f:
        json.dump(config, f, indent=2, ensure_ascii=False)


# ---------------------------------------------------------------------------
# Scanned tracker
# ---------------------------------------------------------------------------

def load_scanned():
    if SCANNED_PATH.exists():
        with open(SCANNED_PATH, "r", encoding="utf-8") as f:
            return json.load(f)
    return {}


def save_scanned(scanned):
    SCANNED_PATH.parent.mkdir(parents=True, exist_ok=True)
    with open(SCANNED_PATH, "w", encoding="utf-8") as f:
        json.dump(scanned, f, indent=2, ensure_ascii=False)


# ---------------------------------------------------------------------------
# HTTP helpers
# ---------------------------------------------------------------------------

def api_get(endpoint, params=None, retries=2, delay=1.0):
    """GET from the NodeBB API with retry and rate-limit awareness."""
    url = f"{API_BASE}/{endpoint}"
    if params:
        url += "?" + urllib.parse.urlencode(params)

    for attempt in range(retries + 1):
        try:
            req = urllib.request.Request(url, headers={
                "User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36",
                "Accept": "application/json"
            })
            with urllib.request.urlopen(req, timeout=15) as resp:
                return json.loads(resp.read().decode("utf-8"))
        except urllib.error.HTTPError as e:
            if e.code == 429 and attempt < retries:
                time.sleep(delay * (attempt + 1))
                continue
            raise
        except urllib.error.URLError as e:
            if attempt < retries:
                time.sleep(delay)
                continue
            raise


# ---------------------------------------------------------------------------
# Cache helpers
# ---------------------------------------------------------------------------

def ensure_cache_dir():
    CACHE_DIR.mkdir(parents=True, exist_ok=True)


def cache_topic(tid, data):
    """Cache a processed topic to disk."""
    ensure_cache_dir()
    path = CACHE_DIR / f"topic_{tid}.json"
    with open(path, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2, ensure_ascii=False)


def load_cached_topic(tid):
    """Load a previously cached topic."""
    path = CACHE_DIR / f"topic_{tid}.json"
    if path.exists():
        with open(path, "r", encoding="utf-8") as f:
            return json.load(f)
    return None


# ---------------------------------------------------------------------------
# Content cleaning
# ---------------------------------------------------------------------------

def clean_post_content(html_content, max_words=500):
    """Convert HTML post content to clean, compact text."""
    text = html_content

    # Remove HiseSnippet blocks (base64 encoded, typically in <pre><code>)
    text = re.sub(
        r'<pre><code>\s*HiseSnippet\s+[A-Za-z0-9+/=.\s]{50,}\s*</code></pre>',
        '[HiseSnippet omitted]',
        text,
        flags=re.DOTALL
    )

    # Also catch HiseSnippets outside code blocks
    text = re.sub(
        r'HiseSnippet\s+[A-Za-z0-9+/=]{100,}',
        '[HiseSnippet omitted]',
        text
    )

    # Remove blockquote duplicates (quoted replies) - keep a short summary
    def shorten_quote(m):
        inner = m.group(1).strip()
        plain = strip_html(inner)
        first_line = plain.split('\n')[0][:100]
        if first_line:
            return f'[Quoting: {first_line}...]'
        return '[Quote omitted]'

    text = re.sub(
        r'<blockquote>\s*(.*?)\s*</blockquote>',
        shorten_quote,
        text,
        flags=re.DOTALL
    )

    # Convert code blocks
    text = re.sub(r'<pre><code>(.*?)</code></pre>', r'```\n\1\n```', text, flags=re.DOTALL)
    text = re.sub(r'<code>(.*?)</code>', r'`\1`', text, flags=re.DOTALL)

    # Strip emoji images entirely
    text = re.sub(r'<img[^>]*class="[^"]*emoji[^"]*"[^>]*/?\s*>', '', text)

    # Convert regular images to alt-text placeholders
    def img_to_alt(m):
        alt = m.group(1) or "image"
        return f'[image: {alt}]'
    text = re.sub(r'<img[^>]*alt="([^"]*)"[^>]*/?\s*>', img_to_alt, text)
    text = re.sub(r'<img[^>]*/?\s*>', '[image]', text)

    # Convert links
    text = re.sub(r'<a[^>]*href="([^"]*)"[^>]*>(.*?)</a>', r'\2 (\1)', text, flags=re.DOTALL)

    # Convert basic HTML structure
    text = re.sub(r'<br\s*/?\s*>', '\n', text)
    text = re.sub(r'<p[^>]*>', '\n', text)
    text = re.sub(r'</p>', '', text)
    text = re.sub(r'<li[^>]*>', '- ', text)
    text = re.sub(r'</li>', '\n', text)
    text = re.sub(r'<h[1-6][^>]*>(.*?)</h[1-6]>', r'\n## \1\n', text, flags=re.DOTALL)
    text = re.sub(r'<strong>(.*?)</strong>', r'**\1**', text, flags=re.DOTALL)
    text = re.sub(r'<em>(.*?)</em>', r'*\1*', text, flags=re.DOTALL)

    # Strip all remaining HTML tags
    text = re.sub(r'<[^>]+>', '', text)

    # Decode HTML entities
    text = html.unescape(text)

    # Strip URLs (keep link text from markdown-style links, drop the URL)
    text = re.sub(r'\[([^\]]*)\]\(https?://[^\)]+\)', r'\1', text)  # [text](url) -> text
    text = re.sub(r'\(https?://[^\)]+\)', '', text)  # (url) -> empty
    text = re.sub(r'https?://\S+', '', text)  # bare urls -> empty

    # Strip @mentions
    text = re.sub(r'@[\w-]+', '', text)

    # Strip NodeBB quote autotext: "said in TITLE (/post/NNN):\n[Quoting: ...]"
    text = re.sub(r'said in .+? \(/post/\d+\):\s*\n\[Quoting:[^\]]*\.\.\.\]\s*', '', text, flags=re.DOTALL)

    # Collapse whitespace
    text = re.sub(r'\n{3,}', '\n\n', text)
    text = re.sub(r' {2,}', ' ', text)
    text = text.strip()

    # Truncate to max words
    words = text.split()
    if len(words) > max_words:
        text = ' '.join(words[:max_words]) + ' [... truncated]'

    return text


def strip_html(text):
    """Quick HTML tag stripper for summaries."""
    text = re.sub(r'<[^>]+>', '', text)
    return html.unescape(text).strip()


def extract_brief(html_content, max_words=50):
    """Extract a short plain-text brief from HTML post content."""
    text = html_content
    text = re.sub(r'<pre><code>\s*HiseSnippet\s+[A-Za-z0-9+/=.\s]{50,}\s*</code></pre>', '', text, flags=re.DOTALL)
    text = re.sub(r'HiseSnippet\s+[A-Za-z0-9+/=]{100,}', '', text)
    text = re.sub(r'<blockquote>.*?</blockquote>', '', text, flags=re.DOTALL)
    text = re.sub(r'<pre><code>.*?</code></pre>', '[code]', text, flags=re.DOTALL)
    text = re.sub(r'<img[^>]*class="[^"]*emoji[^"]*"[^>]*/?\s*>', '', text)
    text = re.sub(r'<img[^>]*/?\s*>', '', text)
    text = re.sub(r'<[^>]+>', ' ', text)
    text = html.unescape(text)
    text = re.sub(r'\s+', ' ', text).strip()
    words = text.split()
    if len(words) > max_words:
        text = ' '.join(words[:max_words]) + '...'
    return text


# ---------------------------------------------------------------------------
# Signal scoring
# ---------------------------------------------------------------------------

def score_topic(topic_meta, config, search_posts=None):
    """Compute a 0-1 signal score for a topic based on config weights."""
    scoring = config["scoring"]
    trusted = config.get("trusted_posters", {})
    cat_weights = config.get("category_weights", {})
    score = 0.0

    cid = str(topic_meta.get("cid", ""))
    score += cat_weights.get(cid, 0.0)

    postcount = topic_meta.get("postcount", 1)
    if postcount >= scoring["high_reply_count_threshold"]:
        score += scoring["high_reply_count_weight"]

    if topic_meta.get("isSolved"):
        score += scoring["solved_weight"]

    ts = topic_meta.get("timestamp", 0)
    if ts:
        age_months = (time.time() * 1000 - ts) / (1000 * 60 * 60 * 24 * 30)
        if age_months <= scoring["recency_months"]:
            score += scoring["recency_weight"]

    if search_posts:
        for post in search_posts:
            uid = str(post.get("uid", ""))
            if uid in trusted:
                score += scoring["trusted_poster_reply"]
                break

    if search_posts:
        max_upvotes = max((p.get("upvotes", 0) for p in search_posts), default=0)
        if max_upvotes > 0:
            score += scoring["upvote_weight"] * min(max_upvotes / 5, 1.0)

    return min(score, 1.0)


# ---------------------------------------------------------------------------
# Topic fetching (shared by fetch and update)
# ---------------------------------------------------------------------------

def fetch_and_clean_topic(tid, config):
    """Fetch a topic from the API, clean it, cache it, and return the result."""
    defaults = config["defaults"]
    max_posts = defaults["max_posts_per_topic"]
    max_words = defaults.get("max_post_words", 500)
    trusted = config.get("trusted_posters", {})

    topic_data = fetch_topic_posts(tid, max_posts, trusted)

    cleaned_posts = []
    skipped_posts = 0
    for post in topic_data["posts"]:
        uid = str(post.get("uid", ""))
        username = post.get("user", {}).get("username", "Unknown")
        ts_iso = post.get("timestampISO", "")[:10]
        upvotes = post.get("upvotes", 0)
        is_trusted = uid in trusted
        is_op = bool(post.get("isMainPost"))
        # For signal filtering, only author/expert roles auto-keep;
        # other trusted posters still need upvotes to be included
        trusted_role = trusted.get(uid, {}).get("role", "") if is_trusted else ""
        is_authority = trusted_role in ("author", "expert")

        # Signal filter: OP, author/expert replies, and upvoted posts
        if not (is_op or is_authority or upvotes > 0):
            skipped_posts += 1
            continue

        content = clean_post_content(post.get("content", ""), max_words)
        if not content.strip():
            continue

        meta_parts = [username]
        if is_trusted:
            role = trusted[uid].get("role", "trusted")
            meta_parts[0] = f"{username} [{role}]"
        if ts_iso:
            meta_parts.append(ts_iso)
        if upvotes > 0:
            meta_parts.append(f"+{upvotes}")
        if is_op:
            meta_parts.append("OP")

        header = " | ".join(meta_parts)

        cleaned_posts.append({
            "uid": uid,
            "username": username,
            "is_trusted": is_trusted,
            "is_op": is_op,
            "upvotes": upvotes,
            "header": header,
            "content": content
        })

    # Sort: OP first, then trusted posters, then by upvotes
    def post_sort_key(p):
        return (
            0 if p["is_op"] else 1,
            0 if p["is_trusted"] else 1,
            -p["upvotes"]
        )
    cleaned_posts.sort(key=post_sort_key)

    # Slim down posts for token efficiency
    slim_posts = []
    for p in cleaned_posts:
        sp = {"header": p["header"], "content": p["content"]}
        if p["upvotes"] > 0:
            sp["upvotes"] = p["upvotes"]
        slim_posts.append(sp)

    topic_title = topic_data.get("title", f"Topic {tid}")
    topic_output = {
        "tid": tid,
        "title": topic_title,
        "posts": slim_posts
    }

    cache_topic(tid, topic_output)
    return topic_output


def fetch_topic_posts(tid, max_posts, trusted):
    """Fetch topic posts with smart pagination for large topics."""
    data = api_get(f"topic/{tid}")
    title = strip_html(data.get("titleRaw", data.get("title", f"Topic {tid}")))
    postcount = data.get("postcount", 0)
    all_posts = data.get("posts", [])

    page_count = data.get("pagination", {}).get("pageCount", 1)

    if page_count > 1:
        pages_to_fetch = set()

        if postcount <= max_posts:
            pages_to_fetch = set(range(2, page_count + 1))
        else:
            pages_to_fetch.add(2)
            if page_count > 2:
                pages_to_fetch.add(page_count)
            if page_count > 5:
                pages_to_fetch.add(page_count // 2)

        for page_num in sorted(pages_to_fetch):
            time.sleep(0.3)
            try:
                page_data = api_get(f"topic/{tid}", {"page": page_num})
                page_posts = page_data.get("posts", [])
                all_posts.extend(page_posts)
            except Exception as e:
                print(f"[warn] Failed to fetch page {page_num} of topic {tid}: {e}",
                      file=sys.stderr)

    # Deduplicate by pid
    seen_pids = set()
    unique_posts = []
    for post in all_posts:
        pid = post.get("pid")
        if pid and pid not in seen_pids:
            seen_pids.add(pid)
            unique_posts.append(post)

    # Limit total posts, but always keep trusted poster posts and OP
    if len(unique_posts) > max_posts:
        trusted_uids = set(trusted.keys())
        priority_posts = [p for p in unique_posts
                         if str(p.get("uid", "")) in trusted_uids
                         or p.get("isMainPost")]
        other_posts = [p for p in unique_posts
                      if str(p.get("uid", "")) not in trusted_uids
                      and not p.get("isMainPost")]
        other_posts.sort(key=lambda p: p.get("upvotes", 0), reverse=True)
        remaining_slots = max_posts - len(priority_posts)
        unique_posts = priority_posts + other_posts[:max(0, remaining_slots)]

    return {
        "title": title,
        "postcount": postcount,
        "posts": unique_posts
    }


# ---------------------------------------------------------------------------
# SEARCH command
# ---------------------------------------------------------------------------

def run_search(term, also_terms, config, include_features=False, max_pages=None, max_age=None):
    """Run a search and return the scored topic list. Used by both search and update."""
    terms = [term] + (also_terms or [])
    defaults = config["defaults"]
    max_pages = max_pages or defaults["max_search_pages"]
    max_age_years = max_age or defaults["max_age_years"]
    excluded_cats = set(config.get("excluded_categories", []))
    if include_features:
        excluded_cats.discard(4)

    cutoff_ts = (time.time() - max_age_years * 365.25 * 24 * 3600) * 1000

    topics = {}
    total_raw = 0
    total_filtered = 0
    filter_reasons = {"no_replies": 0, "excluded_category": 0, "too_old": 0, "duplicate": 0}

    for search_term in terms:
        for page in range(1, max_pages + 1):
            try:
                data = api_get("search", {
                    "term": search_term,
                    "in": "titlesposts",
                    "sortBy": "relevance",
                    "page": page
                })
            except Exception as e:
                print(f"[warn] Search failed for '{search_term}' page {page}: {e}", file=sys.stderr)
                break

            posts = data.get("posts", [])
            if not posts:
                break

            for post in posts:
                total_raw += 1
                topic = post.get("topic", {})
                tid = topic.get("tid")
                if not tid:
                    continue

                if tid in topics:
                    topics[tid]["posts"].append(post)
                    continue

                cat = post.get("category", {})
                cid = cat.get("cid", 0)
                if cid in excluded_cats:
                    total_filtered += 1
                    filter_reasons["excluded_category"] += 1
                    continue

                postcount = topic.get("postcount", 1)
                if postcount <= 1:
                    total_filtered += 1
                    filter_reasons["no_replies"] += 1
                    continue

                ts = topic.get("timestamp", 0)
                if ts and ts < cutoff_ts:
                    total_filtered += 1
                    filter_reasons["too_old"] += 1
                    continue

                brief = extract_brief(post.get("content", ""), max_words=50)

                topics[tid] = {
                    "tid": tid,
                    "title": strip_html(topic.get("titleRaw", topic.get("title", ""))),
                    "postcount": postcount,
                    "category": cat.get("name", "Unknown"),
                    "cid": cid,
                    "timestamp": ts,
                    "age_months": int((time.time() * 1000 - ts) / (1000 * 60 * 60 * 24 * 30)) if ts else 0,
                    "isSolved": bool(topic.get("isSolved")),
                    "brief": brief,
                    "posts": [post]
                }

            time.sleep(0.3)

    # Score topics
    trusted = config.get("trusted_posters", {})
    results = []
    for tid, t in topics.items():
        trusted_names = []
        for post in t["posts"]:
            uid = str(post.get("uid", ""))
            if uid in trusted:
                name = trusted[uid].get("name", post.get("user", {}).get("username", ""))
                if name not in trusted_names:
                    trusted_names.append(name)

        score = score_topic(t, config, t["posts"])

        results.append({
            "tid": t["tid"],
            "title": t["title"],
            "brief": t.get("brief", ""),
            "postcount": t["postcount"],
            "category": t["category"],
            "age_months": t["age_months"],
            "is_solved": t["isSolved"],
            "has_trusted_poster": len(trusted_names) > 0,
            "trusted_posters": trusted_names,
            "signal_score": round(score, 3)
        })

    results.sort(key=lambda x: x["signal_score"], reverse=True)

    return {
        "query_terms": terms,
        "max_age_years": max_age_years,
        "total_raw_results": total_raw,
        "topics_after_filter": len(results),
        "filtered_out": total_filtered,
        "filter_reasons": filter_reasons,
        "topics": results
    }


def cmd_search(args, config):
    """Search the forum and return a filtered, scored topic list."""
    output = run_search(
        args.term, args.also, config,
        include_features=getattr(args, 'include_features', False),
        max_pages=args.max_pages,
        max_age=args.max_age
    )

    # Cache processed output
    safe_primary = re.sub(r'[^a-zA-Z0-9_-]', '_', args.term)
    ensure_cache_dir()
    with open(CACHE_DIR / f"search_{safe_primary}.json", "w", encoding="utf-8") as f:
        json.dump(output, f, indent=2, ensure_ascii=False)

    print(json.dumps(output, indent=2, ensure_ascii=False))


# ---------------------------------------------------------------------------
# FETCH command
# ---------------------------------------------------------------------------

def cmd_fetch(args, config):
    """Fetch and clean specific topics by ID."""
    all_output = []

    for tid in args.tids:
        try:
            topic_output = fetch_and_clean_topic(tid, config)
            all_output.append(topic_output)
        except Exception as e:
            print(f"[warn] Failed to fetch topic {tid}: {e}", file=sys.stderr)
            continue

    # Output as readable text for LLM consumption
    for topic in all_output:
        print(f"{'=' * 70}")
        print(f"TOPIC: {topic['title']}")
        tid = topic.get('tid', '?')
        url = topic.get('url', f'{FORUM_BASE}/topic/{tid}')
        print(f"URL: {url}")
        post_count = len(topic.get("posts", []))
        print(f"Posts: {topic.get('post_count_fetched', post_count)}/{topic.get('post_count_total', post_count)} fetched")
        print(f"{'=' * 70}")
        print()
        for post in topic["posts"]:
            print(f"--- {post['header']} ---")
            print(post["content"])
            print()


# ---------------------------------------------------------------------------
# UPDATE command (primary command for the /update-forum agent)
# ---------------------------------------------------------------------------

def cmd_update(args, config):
    """Search + diff + fetch + combine in one pass.

    This is the primary command used by the /update-forum agent.
    It searches the forum, diffs against already-scanned topics for the given
    scope, fetches only new topics, combines all topics into a single output
    file, and updates the scanned tracker.
    """
    scope = args.scope
    if not scope:
        print("Error: --scope is required for update command", file=sys.stderr)
        sys.exit(1)

    # Validate scope format
    if ":" not in scope:
        print(f"Error: scope must be 'domain:target' (e.g., 'modules:LFO'), got '{scope}'",
              file=sys.stderr)
        sys.exit(1)

    # Step 1: Search
    print(f"[1/5] Searching forum for '{args.term}'...", file=sys.stderr)
    search_result = run_search(
        args.term, args.also, config,
        include_features=getattr(args, 'include_features', False)
    )
    all_topics = search_result["topics"]
    before_count = len(all_topics)

    # Filter 1: minimum signal score
    min_score = getattr(args, 'min_score', None) or 0.15
    all_topics = [t for t in all_topics if t["signal_score"] >= min_score]
    score_dropped = before_count - len(all_topics)

    # Filter 2: title relevance - require any search term (primary or --also) in the title
    # Uses stem matching: "Delay" also matches "Delaying", "Delayed", etc.
    all_terms = [args.term] + (args.also or [])
    term_patterns = [re.compile(r'\b' + re.escape(t), re.IGNORECASE) for t in all_terms]
    before_relevance = len(all_topics)
    all_topics = [t for t in all_topics
                  if any(p.search(t.get("title", "")) for p in term_patterns)]
    relevance_dropped = before_relevance - len(all_topics)

    all_tids = [t["tid"] for t in all_topics]
    print(f"      Found {before_count} topics, kept {len(all_tids)} "
          f"(score dropped {score_dropped}, title relevance dropped {relevance_dropped})",
          file=sys.stderr)

    # Step 2: Diff against scanned tracker
    scanned = load_scanned()
    scope_data = scanned.get(scope, {})
    already_scanned = set(scope_data.get("topicIds", []))
    new_tids = [tid for tid in all_tids if tid not in already_scanned]
    print(f"[2/5] Diff: {len(already_scanned)} already scanned, {len(new_tids)} new",
          file=sys.stderr)

    if not new_tids and not already_scanned:
        print("      No topics found. Nothing to do.", file=sys.stderr)
        print(json.dumps({"scope": scope, "status": "empty", "topics": 0}))
        return

    # Step 3: Fetch new topics (use cache for already-fetched ones)
    print(f"[3/5] Fetching {len(new_tids)} new topics...", file=sys.stderr)
    fetched_count = 0
    for tid in new_tids:
        # Check if already cached from a different scope
        cached = load_cached_topic(tid)
        if cached:
            print(f"      Topic {tid}: using cache", file=sys.stderr)
            continue

        try:
            fetch_and_clean_topic(tid, config)
            fetched_count += 1
            print(f"      Topic {tid}: fetched", file=sys.stderr)
        except Exception as e:
            print(f"      Topic {tid}: FAILED - {e}", file=sys.stderr)

    print(f"      Fetched {fetched_count} from API, {len(new_tids) - fetched_count} from cache",
          file=sys.stderr)

    # Step 4: Combine all topics into single output file
    print(f"[4/5] Combining {len(all_tids)} topics...", file=sys.stderr)
    combined = []
    for tid in all_tids:
        topic = load_cached_topic(tid)
        if topic:
            combined.append(topic)
        else:
            print(f"      Topic {tid}: not in cache, skipping", file=sys.stderr)

    # Write combined output
    safe_scope = scope.replace(":", "_")
    output_path = CACHE_DIR / f"{safe_scope}.json"
    ensure_cache_dir()
    with open(output_path, "w", encoding="utf-8") as f:
        json.dump(combined, f, indent=2, ensure_ascii=False)

    total_posts = sum(len(t.get("posts", [])) for t in combined)
    file_size = output_path.stat().st_size

    # File size sanity check
    MAX_COMBINED_SIZE = 400_000  # 400KB ~ 100k tokens
    if file_size > MAX_COMBINED_SIZE:
        print(f"      WARNING: Combined output is {file_size // 1024}KB "
              f"(>{MAX_COMBINED_SIZE // 1024}KB). Search terms may be too broad.",
              file=sys.stderr)

    # Step 5: Update scanned tracker
    print(f"[5/5] Updating scanned tracker...", file=sys.stderr)
    all_combined_tids = [t["tid"] for t in combined]
    scanned[scope] = {
        "lastRun": datetime.now(timezone.utc).strftime("%Y-%m-%d"),
        "queries": [args.term] + (args.also or []),
        "topicIds": sorted(set(list(already_scanned) + all_combined_tids))
    }
    save_scanned(scanned)

    # Output summary as JSON
    summary = {
        "scope": scope,
        "status": "ok",
        "search_results": len(all_tids),
        "already_scanned": len(already_scanned),
        "new_topics": len(new_tids),
        "fetched_from_api": fetched_count,
        "combined_topics": len(combined),
        "combined_posts": total_posts,
        "output_file": str(output_path),
        "file_size_bytes": file_size,
        "approx_tokens": file_size // 4
    }
    print(json.dumps(summary, indent=2, ensure_ascii=False))


# ---------------------------------------------------------------------------
# EXTRACT-CODE command
# ---------------------------------------------------------------------------

def cmd_extract_code(args, config):
    """Extract code fences from a combined topic file.

    Filters for posts with upvotes > 0 and extracts code blocks with
    surrounding context. Output is a JSON file ready for LLM consumption
    (method tagging, quality rating).
    """
    scope = args.scope
    if not scope:
        print("Error: --scope is required", file=sys.stderr)
        sys.exit(1)

    # Load combined topic file
    safe_scope = scope.replace(":", "_")
    input_path = CACHE_DIR / f"{safe_scope}.json"
    if not input_path.exists():
        print(f"Error: Combined file not found: {input_path}", file=sys.stderr)
        print(f"Run 'update' first with --scope {scope}", file=sys.stderr)
        sys.exit(1)

    with open(input_path, "r", encoding="utf-8") as f:
        topics = json.load(f)

    # Extract code blocks from posts with upvotes > 0
    code_blocks = []
    for topic in topics:
        tid = topic["tid"]
        title = topic["title"]

        for post in topic.get("posts", []):
            upvotes = post.get("upvotes", 0)
            if upvotes < 1:
                continue

            content = post.get("content", "")

            # Find code fences in the cleaned content
            # Pattern: ``` ... ``` (multiline)
            fences = list(re.finditer(r'```\n?(.*?)\n?```', content, re.DOTALL))
            if not fences:
                continue

            for fence in fences:
                code = fence.group(1).strip()

                # Skip empty or trivial code blocks (< 2 lines)
                lines = code.split('\n')
                if len(lines) < 2:
                    continue

                # Skip HiseSnippet remnants
                if code.startswith('HiseSnippet') or '[HiseSnippet' in code:
                    continue

                # Extract surrounding context (text before the code fence)
                pre_text = content[:fence.start()].strip()
                # Get last 2-3 sentences of context
                sentences = re.split(r'[.!?\n]', pre_text)
                sentences = [s.strip() for s in sentences if s.strip()]
                context = '. '.join(sentences[-3:]) if sentences else ""
                # Truncate context
                if len(context) > 200:
                    context = context[:200] + "..."

                code_blocks.append({
                    "tid": tid,
                    "topic_title": title,
                    "username": post.get("username", "Unknown"),
                    "is_trusted": post.get("is_trusted", False),
                    "upvotes": upvotes,
                    "context": context,
                    "code": code
                })

    # Write output
    output_path = CACHE_DIR / f"{safe_scope}_code.json"
    output = {
        "scope": scope,
        "total_code_blocks": len(code_blocks),
        "source_topics": len(topics),
        "filter": "upvotes > 0",
        "code_blocks": code_blocks
    }

    with open(output_path, "w", encoding="utf-8") as f:
        json.dump(output, f, indent=2, ensure_ascii=False)

    # Print summary
    total_lines = sum(len(b["code"].split('\n')) for b in code_blocks)
    print(json.dumps({
        "scope": scope,
        "status": "ok",
        "code_blocks": len(code_blocks),
        "total_code_lines": total_lines,
        "source_topics": len(topics),
        "output_file": str(output_path),
        "file_size_bytes": output_path.stat().st_size,
        "approx_tokens": output_path.stat().st_size // 4
    }, indent=2, ensure_ascii=False))


# ---------------------------------------------------------------------------
# REFRESH-USERS command
# ---------------------------------------------------------------------------

def cmd_refresh_users(args, config):
    """Fetch users sorted by reputation and update config."""
    min_rep = args.min_reputation
    print(f"Fetching users with reputation >= {min_rep}...", file=sys.stderr)

    users = {}
    page = 1
    while True:
        try:
            data = api_get("users", {"section": "sort-reputation", "page": page})
        except Exception as e:
            print(f"[warn] Failed to fetch user page {page}: {e}", file=sys.stderr)
            break

        user_list = data.get("users", [])
        if not user_list:
            break

        found_below_threshold = False
        for user in user_list:
            rep = user.get("reputation", 0)
            if rep < min_rep:
                found_below_threshold = True
                break
            uid = str(user["uid"])
            users[uid] = {
                "name": user["username"],
                "reputation": rep,
                "postcount": user.get("postcount", 0),
                "role": "author" if uid == "1" else "trusted"
            }

        if found_below_threshold:
            break

        page_count = data.get("pagination", {}).get("pageCount", 1)
        if page >= page_count:
            break
        page += 1
        time.sleep(0.3)

    config["trusted_posters"] = users
    save_config(config)

    print(f"Updated config with {len(users)} trusted posters (reputation >= {min_rep}):",
          file=sys.stderr)
    for uid, info in sorted(users.items(), key=lambda x: x[1]["reputation"], reverse=True):
        print(f"  uid {uid:>5}: {info['name']:<25} rep={info['reputation']}", file=sys.stderr)

    print(json.dumps({"trusted_posters_count": len(users), "users": users}, indent=2))


# ---------------------------------------------------------------------------
# REBUILD command (re-filter cached topics without re-fetching)
# ---------------------------------------------------------------------------

def cmd_rebuild(args, config):
    """Re-filter per-topic cache files and rebuild the combined output.

    Applies the current signal filter (OP + trusted + upvoted) to existing
    cached topics without hitting the forum API. Use after changing the
    post filter logic.
    """
    scope = args.scope
    if not scope or ":" not in scope:
        print(f"Error: scope must be 'domain:target', got '{scope}'", file=sys.stderr)
        sys.exit(1)

    scanned = load_scanned()
    scope_data = scanned.get(scope, {})
    topic_ids = scope_data.get("topicIds", [])
    if not topic_ids:
        print(f"No scanned topics for scope '{scope}'", file=sys.stderr)
        sys.exit(1)

    trusted = config.get("trusted_posters", {})
    trusted_uids = set(trusted.keys())

    print(f"[1/2] Re-filtering {len(topic_ids)} cached topics for '{scope}'...",
          file=sys.stderr)
    combined = []
    total_before = 0
    total_after = 0

    for tid in topic_ids:
        topic = load_cached_topic(tid)
        if not topic:
            print(f"      Topic {tid}: not in cache, skipping", file=sys.stderr)
            continue

        posts = topic.get("posts", [])
        total_before += len(posts)

        # Apply signal filter: OP + author/expert + upvoted only
        def is_signal_post(p):
            if p.get("is_op"):
                return True
            if p.get("upvotes", 0) > 0:
                return True
            # Only auto-keep author/expert roles, not all trusted
            uid = str(p.get("uid", ""))
            role = trusted.get(uid, {}).get("role", "")
            return role in ("author", "expert")
        filtered = [p for p in posts if is_signal_post(p)]
        total_after += len(filtered)

        # Slim down posts: keep header + content + upvotes (if > 0)
        slim = []
        for p in filtered:
            sp = {"header": p.get("header", ""), "content": p.get("content", "")}
            if p.get("upvotes", 0) > 0:
                sp["upvotes"] = p["upvotes"]
            slim.append(sp)

        topic["posts"] = slim
        # Remove unnecessary topic-level fields
        for key in ("url", "post_count_total", "post_count_fetched"):
            topic.pop(key, None)

        # Write filtered version back to per-topic cache
        cache_topic(tid, topic)
        combined.append(topic)

    # Write combined output
    safe_scope = scope.replace(":", "_")
    output_path = CACHE_DIR / f"{safe_scope}.json"
    ensure_cache_dir()
    with open(output_path, "w", encoding="utf-8") as f:
        json.dump(combined, f, indent=2, ensure_ascii=False)

    file_size = output_path.stat().st_size
    reduction = (1 - total_after / max(total_before, 1)) * 100

    print(f"[2/2] Rebuilt {len(combined)} topics", file=sys.stderr)
    print(f"      Posts: {total_before} -> {total_after} "
          f"({reduction:.0f}% reduction)", file=sys.stderr)
    print(f"      File: {file_size // 1024}KB", file=sys.stderr)

    MAX_COMBINED_SIZE = 400_000
    if file_size > MAX_COMBINED_SIZE:
        print(f"      WARNING: Still >{MAX_COMBINED_SIZE // 1024}KB. "
              f"Search terms may be too broad.", file=sys.stderr)

    summary = {
        "scope": scope,
        "status": "ok",
        "topics": len(combined),
        "posts_before": total_before,
        "posts_after": total_after,
        "reduction_pct": round(reduction, 1),
        "file_size_bytes": file_size,
        "approx_tokens": file_size // 4
    }
    print(json.dumps(summary, indent=2, ensure_ascii=False))


# ---------------------------------------------------------------------------
# CRAWL-CODE command (category-based code extraction for vector DB)
# ---------------------------------------------------------------------------

CODE_KEYWORDS_RE = re.compile(
    r'\b(Content|Engine|Synth|Graphics|Server|Math|Array|Buffer|Message|Timer|'
    r'ScriptPanel|FileSystem|Sampler|MidiPlayer|ScriptSlider|ScriptButton|'
    r'ScriptComboBox|ScriptTable|Broadcaster|UserPresetHandler|'
    r'inline\s+function|reg\s+\w|local\s+\w|include\(")\b'
)

# Patterns that indicate non-HISE code (PHP, Python, C++, etc.)
NON_HISE_RE = re.compile(
    r'\b(add_filter|add_action|def\s+\w+\(|#include\s*<|import\s+\w+|'
    r'class\s+\w+\s*:|pip\s+install|npm\s+install|require\(|from\s+\w+\s+import)\b'
)


# ---------------------------------------------------------------------------
# HiseSnippet decoding (JUCE custom base64 + zlib)
# ---------------------------------------------------------------------------

_JUCE_B64_DECODE_TABLE = [
    63, 0, 0, 0, 0, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 0, 0, 0, 0, 0, 0, 0,
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22,
    23, 24, 25, 26, 0, 0, 0, 0, 0, 0, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37,
    38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52
]

_HISE_SNIPPET_RE = re.compile(r'HiseSnippet \d+\.[A-Za-z0-9.+]+')


def decode_juce_base64(s):
    """Decode a JUCE MemoryBlock base64 string (format: <size>.<data>).

    JUCE uses a non-standard base64 alphabet:
      .ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+
    The first '.' after the decimal size is a separator; subsequent '.' are
    index-0 in the alphabet.
    """
    dot = s.index('.')
    num_bytes = int(s[:dot])
    data = bytearray(num_bytes)
    src = s[dot + 1:]
    pos = 0
    for ch in src:
        c = ord(ch) - 43
        if 0 <= c < len(_JUCE_B64_DECODE_TABLE):
            bits_to_set = _JUCE_B64_DECODE_TABLE[c]
            byte_idx = pos >> 3
            offset_in_byte = pos & 7
            num_bits = 6
            while num_bits > 0 and byte_idx < num_bytes:
                bits_this_time = min(num_bits, 8 - offset_in_byte)
                bit_mask = (1 << bits_this_time) - 1
                clear_mask = ~(bit_mask << offset_in_byte) & 0xFF
                set_bits = (bits_to_set & bit_mask) << offset_in_byte
                data[byte_idx] = (data[byte_idx] & clear_mask) | set_bits
                byte_idx += 1
                num_bits -= bits_this_time
                bits_to_set >>= bits_this_time
                offset_in_byte = 0
            pos += 6
    return bytes(data)


def decode_hise_snippet(snippet_string):
    """Decode a HiseSnippet string → decompressed JUCE ValueTree bytes."""
    import zlib as _zlib
    data = snippet_string.strip()
    if data.startswith("HiseSnippet "):
        data = data[len("HiseSnippet "):]
    raw = decode_juce_base64(data)
    return _zlib.decompress(raw)


def extract_script_from_snippet(snippet_string):
    """Extract the Interface script code from a HiseSnippet string.

    Decodes the base64, decompresses, and finds the script content in the
    JUCE binary ValueTree. Returns the script string or None.
    """
    try:
        vt_bytes = decode_hise_snippet(snippet_string)
    except Exception:
        return None

    # The script is stored as readable text in the binary ValueTree.
    # Find runs of printable ASCII (including tabs/newlines) that contain
    # HISE API markers.
    strings = re.findall(rb'[\x09\x0a\x0d\x20-\x7e]{20,}', vt_bytes)
    for s in strings:
        text = s.decode('ascii', errors='replace')
        if any(marker in text for marker in
               ('Content.make', 'function onNoteOn', 'inline function',
                'const var', 'namespace ')):
            return text.strip()
    return None


def find_snippets_in_html(html_content):
    """Find all HiseSnippet strings in raw HTML post content."""
    return _HISE_SNIPPET_RE.findall(html_content)


_CALLBACK_RE = re.compile(
    r'\s*function\s+on(NoteOn|NoteOff|Controller|Timer|Control)\s*\([^)]*\)\s*\{([^}]*)\}',
    re.DOTALL
)


def strip_callbacks_from_script(script):
    """Strip HISE callback functions from an extracted script.

    Returns (cleaned_script, has_code_in_callbacks).
    If any callback contains actual code, has_code_in_callbacks is True
    and the snippet should be rejected (not copy-pasteable).
    """
    has_code = False
    def replacer(m):
        nonlocal has_code
        body = m.group(2).strip()
        if body:
            has_code = True
        return ''
    cleaned = _CALLBACK_RE.sub(replacer, script).strip()
    return cleaned, has_code


def extract_snippets_from_post(post_html, config):
    """Find HiseSnippets in raw HTML, decode, and extract scripts.

    Returns list of dicts: {code, context, had_callback_code}
    """
    snippets = find_snippets_in_html(post_html)
    if not snippets:
        return []

    # Get cleaned text for context (strip the snippets themselves)
    context_text = re.sub(
        r'HiseSnippet \d+\.[A-Za-z0-9.+]+', '', post_html
    )
    context_text = strip_html(context_text)
    sentences = re.split(r'[.!?\n]', context_text)
    sentences = [s.strip() for s in sentences if s.strip()]
    context = '. '.join(sentences[-3:]) if sentences else ""
    if len(context) > 300:
        context = context[:300] + "..."

    results = []
    for snippet_str in snippets:
        script = extract_script_from_snippet(snippet_str)
        if not script:
            continue

        cleaned, had_callback_code = strip_callbacks_from_script(script)
        if not cleaned or len(cleaned.split('\n')) < 3:
            continue

        results.append({
            "code": cleaned,
            "context": context,
            "had_callback_code": had_callback_code
        })

    return results


CRAWL_STATE_PATH = SCRIPT_DIR / "forum_search" / "crawl_state.json"


def load_crawl_state():
    if CRAWL_STATE_PATH.exists():
        with open(CRAWL_STATE_PATH, "r", encoding="utf-8") as f:
            return json.load(f)
    return {"last_crawl": None, "categories": {}, "processed_tids": []}


def save_crawl_state(state):
    CRAWL_STATE_PATH.parent.mkdir(parents=True, exist_ok=True)
    with open(CRAWL_STATE_PATH, "w", encoding="utf-8") as f:
        json.dump(state, f, indent=2, ensure_ascii=False)


def crawl_category_topics(cid, config, max_topics=2000, start_page=1):
    """Walk a category via /api/category/{cid}?page=N and return topic metadata."""
    excluded = set(config.get("excluded_categories", []))
    if cid in excluded:
        print(f"      Category {cid}: excluded, skipping", file=sys.stderr)
        return [], 0

    topics = []
    page = start_page
    while len(topics) < max_topics:
        try:
            data = api_get(f"category/{cid}", {"page": page})
        except Exception as e:
            print(f"[warn] Failed to fetch category {cid} page {page}: {e}",
                  file=sys.stderr)
            break

        topic_list = data.get("topics", [])
        if not topic_list:
            break

        for t in topic_list:
            topics.append({
                "tid": t.get("tid"),
                "title": strip_html(t.get("titleRaw", t.get("title", ""))),
                "postcount": t.get("postcount", 0),
                "isSolved": bool(t.get("isSolved")),
                "cid": cid,
                "category": data.get("name", f"Category {cid}")
            })

        page_count = data.get("pagination", {}).get("pageCount", 1)
        if page >= page_count:
            page = page_count  # signal: reached the end
            break
        page += 1
        time.sleep(0.3)

    return topics[:max_topics], page


def fetch_topic_for_code(tid, config):
    """Fetch a topic from the API with minimal filtering — keeps all upvoted posts.

    Unlike fetch_and_clean_topic (which aggressively filters for LLM context),
    this keeps any post with upvotes > 0 to maximize code extraction coverage.
    """
    defaults = config["defaults"]
    max_posts = defaults["max_posts_per_topic"]
    max_words = defaults.get("max_post_words", 500)
    trusted = config.get("trusted_posters", {})

    topic_data = fetch_topic_posts(tid, max_posts, trusted)

    cleaned_posts = []
    for post in topic_data["posts"]:
        uid = str(post.get("uid", ""))
        username = post.get("user", {}).get("username", "Unknown")
        upvotes = post.get("upvotes", 0)
        is_trusted = uid in trusted
        trusted_role = trusted.get(uid, {}).get("role", "") if is_trusted else ""

        content = clean_post_content(post.get("content", ""), max_words)
        if not content.strip():
            continue

        cleaned_posts.append({
            "uid": uid,
            "username": username,
            "role": trusted_role,
            "is_trusted": is_trusted,
            "upvotes": upvotes,
            "content": content
        })

    return {
        "tid": tid,
        "title": topic_data.get("title", f"Topic {tid}"),
        "posts": cleaned_posts
    }


def extract_code_blocks_from_posts(topic_data, config, min_upvotes_author=1,
                                   min_upvotes_other=2, min_lines=3):
    """Extract qualifying code blocks from a fetched topic.

    Returns list of dicts with code, context, username, upvotes, etc.
    """
    blocks = []
    tid = topic_data.get("tid")
    title = topic_data.get("title", f"Topic {tid}")

    for post in topic_data.get("posts", []):
        content = post.get("content", "")
        upvotes = post.get("upvotes", 0)
        username = post.get("username", "Unknown")
        role = post.get("role", "")
        is_authority = role in ("author", "expert")

        # Post-level filter: author/expert need upvotes >= 1, others need >= 2
        if is_authority:
            if upvotes < min_upvotes_author:
                continue
        else:
            if upvotes < min_upvotes_other:
                continue

        # Find code fences (with optional language hint)
        fences = list(re.finditer(
            r'```(\w*)\n?(.*?)\n?```', content, re.DOTALL
        ))
        if not fences:
            continue

        for fence in fences:
            lang_hint = fence.group(1).lower()
            code = fence.group(2).strip()
            lines = code.split('\n')

            # Code-block-level filters
            if len(lines) < min_lines:
                continue
            if code.startswith('HiseSnippet') or '[HiseSnippet' in code:
                continue
            if len(code) > 5000:
                continue

            # If language hint says javascript/js, trust it as HISE code
            is_js_hint = lang_hint in ('javascript', 'js')

            if not is_js_hint:
                if not CODE_KEYWORDS_RE.search(code):
                    continue
                if NON_HISE_RE.search(code):
                    continue

            # Extract surrounding context
            pre_text = content[:fence.start()].strip()
            sentences = re.split(r'[.!?\n]', pre_text)
            sentences = [s.strip() for s in sentences if s.strip()]
            context = '. '.join(sentences[-3:]) if sentences else ""
            if len(context) > 300:
                context = context[:300] + "..."

            blocks.append({
                "tid": tid,
                "topic_title": title,
                "username": username,
                "role": role,
                "upvotes": upvotes,
                "context": context,
                "code": code
            })

    return blocks


def cmd_crawl_code(args, config):
    """Crawl forum categories and extract qualifying code blocks."""
    # Determine categories to crawl
    if args.categories:
        categories = [int(c.strip()) for c in args.categories.split(",")]
    else:
        # Default: all known categories except excluded
        excluded = set(config.get("excluded_categories", []))
        cat_weights = config.get("category_weights", {})
        categories = [int(c) for c in cat_weights.keys() if int(c) not in excluded]
        if not categories:
            categories = [2, 3, 5, 7, 8, 15, 17]

    max_topics = args.max_topics
    min_lines = args.min_lines
    output_path = Path(args.output)
    trusted = config.get("trusted_posters", {})
    resume = getattr(args, 'resume', False)

    # Load crawl state for resume / incremental support
    state = load_crawl_state()
    already_processed = set(state.get("processed_tids", []))

    if resume:
        print(f"Resuming crawl. {len(already_processed)} topics already processed.",
              file=sys.stderr)

    print(f"Crawling categories: {categories}", file=sys.stderr)
    print(f"Max topics: {max_topics}, min code lines: {min_lines}", file=sys.stderr)

    # Step 1: Collect topic IDs from all categories
    all_topic_meta = []
    for cid in categories:
        # Resume from last page if available
        start_page = 1
        if resume:
            start_page = state.get("categories", {}).get(str(cid), {}).get("next_page", 1)
            if start_page > 1:
                print(f"[1/4] Resuming category {cid} from page {start_page}...",
                      file=sys.stderr)

        print(f"[1/4] Listing topics in category {cid}...", file=sys.stderr)
        topics, last_page = crawl_category_topics(
            cid, config, max_topics=max_topics, start_page=start_page
        )
        print(f"      Found {len(topics)} topics (ended at page {last_page})",
              file=sys.stderr)
        all_topic_meta.extend(topics)

        # Save page progress
        if str(cid) not in state["categories"]:
            state["categories"][str(cid)] = {}
        state["categories"][str(cid)]["last_page"] = last_page
        state["categories"][str(cid)]["next_page"] = last_page + 1

    # Deduplicate by tid (topics can appear in subcategories)
    seen_tids = set()
    unique_topics = []
    for t in all_topic_meta:
        if t["tid"] not in seen_tids:
            seen_tids.add(t["tid"])
            unique_topics.append(t)

    # Skip already-processed topics when resuming
    if resume and already_processed:
        before = len(unique_topics)
        unique_topics = [t for t in unique_topics if t["tid"] not in already_processed]
        skipped = before - len(unique_topics)
        if skipped:
            print(f"      Skipped {skipped} already-processed topics", file=sys.stderr)

    # Cap total topics
    if len(unique_topics) > max_topics:
        unique_topics = unique_topics[:max_topics]

    print(f"      Total unique topics to process: {len(unique_topics)}", file=sys.stderr)

    # Step 2+3: Fetch topics and extract code blocks
    print(f"[2/4] Fetching {len(unique_topics)} topics and extracting code...",
          file=sys.stderr)
    all_code_blocks = []
    code_hashes = set()
    fetched = 0
    failed = 0

    for i, t_meta in enumerate(unique_topics):
        tid = t_meta["tid"]
        try:
            topic = fetch_topic_for_code(tid, config)
            fetched += 1
            if fetched % 20 == 0:
                print(f"      Processed {fetched} topics...", file=sys.stderr)
        except Exception as e:
            print(f"      Topic {tid}: FAILED - {e}", file=sys.stderr)
            failed += 1
            continue

        blocks = extract_code_blocks_from_posts(
            topic, config,
            min_upvotes_author=1,
            min_upvotes_other=2,
            min_lines=min_lines
        )

        for block in blocks:
            # Deduplicate by normalized code hash
            normalized = re.sub(r'\s+', ' ', block["code"]).strip()
            code_hash = hash(normalized)
            if code_hash in code_hashes:
                continue
            code_hashes.add(code_hash)

            # Build tags
            tags = ["forum"]
            if t_meta.get("category"):
                tags.append(t_meta["category"])
            if block["role"]:
                tags.append(block["role"])
            if t_meta.get("isSolved"):
                tags.append("solved")

            all_code_blocks.append({
                "title": block["topic_title"],  # placeholder, LLM replaces later
                "category": "Forum",
                "tags": tags,
                "description": block["context"],  # placeholder, LLM replaces later
                "code": block["code"],
                "url": f"{FORUM_BASE}/topic/{block['tid']}",
                "_meta": {
                    "username": block["username"],
                    "upvotes": block["upvotes"],
                    "needs_llm": True
                }
            })

    print(f"      Done: {fetched} fetched, {failed} failed", file=sys.stderr)
    print(f"[3/4] Extracted {len(all_code_blocks)} code blocks", file=sys.stderr)

    # Step 4: Write output
    print(f"[4/4] Writing output...", file=sys.stderr)
    ensure_cache_dir()
    with open(output_path, "w", encoding="utf-8") as f:
        json.dump(all_code_blocks, f, indent=2, ensure_ascii=False)

    # Update crawl state
    new_tids = [t["tid"] for t in unique_topics]
    state["processed_tids"] = sorted(set(state.get("processed_tids", []) + new_tids))
    state["last_crawl"] = datetime.now(timezone.utc).strftime("%Y-%m-%d")
    save_crawl_state(state)

    file_size = output_path.stat().st_size
    total_lines = sum(len(b["code"].split('\n')) for b in all_code_blocks)

    summary = {
        "status": "ok",
        "categories_crawled": len(categories),
        "topics_processed": len(unique_topics),
        "total_processed_all_time": len(state["processed_tids"]),
        "code_blocks": len(all_code_blocks),
        "total_code_lines": total_lines,
        "output_file": str(output_path),
        "file_size_bytes": file_size,
        "approx_tokens": file_size // 4,
        "needs_describe": True
    }
    print(json.dumps(summary, indent=2, ensure_ascii=False))


# ---------------------------------------------------------------------------
# CRAWL-SNIPPETS command (HiseSnippet extraction for vector DB)
# ---------------------------------------------------------------------------

def cmd_crawl_snippets(args, config):
    """Crawl forum categories and extract scripts from HiseSnippets."""
    if args.categories:
        categories = [int(c.strip()) for c in args.categories.split(",")]
    else:
        excluded = set(config.get("excluded_categories", []))
        cat_weights = config.get("category_weights", {})
        categories = [int(c) for c in cat_weights.keys() if int(c) not in excluded]
        if not categories:
            categories = [2, 3, 5, 7, 8, 15, 17]

    max_topics = args.max_topics
    output_path = Path(args.output)
    trusted = config.get("trusted_posters", {})
    resume = getattr(args, 'resume', False)

    state = load_crawl_state()
    already_processed = set(state.get("snippet_processed_tids", []))

    if resume:
        print(f"Resuming snippet crawl. {len(already_processed)} topics already processed.",
              file=sys.stderr)

    print(f"Crawling categories for HiseSnippets: {categories}", file=sys.stderr)
    print(f"Max topics: {max_topics}", file=sys.stderr)

    # Step 1: Collect topic IDs
    all_topic_meta = []
    for cid in categories:
        start_page = 1
        if resume:
            start_page = state.get("categories", {}).get(
                f"snip_{cid}", {}
            ).get("next_page", 1)
            if start_page > 1:
                print(f"[1/4] Resuming category {cid} from page {start_page}...",
                      file=sys.stderr)

        print(f"[1/4] Listing topics in category {cid}...", file=sys.stderr)
        topics, last_page = crawl_category_topics(
            cid, config, max_topics=max_topics, start_page=start_page
        )
        print(f"      Found {len(topics)} topics (ended at page {last_page})",
              file=sys.stderr)
        all_topic_meta.extend(topics)

        if f"snip_{cid}" not in state.get("categories", {}):
            state.setdefault("categories", {})[f"snip_{cid}"] = {}
        state["categories"][f"snip_{cid}"]["last_page"] = last_page
        state["categories"][f"snip_{cid}"]["next_page"] = last_page + 1

    # Deduplicate
    seen_tids = set()
    unique_topics = []
    for t in all_topic_meta:
        if t["tid"] not in seen_tids:
            seen_tids.add(t["tid"])
            unique_topics.append(t)

    if resume and already_processed:
        before = len(unique_topics)
        unique_topics = [t for t in unique_topics if t["tid"] not in already_processed]
        skipped = before - len(unique_topics)
        if skipped:
            print(f"      Skipped {skipped} already-processed topics", file=sys.stderr)

    if len(unique_topics) > max_topics:
        unique_topics = unique_topics[:max_topics]

    print(f"      Total unique topics to process: {len(unique_topics)}", file=sys.stderr)

    # Step 2+3: Fetch topics and extract snippets
    print(f"[2/4] Fetching {len(unique_topics)} topics and extracting snippets...",
          file=sys.stderr)
    all_blocks = []
    code_hashes = set()
    fetched = 0
    failed = 0
    rejected_callbacks = 0

    defaults = config["defaults"]
    max_posts = defaults["max_posts_per_topic"]

    for i, t_meta in enumerate(unique_topics):
        tid = t_meta["tid"]
        try:
            # Fetch raw posts (not cleaned — need HTML for snippet detection)
            topic_data = fetch_topic_posts(tid, max_posts, trusted)
            fetched += 1
            if fetched % 20 == 0:
                print(f"      Processed {fetched} topics...", file=sys.stderr)
        except Exception as e:
            print(f"      Topic {tid}: FAILED - {e}", file=sys.stderr)
            failed += 1
            continue

        title = topic_data.get("title", f"Topic {tid}")

        for post in topic_data.get("posts", []):
            uid = str(post.get("uid", ""))
            username = post.get("user", {}).get("username", "Unknown")
            upvotes = post.get("upvotes", 0)
            is_trusted = uid in trusted
            trusted_role = trusted.get(uid, {}).get("role", "") if is_trusted else ""
            is_authority = trusted_role in ("author", "expert")

            # Post-level quality filter
            if is_authority:
                if upvotes < 1:
                    continue
            else:
                if upvotes < 2:
                    continue

            raw_html = post.get("content", "")
            extracted = extract_snippets_from_post(raw_html, config)

            for item in extracted:
                if item["had_callback_code"]:
                    rejected_callbacks += 1
                    continue

                # Deduplicate
                normalized = re.sub(r'\s+', ' ', item["code"]).strip()
                code_hash = hash(normalized)
                if code_hash in code_hashes:
                    continue
                code_hashes.add(code_hash)

                tags = ["forum"]
                if t_meta.get("category"):
                    tags.append(t_meta["category"])
                if trusted_role:
                    tags.append(trusted_role)
                if t_meta.get("isSolved"):
                    tags.append("solved")

                all_blocks.append({
                    "title": title,
                    "category": "Forum",
                    "tags": tags,
                    "description": item["context"],
                    "code": item["code"],
                    "url": f"{FORUM_BASE}/topic/{tid}",
                    "_meta": {
                        "username": username,
                        "upvotes": upvotes,
                        "needs_llm": True
                    }
                })

    print(f"      Done: {fetched} fetched, {failed} failed", file=sys.stderr)
    print(f"      Rejected (callback code): {rejected_callbacks}", file=sys.stderr)
    print(f"[3/4] Extracted {len(all_blocks)} snippet scripts", file=sys.stderr)

    # Step 4: Write output
    print(f"[4/4] Writing output...", file=sys.stderr)
    ensure_cache_dir()
    with open(output_path, "w", encoding="utf-8") as f:
        json.dump(all_blocks, f, indent=2, ensure_ascii=False)

    # Update crawl state
    new_tids = [t["tid"] for t in unique_topics]
    state["snippet_processed_tids"] = sorted(
        set(state.get("snippet_processed_tids", []) + new_tids)
    )
    state["last_snippet_crawl"] = datetime.now(timezone.utc).strftime("%Y-%m-%d")
    save_crawl_state(state)

    file_size = output_path.stat().st_size
    total_lines = sum(len(b["code"].split('\n')) for b in all_blocks)

    summary = {
        "status": "ok",
        "categories_crawled": len(categories),
        "topics_processed": len(unique_topics),
        "total_processed_all_time": len(state["snippet_processed_tids"]),
        "snippet_scripts": len(all_blocks),
        "rejected_callback_code": rejected_callbacks,
        "total_code_lines": total_lines,
        "output_file": str(output_path),
        "file_size_bytes": file_size,
        "needs_describe": True
    }
    print(json.dumps(summary, indent=2, ensure_ascii=False))


# ---------------------------------------------------------------------------
# DESCRIBE-CODE command (LLM post-processing for title & description)
# ---------------------------------------------------------------------------

def cmd_describe_code(args, config):
    """Export raw code blocks as JSONL for LLM triage and description.

    Reads the raw output from crawl-code and writes a JSONL file where each
    line contains the code, context, and metadata needed for triage.
    The LLM processing follows the rules in style-guide/forum-code-scraper.md.
    """
    input_path = Path(args.input)
    if not input_path.exists():
        print(f"Error: Input file not found: {input_path}", file=sys.stderr)
        sys.exit(1)

    with open(input_path, "r", encoding="utf-8") as f:
        blocks = json.load(f)

    output_path = Path(args.output) if args.output else input_path.with_name(
        input_path.stem + "_described.json"
    )

    # Filter to blocks that still need LLM processing
    needs_llm = [b for b in blocks if b.get("_meta", {}).get("needs_llm", False)]
    already_done = len(blocks) - len(needs_llm)

    if not needs_llm:
        print("All blocks already have descriptions. Nothing to do.", file=sys.stderr)
        return

    print(f"Blocks to process: {len(needs_llm)} ({already_done} already done)",
          file=sys.stderr)

    # Apply offset and batch size
    offset = getattr(args, 'offset', 0) or 0
    batch_size = getattr(args, 'batch_size', 0) or 0

    if offset >= len(needs_llm):
        print(f"Offset {offset} >= total blocks {len(needs_llm)}. Nothing to do.",
              file=sys.stderr)
        return

    if batch_size > 0:
        batch = needs_llm[offset:offset + batch_size]
    else:
        batch = needs_llm[offset:]

    # Build output filename with batch suffix
    if batch_size > 0:
        batch_suffix = f"_batch_{offset}_{offset + len(batch)}"
        prompts_path = output_path.with_name(output_path.stem + batch_suffix + ".jsonl")
    else:
        prompts_path = output_path.with_name(output_path.stem + "_prompts.jsonl")

    # Write raw blocks as JSONL for external LLM processing
    with open(prompts_path, "w", encoding="utf-8") as pf:
        for i, block in enumerate(batch):
            global_index = offset + i
            pf.write(json.dumps({
                "index": global_index,
                "topic_title": block.get("title", ""),
                "context": block.get("description", "")[:300],
                "code": block["code"][:3000],
                "username": block.get("_meta", {}).get("username", ""),
                "upvotes": block.get("_meta", {}).get("upvotes", 0),
                "tags": block.get("tags", []),
                "url": block.get("url", "")
            }, ensure_ascii=False) + "\n")

    print(f"Wrote {len(batch)} blocks (offset {offset}) to: {prompts_path}",
          file=sys.stderr)

    if batch_size > 0:
        remaining = len(needs_llm) - offset - len(batch)
        if remaining > 0:
            print(f"{remaining} blocks remaining. Next batch:",
                  file=sys.stderr)
            print(f"  python forum-search.py describe-code --input {input_path} "
                  f"--batch-size {batch_size} --offset {offset + len(batch)}",
                  file=sys.stderr)

    print(f"Process with LLM following style-guide/forum-code-scraper.md, then run:",
          file=sys.stderr)
    print(f"  python forum-search.py apply-descriptions "
          f"--input {input_path} --responses <responses.jsonl> --output {output_path}",
          file=sys.stderr)

    summary = {
        "status": "blocks_exported",
        "total_blocks": len(blocks),
        "needs_llm": len(needs_llm),
        "batch_offset": offset,
        "batch_size": len(batch),
        "prompts_file": str(prompts_path),
        "next_step": "Process with LLM per style-guide/forum-code-scraper.md, then run apply-descriptions"
    }
    print(json.dumps(summary, indent=2, ensure_ascii=False))


# ---------------------------------------------------------------------------
# APPLY-DESCRIPTIONS command (merge LLM responses back into dataset)
# ---------------------------------------------------------------------------

CODE_EXAMPLES_DIR = SCRIPT_DIR / "code_examples"


def next_batch_path(prefix="batch"):
    """Find the next batch number in code_examples/."""
    CODE_EXAMPLES_DIR.mkdir(parents=True, exist_ok=True)
    existing = sorted(CODE_EXAMPLES_DIR.glob(f"{prefix}_*.json"))
    if not existing:
        return CODE_EXAMPLES_DIR / f"{prefix}_001.json"
    last_num = int(existing[-1].stem.split("_")[-1])
    return CODE_EXAMPLES_DIR / f"{prefix}_{last_num + 1:03d}.json"


def cmd_apply_descriptions(args, config):
    """Merge LLM triage results back into the code dataset.

    Handles three verdicts:
    - KEEP: update title and description, keep original code
    - FIX: update title, description, AND replace code with fixed version
    - REJECT: drop the block entirely

    Output goes to forum-search/code_examples/batch_NNN.json by default.
    """
    input_path = Path(args.input)
    responses_path = Path(args.responses)
    prefix = "snippet_batch" if getattr(args, 'snippet', False) else "batch"
    output_path = Path(args.output) if args.output else next_batch_path(prefix)

    with open(input_path, "r", encoding="utf-8") as f:
        blocks = json.load(f)

    # Parse responses
    responses = {}
    with open(responses_path, "r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            resp = json.loads(line)
            responses[resp["index"]] = resp.get("response", "")

    needs_llm = [i for i, b in enumerate(blocks) if b.get("_meta", {}).get("needs_llm")]
    kept = 0
    fixed = 0
    rejected = 0
    failed = 0
    reject_indices = set()

    for seq, block_idx in enumerate(needs_llm):
        resp_text = responses.get(seq, "")
        if not resp_text:
            failed += 1
            continue

        # Parse VERDICT
        verdict_match = re.search(r'VERDICT:\s*(\w+)', resp_text)
        verdict = verdict_match.group(1).upper() if verdict_match else ""

        if verdict == "REJECT":
            reject_indices.add(block_idx)
            reason_match = re.search(r'REASON:\s*(.+)', resp_text)
            reason = reason_match.group(1).strip() if reason_match else "no reason given"
            print(f"  REJECT [{block_idx}]: {reason}", file=sys.stderr)
            rejected += 1
            continue

        # Parse TITLE and DESCRIPTION (required for KEEP and FIX)
        title_match = re.search(r'TITLE:\s*(.+)', resp_text)
        desc_match = re.search(r'DESCRIPTION:\s*(.+?)(?:\nCODE:|\Z)', resp_text, re.DOTALL)

        if not (title_match and desc_match):
            failed += 1
            print(f"[warn] Could not parse response for block {block_idx}: {resp_text[:100]}",
                  file=sys.stderr)
            continue

        blocks[block_idx]["title"] = title_match.group(1).strip()
        blocks[block_idx]["description"] = desc_match.group(1).strip()

        # Parse FEATURED flag
        featured_match = re.search(r'FEATURED:\s*(\w+)', resp_text)
        if featured_match:
            blocks[block_idx]["featured"] = featured_match.group(1).strip().lower() == "yes"

        if verdict == "FIX":
            # Extract fixed code after CODE: marker
            code_match = re.search(r'CODE:\s*\n(.*)', resp_text, re.DOTALL)
            if code_match:
                blocks[block_idx]["code"] = code_match.group(1).strip()
                fixed += 1
            else:
                # CODE block missing — treat as KEEP with warning
                print(f"[warn] FIX verdict but no CODE block for [{block_idx}], keeping original",
                      file=sys.stderr)
                kept += 1
        else:
            kept += 1

        blocks[block_idx].pop("_meta", None)

    # Build final output — drop rejected blocks and strip _meta
    final_blocks = []
    for i, b in enumerate(blocks):
        if i in reject_indices:
            continue
        clean = {k: v for k, v in b.items() if k != "_meta"}
        final_blocks.append(clean)

    with open(output_path, "w", encoding="utf-8") as f:
        json.dump(final_blocks, f, indent=2, ensure_ascii=False)

    featured_count = sum(1 for b in final_blocks if b.get("featured"))

    summary = {
        "status": "ok",
        "total_blocks": len(blocks),
        "kept": kept,
        "fixed": fixed,
        "rejected": rejected,
        "failed": failed,
        "output_blocks": len(final_blocks),
        "featured": featured_count,
        "output_file": str(output_path)
    }
    print(json.dumps(summary, indent=2, ensure_ascii=False))


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description="HISE Forum Search Tool - pre-filter and clean forum data for LLM pipelines"
    )
    subparsers = parser.add_subparsers(dest="command", required=True)

    # search
    sp_search = subparsers.add_parser("search", help="Search forum, return filtered topic list")
    sp_search.add_argument("term", help="Primary search term")
    sp_search.add_argument("--also", nargs="*", help="Additional search terms")
    sp_search.add_argument("--max-age", type=float, help="Max age in years (default: from config)")
    sp_search.add_argument("--max-pages", type=int, help="Max search result pages per term")
    sp_search.add_argument("--include-features", action="store_true",
                           help="Include Feature Requests category (normally excluded)")

    # fetch
    sp_fetch = subparsers.add_parser("fetch", help="Fetch and clean specific topics")
    sp_fetch.add_argument("tids", nargs="+", type=int, help="Topic IDs to fetch")

    # update (primary command for /update-forum agent)
    sp_update = subparsers.add_parser("update",
        help="Search + diff + fetch + combine in one pass")
    sp_update.add_argument("term", help="Primary search term")
    sp_update.add_argument("--also", nargs="*", help="Additional search terms")
    sp_update.add_argument("--scope", required=True,
                           help="Scope identifier (e.g., 'modules:LFO', 'api:ScriptPanel')")
    sp_update.add_argument("--min-score", type=float, default=0.15,
                            help="Minimum signal score to include a topic (default: 0.15)")
    sp_update.add_argument("--include-features", action="store_true",
                            help="Include Feature Requests category")

    # rebuild
    sp_rebuild = subparsers.add_parser("rebuild",
        help="Re-filter cached topics and rebuild combined output (no API calls)")
    sp_rebuild.add_argument("--scope", required=True,
                            help="Scope identifier (must match a previous update run)")

    # extract-code
    sp_code = subparsers.add_parser("extract-code",
        help="Extract code fences from combined topic file (upvote > 0)")
    sp_code.add_argument("--scope", required=True,
                         help="Scope identifier (must match a previous update run)")

    # refresh-users
    sp_refresh = subparsers.add_parser("refresh-users",
        help="Update trusted posters from forum reputation")
    sp_refresh.add_argument("--min-reputation", type=int, default=100,
                           help="Minimum reputation threshold (default: 100)")

    # crawl-code
    sp_crawl = subparsers.add_parser("crawl-code",
        help="Crawl forum categories and extract qualifying code blocks for vector DB")
    sp_crawl.add_argument("--categories",
        help="Comma-separated category IDs to crawl (default: all non-excluded)")
    sp_crawl.add_argument("--min-lines", type=int, default=3,
        help="Minimum code block lines (default: 3)")
    sp_crawl.add_argument("--max-topics", type=int, default=2000,
        help="Safety cap on total topics to process (default: 2000)")
    sp_crawl.add_argument("--output", default=str(CACHE_DIR / "forum_code_dataset.json"),
        help="Output file path")
    sp_crawl.add_argument("--resume", action="store_true",
        help="Resume from last crawl position, skip already-processed topics")

    # describe-code
    sp_describe = subparsers.add_parser("describe-code",
        help="Export raw code blocks as JSONL for LLM triage")
    sp_describe.add_argument("--input", required=True,
        help="Input file from crawl-code")
    sp_describe.add_argument("--output",
        help="Output file path (default: <input>_described.json)")
    sp_describe.add_argument("--batch-size", type=int, default=0,
        help="Number of blocks per batch (0 = all at once)")
    sp_describe.add_argument("--offset", type=int, default=0,
        help="Start from this block index (for resuming)")

    # apply-descriptions
    sp_apply = subparsers.add_parser("apply-descriptions",
        help="Merge LLM-generated titles/descriptions back into code dataset")
    sp_apply.add_argument("--input", required=True,
        help="Original input file from crawl-code")
    sp_apply.add_argument("--responses", required=True,
        help="JSONL file with LLM responses")
    sp_apply.add_argument("--output",
        help="Output file path (default: <input>_final.json)")
    sp_apply.add_argument("--snippet", action="store_true",
        help="Output as snippet_batch_NNN.json instead of batch_NNN.json")

    # crawl-snippets
    sp_crawl_snip = subparsers.add_parser("crawl-snippets",
        help="Crawl forum categories and extract scripts from HiseSnippets")
    sp_crawl_snip.add_argument("--categories",
        help="Comma-separated category IDs to crawl (default: all non-excluded)")
    sp_crawl_snip.add_argument("--max-topics", type=int, default=2000,
        help="Safety cap on total topics to process (default: 2000)")
    sp_crawl_snip.add_argument("--output", default=str(CACHE_DIR / "snippet_dataset_raw.json"),
        help="Output file path")
    sp_crawl_snip.add_argument("--resume", action="store_true",
        help="Resume from last crawl position, skip already-processed topics")

    args = parser.parse_args()
    config = load_config()

    if args.command == "search":
        cmd_search(args, config)
    elif args.command == "fetch":
        cmd_fetch(args, config)
    elif args.command == "update":
        cmd_update(args, config)
    elif args.command == "rebuild":
        cmd_rebuild(args, config)
    elif args.command == "extract-code":
        cmd_extract_code(args, config)
    elif args.command == "refresh-users":
        cmd_refresh_users(args, config)
    elif args.command == "crawl-code":
        cmd_crawl_code(args, config)
    elif args.command == "describe-code":
        cmd_describe_code(args, config)
    elif args.command == "apply-descriptions":
        cmd_apply_descriptions(args, config)
    elif args.command == "crawl-snippets":
        cmd_crawl_snippets(args, config)


if __name__ == "__main__":
    main()
