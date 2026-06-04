#!/usr/bin/env python3
"""Check git contributors against AUTHORS.yaml and suggest potential additions.

Scans git history for the past N years (default: 2) and lists contributors
whose names do not match any entry in AUTHORS.yaml.  Results are ranked by
commit count and formatted as ready-to-paste YAML snippets for review.

This script is advisory only — it does not modify AUTHORS.yaml.

Usage:
    python3 scripts/check_authors.py
    python3 scripts/check_authors.py --years 1
    python3 scripts/check_authors.py --all-time
    python3 scripts/check_authors.py --min-commits 3

Exit code:
    0   all recent contributors are accounted for
    1   unaccounted contributors found (for CI visibility; treat as advisory)
"""

import argparse
import re
import subprocess
import sys
import unicodedata
from collections import defaultdict
from datetime import datetime, timedelta
from pathlib import Path
from typing import Optional

import yaml

AUTHORS_FILE = Path(__file__).parent.parent / "AUTHORS.yaml"


# ---------------------------------------------------------------------------
# YAML loading
# ---------------------------------------------------------------------------

def load_yaml(path: Path) -> dict:
    with open(path) as f:
        return yaml.safe_load(f)


# ---------------------------------------------------------------------------
# Git contributor extraction
# ---------------------------------------------------------------------------

def git_contributors(repo_root: Path, since: Optional[str]) -> dict[str, dict]:
    """Return {git_entry: {count, first, last}} for commits since `since`."""
    cmd = ["git", "log", "--format=%aN|%aE|%cd", "--date=short"]
    if since:
        cmd.append(f"--since={since}")
    result = subprocess.run(cmd, capture_output=True, text=True, cwd=repo_root, check=True)

    stats: dict[str, dict] = defaultdict(lambda: {"count": 0, "first": None, "last": None})
    for line in result.stdout.splitlines():
        line = line.strip()
        if not line:
            continue
        parts = line.split("|", 2)
        if len(parts) != 3:
            continue
        name, email, date = parts
        key = f"{name.strip()} <{email.strip()}>"
        s = stats[key]
        s["count"] += 1
        s["first"] = date if s["first"] is None else min(s["first"], date)
        s["last"] = date if s["last"] is None else max(s["last"], date)

    return dict(stats)


# ---------------------------------------------------------------------------
# Name normalisation and matching
# ---------------------------------------------------------------------------

def _tokens(text: str) -> set[str]:
    """Lowercase, accent-stripped tokens of ≥2 characters."""
    text = unicodedata.normalize("NFKD", text)
    text = "".join(c for c in text if unicodedata.category(c) != "Mn")
    return {t for t in re.split(r"[\s,.\-<>@_]+", text.lower()) if len(t) >= 2}


def matches_author(git_entry: str, author: dict) -> bool:
    display = git_entry.split("<")[0].strip()
    email = git_entry.split("<")[-1].rstrip(">").strip() if "<" in git_entry else ""
    git_parts = _tokens(display)
    email_user = email.split("@")[0]

    for candidate in [author["name"]] + list(author.get("git_names", [])):
        cand_display = candidate.split("<")[0].strip()
        cand_parts = _tokens(cand_display)
        # Two-token overlap covers first+last name.
        if len(git_parts & cand_parts) >= 2:
            return True
        # Single-token git identity (username).
        if len(git_parts) == 1 and git_parts & cand_parts:
            return True
        # Email-user matches a name token.
        if email_user and email_user in cand_parts:
            return True

    return False


def is_excluded(git_entry: str, patterns: list[str]) -> bool:
    lower = git_entry.lower()
    return any(p.lower() in lower for p in patterns)


# ---------------------------------------------------------------------------
# Formatting helpers
# ---------------------------------------------------------------------------

def guess_formal_name(display_name: str) -> str:
    """Guess 'Last, First' from 'First Last' style git name."""
    parts = display_name.split()
    if len(parts) >= 2:
        return f"{parts[-1]}, {' '.join(parts[:-1])}"
    return display_name


