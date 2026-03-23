#!/usr/bin/env python3
import argparse
import json
import re
import urllib.parse
import urllib.request
from html import unescape
from pathlib import Path


DEFAULT_MANIFEST = Path("tracks_manifest.json")
DEFAULT_USER_AGENT = "speed-racer-rl/1.0 (lap-record-updater; contact: local)"
WIKI_API = "https://en.wikipedia.org/w/api.php"


TITLE_OVERRIDES = {
    "sao-paulo-gp": "Interlagos Circuit",
    "spanish-gp-madrid": "Madring",
    "mexico-city-gp": "Autódromo Hermanos Rodríguez",
}


def api_get(params, user_agent, timeout):
    url = WIKI_API + "?" + urllib.parse.urlencode(params)
    req = urllib.request.Request(url, headers={"User-Agent": user_agent})
    with urllib.request.urlopen(req, timeout=timeout) as resp:
        return json.loads(resp.read().decode("utf-8"))


def resolve_title(track):
    if track["slug"] in TITLE_OVERRIDES:
        return TITLE_OVERRIDES[track["slug"]]

    lap_source = (track.get("lap_record") or {}).get("source")
    if lap_source and "/wiki/" in lap_source:
        page = lap_source.split("/wiki/", 1)[1]
        return urllib.parse.unquote(page).replace("_", " ")

    return track.get("repo_name") or track["slug"]


def wikipedia_title_redirected(title, user_agent, timeout):
    data = api_get(
        {
            "action": "query",
            "titles": title,
            "redirects": 1,
            "format": "json",
            "formatversion": 2,
        },
        user_agent,
        timeout,
    )
    page = data["query"]["pages"][0]
    if "missing" in page:
        raise RuntimeError(f"Wikipedia page missing for title '{title}'")
    return page["title"]


def fetch_infobox_html(title, user_agent, timeout):
    data = api_get(
        {
            "action": "parse",
            "page": title,
            "prop": "text",
            "format": "json",
            "formatversion": 2,
        },
        user_agent,
        timeout,
    )
    return data["parse"]["text"]


def clean_html_text(raw):
    text = re.sub(r"<[^>]+>", " ", raw)
    text = unescape(text)
    text = text.replace("\xa0", " ")
    text = re.sub(r"\[[^\]]*]", " ", text)
    return re.sub(r"\s+", " ", text).strip()


def parse_lap_record(html):
    # Handles "Race lap record" and "Lap record" variants in infobox.
    match = re.search(
        r"<th[^>]*>\s*(?:Race\s+)?(?:Official\s+)?Lap record\s*</th>\s*<td[^>]*>(.*?)</td>",
        html,
        flags=re.I | re.S,
    )
    if not match:
        return None

    raw = clean_html_text(match.group(1))
    tm = re.search(r"(\d+:\d{2}\.\d{3}|\d+:\d{2}\.\d{2}|\d+:\d{2})", raw)
    yr = re.search(r"(19\d{2}|20\d{2})", raw)
    if not tm or not yr:
        return None

    time_val = tm.group(1)
    year_val = int(yr.group(1))

    driver = None
    parenthesized = re.search(r"\((.*?)\)", raw)
    if parenthesized:
        parts = [p.strip() for p in parenthesized.group(1).split(",") if p.strip()]
        if parts:
            # Most pages format as: Driver, Car, Year, Series.
            driver = parts[0]

    if not driver:
        start = raw.find(time_val) + len(time_val)
        end = raw.find(str(year_val))
        mid = raw[start : end if end != -1 else None]
        mid = re.sub(r"\([^)]*\)", " ", mid)
        mid = re.sub(r"\s+", " ", mid).strip(" ,;")
        driver = mid or None

    if not driver:
        return None

    return {"time": time_val, "driver": driver, "year": year_val}


def update_manifest(path, user_agent, timeout, dry_run):
    data = json.loads(path.read_text(encoding="utf-8"))

    filled = 0
    failures = []
    for track in data.get("tracks", []):
        slug = track["slug"]
        query_title = resolve_title(track)
        try:
            resolved = wikipedia_title_redirected(query_title, user_agent, timeout)
            html = fetch_infobox_html(resolved, user_agent, timeout)
            parsed = parse_lap_record(html)
            source = "https://en.wikipedia.org/wiki/" + urllib.parse.quote(
                resolved.replace(" ", "_")
            )

            if parsed:
                track["lap_record"] = {
                    "time": parsed["time"],
                    "driver": parsed["driver"],
                    "year": parsed["year"],
                    "source": source,
                }
                filled += 1
            else:
                track["lap_record"] = {
                    "time": None,
                    "driver": None,
                    "year": None,
                    "source": source,
                    "note": "Lap record not available or not parseable from source",
                }
                failures.append((slug, "no lap record row"))
        except Exception as exc:
            track["lap_record"] = {
                "time": None,
                "driver": None,
                "year": None,
                "source": None,
                "note": f"Lookup failed: {exc}",
            }
            failures.append((slug, str(exc)))

    if not dry_run:
        path.write_text(json.dumps(data, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")

    print(f"Updated: {path}")
    print(f"Lap records filled: {filled}/{len(data.get('tracks', []))}")
    if failures:
        print(f"Failures: {len(failures)}")
        for slug, msg in failures:
            print(f"- {slug}: {msg}")


def main():
    parser = argparse.ArgumentParser(
        description="Update normalized lap_record fields in tracks_manifest.json from Wikipedia infobox data."
    )
    parser.add_argument(
        "--manifest",
        type=Path,
        default=DEFAULT_MANIFEST,
        help="Path to tracks_manifest.json (default: tracks_manifest.json)",
    )
    parser.add_argument(
        "--timeout",
        type=int,
        default=30,
        help="HTTP timeout in seconds (default: 30)",
    )
    parser.add_argument(
        "--user-agent",
        default=DEFAULT_USER_AGENT,
        help="HTTP User-Agent header",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Resolve and parse records but do not write changes",
    )
    args = parser.parse_args()

    if not args.manifest.exists():
        raise SystemExit(f"Manifest not found: {args.manifest}")

    update_manifest(args.manifest, args.user_agent, args.timeout, args.dry_run)


if __name__ == "__main__":
    main()
