import re
import subprocess
import sys
import os


def get_latest_version(url):
    try:
        cmd = ["git", "ls-remote", "-t", "--refs", url]
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=15)
        if result.returncode != 0:
            return "Error"

        lines = result.stdout.strip().split("\n")
        tags = [line.split()[1].replace("refs/tags/", "") for line in lines if len(line.split()) >= 2]

        if not tags:
            return "No tags"

        # Filter out unstable
        stable_tags = [t for t in tags if not re.search(r"(rc|beta|alpha|dev|test|pre)", t, re.IGNORECASE)]
        if not stable_tags:
            stable_tags = tags

        def parse_to_tuple(tag):
            # Normalize: replace _ with .
            norm = tag.replace("_", ".")
            # Extract all digit sequences
            parts = re.findall(r"\d+", norm)
            return [int(p) for p in parts] if parts else [0]

        # Heuristic: Prefer tags that look like multi-part versions (contain dots or underscores)
        # over tags that are just a single number (often dates or old identifiers)
        semver_ish = [t for t in stable_tags if "." in t or "_" in t]

        # Exception: if it's just 'v1' or 'n2', we might want it if there's nothing better
        if not semver_ish:
            # Look for tags starting with v/n/R followed by a number
            semver_ish = [t for t in stable_tags if re.match(r"^[vVnR]\d+", t)]

        candidates = semver_ish if semver_ish else stable_tags

        # If it's FFmpeg, prefer tags starting with 'n'
        if "FFmpeg" in url:
            n_tags = [t for t in candidates if t.startswith("n")]
            if n_tags:
                candidates = n_tags

        candidates.sort(key=parse_to_tuple)

        if candidates:
            return candidates[-1]
        return "Unknown"

    except Exception:
        return "Error"


def main():
    filepath = "cmake/defaults/CYCOMMON.cmake"
    if not os.path.exists(filepath):
        print(f"File not found: {filepath}")
        sys.exit(1)

    with open(filepath, "r") as f:
        content = f.read()

    pkg_pattern = re.compile(r"^#\s+([a-zA-Z0-9_-]+)\s+(https?://\S+)", re.MULTILINE)
    matches = pkg_pattern.findall(content)

    GREEN = "\033[92m"
    RESET = "\033[0m"

    results = []

    print(f"Analyzing {len(matches)} packages...")

    for name, url in matches:
        var_base = name.upper().replace("-", "_")
        var_name = f"RV_DEPS_{var_base}_VERSION"

        var_regex = re.compile(rf'SET\(\s*{re.escape(var_name)}\s+"([^"]+)"', re.MULTILINE | re.DOTALL)
        versions_in_file = [v.strip() for v in var_regex.findall(content)]
        current_display = ", ".join(versions_in_file) if versions_in_file else "Not Found"

        latest_raw = get_latest_version(url)
        display_latest = latest_raw

        if latest_raw not in ["Error", "No tags", "Unknown"]:
            # Strip prefixes for display/comparison
            if display_latest.startswith(("v", "V")):
                display_latest = display_latest[1:]
            elif display_latest.startswith("R_"):
                display_latest = display_latest[2:]
            # ffmpeg n is kept

            display_latest = display_latest.replace("_", ".")

        is_match = display_latest in versions_in_file
        results.append({"name": name, "current": current_display, "latest": display_latest, "match": is_match})

    col_name = 15
    col_curr = 25
    col_late = 25

    header = f"{'Package':<{col_name}} | {'Current Version':<{col_curr}} | {'Latest Tag':<{col_late}}"
    print("\n" + header)
    print("-" * len(header))

    for r in results:
        name = r["name"]
        curr = r["current"]
        late = r["latest"]

        if r["match"]:
            late_colored = f"{GREEN}{late}{RESET}"
            padding = " " * (col_late - len(late))
            late_display = f"{late_colored}{padding}"
        else:
            late_display = f"{late:<{col_late}}"

        print(f"{name:<{col_name}} | {curr:<{col_curr}} | {late_display}")


if __name__ == "__main__":
    main()