def yaml_snippet(display_name: str, git_entry: str) -> str:
    formal = guess_formal_name(display_name)
    lines = [
        f'  - name: "{formal}"',
        f'    affiliation: ""',
        f'    # orcid: "0000-0000-0000-0000"',
        f'    git_names:',
        f'      - "{display_name}"',
    ]
    return "\n".join(lines)


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main() -> None:
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument(
        "--years", type=float, default=2.0,
        help="Look back N years in git history (default: %(default)s)",
    )
    parser.add_argument(
        "--all-time", action="store_true",
        help="Check entire git history regardless of date",
    )
    parser.add_argument(
        "--min-commits", type=int, default=3,
        help="Minimum commit count to be reported (default: %(default)s)",
    )
    parser.add_argument(
        "--authors-file", default=str(AUTHORS_FILE),
        help="Path to AUTHORS.yaml (default: %(default)s)",
    )
    args = parser.parse_args()

    repo_root = Path(__file__).parent.parent
    data = load_yaml(Path(args.authors_file))
    authors = data.get("authors", [])
    excluded = data.get("excluded_git_names", [])

    if args.all_time:
        since = None
        period_label = "all time"
    else:
        cutoff = datetime.now() - timedelta(days=int(args.years * 365))
        since = cutoff.strftime("%Y-%m-%d")
        period_label = f"the last {args.years:.0f} year(s) (since {since})"

    print(f"AUTHORS.yaml: {len(authors)} authors")
    print(f"Checking git history for: {period_label}")
    if args.min_commits > 1:
        print(f"Minimum commits: {args.min_commits}")
    print()

    contributors = git_contributors(repo_root, since)

    # --- Contributors not in AUTHORS.yaml ---
    unmatched = []
    for entry, stats in contributors.items():
        if stats["count"] < args.min_commits:
            continue
        if is_excluded(entry, excluded):
            continue
        if any(matches_author(entry, a) for a in authors):
            continue
        unmatched.append((entry, stats))

    unmatched.sort(key=lambda x: x[1]["count"], reverse=True)

    if unmatched:
        print(f"Contributors NOT in AUTHORS.yaml ({len(unmatched)} found, >= {args.min_commits} commits):")
        print(f"{'Name / git identity':<50} {'Commits':>7}  {'First':>10}  {'Last':>10}")
        print("-" * 83)
        for entry, stats in unmatched:
            print(f"{entry:<50} {stats['count']:>7}  {stats['first']:>10}  {stats['last']:>10}")
        print()
        print("Suggested AUTHORS.yaml snippets (fill in affiliation and verify name order):")
        print()
        seen_displays: set[str] = set()
        for entry, stats in unmatched:
            display = entry.split("<")[0].strip()
            if display in seen_displays:
                continue
            seen_displays.add(display)
            print(yaml_snippet(display, entry))
            print()
    else:
        print(f"All contributors with >= {args.min_commits} commits are in AUTHORS.yaml.")
        print()

    # --- Authors with no commits in the selected period ---
    # Useful for identifying authors who no longer actively contribute.
    # Note: some authors support the project without committing code directly.
    inactive = []
    for author in authors:
        if not any(matches_author(entry, author) for entry in contributors):
            inactive.append(author["name"])

    if inactive:
        print(f"Authors in AUTHORS.yaml with NO commits in {period_label} ({len(inactive)} found):")
        print("(These may contribute non-code support; review before removing.)")
        for name in inactive:
            print(f"  {name}")
        print()
    else:
        print(f"All authors in AUTHORS.yaml have commits in {period_label}.")
        print()

    if unmatched or inactive:
        # Exit 1 so the CI job is visibly non-green, but allow_failure keeps it advisory.
        sys.exit(1)


if __name__ == "__main__":
    main()
