#!/usr/bin/env python3
"""Publish a YARR release to Zenodo using AUTHORS.yaml for metadata.

Usage:
    python3 scripts/zenodo_publish.py --tag v1.5.4

The Zenodo API token must be provided via --token or the ZENODO_TOKEN
environment variable.  Use --sandbox to test against sandbox.zenodo.org.
"""

import argparse
import os
import subprocess
import tempfile
from pathlib import Path

import requests
import yaml

ZENODO_API = "https://zenodo.org/api"
SANDBOX_API = "https://sandbox.zenodo.org/api"
CONCEPT_RECID = "15007378"
AUTHORS_FILE = Path(__file__).parent.parent.parent / "AUTHORS.yaml"


def load_yaml(path: Path) -> dict:
    with open(path) as f:
        return yaml.safe_load(f)


def session(token: str) -> requests.Session:
    s = requests.Session()
    s.headers["Authorization"] = f"Bearer {token}"
    return s


def latest_deposit_id(s: requests.Session, base: str, concept_id: str) -> int:
    r = s.get(
        f"{base}/deposit/depositions",
        params={"q": f"conceptrecid:{concept_id}", "sort": "mostrecent", "size": 1},
    )
    r.raise_for_status()
    results = r.json()
    if not results:
        raise RuntimeError(f"No depositions found for concept record {concept_id}")
    return results[0]["id"]


def new_draft(s: requests.Session, base: str, deposit_id: int) -> tuple[int, str]:
    """Create a new draft version. Returns (new_deposit_id, bucket_url)."""
    r = s.post(f"{base}/deposit/depositions/{deposit_id}/actions/newversion")
    r.raise_for_status()
    draft_url = r.json()["links"]["latest_draft"]
    dr = s.get(draft_url)
    dr.raise_for_status()
    draft = dr.json()
    return draft["id"], draft["links"]["bucket"]


def delete_files(s: requests.Session, base: str, deposit_id: int) -> None:
    r = s.get(f"{base}/deposit/depositions/{deposit_id}/files")
    r.raise_for_status()
    for f in r.json():
        s.delete(f"{base}/deposit/depositions/{deposit_id}/files/{f['id']}").raise_for_status()


def upload(s: requests.Session, bucket_url: str, path: Path) -> None:
    with open(path, "rb") as fh:
        r = s.put(f"{bucket_url}/{path.name}", data=fh)
    r.raise_for_status()
    print(f"  Uploaded {path.name} ({path.stat().st_size / 1e6:.1f} MB)")


def build_metadata(data: dict, tag: str) -> dict:
    meta = data["metadata"]
    creators = []
    for author in data["authors"]:
        entry = {"name": author["name"]}
        if author.get("affiliation"):
            entry["affiliation"] = author["affiliation"]
        if author.get("orcid"):
            entry["orcid"] = author["orcid"]
        creators.append(entry)

    return {
        "metadata": {
            "title": meta["title"],
            "upload_type": "software",
            "description": meta["description"].strip(),
            "creators": creators,
            "version": tag,
            "license": meta["license"],
            "keywords": meta.get("keywords", []),
            "communities": meta.get("communities", []),
            "related_identifiers": [
                {
                    "identifier": "https://gitlab.cern.ch/YARR/YARR",
                    "relation": "isSupplementTo",
                    "scheme": "url",
                },
            ],
        }
    }


def make_archive(repo_root: Path, tag: str, dest: Path) -> Path:
    archive = dest / f"YARR-{tag}.zip"
    subprocess.run(
        ["git", "archive", "--format=zip", f"--prefix=YARR-{tag}/",
         f"--output={archive}", tag],
        cwd=repo_root, check=True,
    )
    return archive


def main() -> None:
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument(
        "--token", default=os.environ.get("ZENODO_TOKEN"),
        help="Zenodo API token (or set ZENODO_TOKEN env var)",
    )
    parser.add_argument("--tag", required=True, help="Git tag to publish, e.g. v1.5.4")
    parser.add_argument(
        "--concept-id", default=CONCEPT_RECID,
        help="Zenodo concept record ID (default: %(default)s)",
    )
    parser.add_argument(
        "--authors-file", default=str(AUTHORS_FILE),
        help="Path to AUTHORS.yaml (default: %(default)s)",
    )
    parser.add_argument("--sandbox", action="store_true",
                        help="Use sandbox.zenodo.org instead of production")
    parser.add_argument("--dry-run", action="store_true",
                        help="Build archive and print metadata without uploading anything")
    args = parser.parse_args()

    if not args.dry_run and not args.token:
        parser.error("--token or ZENODO_TOKEN env var is required (or use --dry-run)")

    base = SANDBOX_API if args.sandbox else ZENODO_API
    repo_root = Path(__file__).parent.parent.parent

    import json

    print(f"Loading {args.authors_file} ...")
    data = load_yaml(Path(args.authors_file))
    print(f"  {len(data.get('authors', []))} authors")

    print(f"Creating archive for ref {args.tag} ...")
    with tempfile.TemporaryDirectory() as tmp:
        archive = make_archive(repo_root, args.tag, Path(tmp))
        print(f"  Archive: {archive.name} ({archive.stat().st_size / 1e6:.1f} MB)")

        if args.dry_run:
            print("\n-- Metadata (dry run, nothing uploaded) --")
            print(json.dumps(build_metadata(data, args.tag), indent=2))
            print("\nDry run complete.")
            return

        s = session(args.token)

        print(f"Finding latest deposit for concept {args.concept_id} ...")
        latest_id = latest_deposit_id(s, base, args.concept_id)
        print(f"  Latest deposit ID: {latest_id}")

        print("Creating new draft version ...")
        new_id, bucket_url = new_draft(s, base, latest_id)
        print(f"  New draft deposit ID: {new_id}")

        print("Removing existing files from draft ...")
        delete_files(s, base, new_id)
        print("Uploading archive ...")
        upload(s, bucket_url, archive)

    print("Updating metadata ...")
    r = s.put(f"{base}/deposit/depositions/{new_id}", json=build_metadata(data, args.tag))
    r.raise_for_status()

    print("Publishing ...")
    r = s.post(f"{base}/deposit/depositions/{new_id}/actions/publish")
    r.raise_for_status()
    result = r.json()

    print(f"\nDone!")
    print(f"  DOI: {result['doi']}")
    print(f"  URL: https://zenodo.org/records/{result['id']}")


if __name__ == "__main__":
    main()
