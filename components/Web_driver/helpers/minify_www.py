#!/usr/bin/env python3
"""
Minify web source files for the PTR firmware SPIFFS image.

Reads from:  components/Web_driver/www_uncompressed/
Writes to:   components/Web_driver/www/

Run from the project root before building, or whenever www_uncompressed files change.
Requires Python 3.6+. Installs rcssmin, rjsmin, htmlmin automatically on first run.
"""

import os
import re
import subprocess
import sys

SCRIPT_DIR     = os.path.dirname(os.path.abspath(__file__))
COMPONENT_DIR = os.path.dirname(SCRIPT_DIR)   # one level up → components/Web_driver/

SRC_DIR = os.path.join(COMPONENT_DIR, "www_uncompressed")
DST_DIR = os.path.join(COMPONENT_DIR, "www")

sys.stdin.close()   # detach stdin immediately — prevents any blocking read


# ---------------------------------------------------------------------------
# Minifier implementations (prefer installed packages, fall back to regex)
# ---------------------------------------------------------------------------

def _install_packages():
    pkgs = ["rcssmin", "rjsmin", "htmlmin"]
    try:
        subprocess.check_call(
            [sys.executable, "-m", "pip", "install", "--quiet"] + pkgs,
            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL
        )
        return True
    except Exception:
        return False


def minify_css(text):
    try:
        import rcssmin
        return rcssmin.cssmin(text)
    except ImportError:
        # Remove comments
        text = re.sub(r"/\*.*?\*/", "", text, flags=re.DOTALL)
        # Collapse whitespace
        text = re.sub(r"\s+", " ", text)
        # Remove spaces around structural characters
        text = re.sub(r"\s*([{}:;,>~+])\s*", r"\1", text)
        # Remove trailing semicolons before closing brace
        text = re.sub(r";}", "}", text)
        return text.strip()


def minify_js(text):
    try:
        import rjsmin
        return rjsmin.jsmin(text)
    except ImportError:
        # Safe-ish fallback: only strip block comments and blank lines
        # (single-line comment removal is skipped to avoid breaking URLs / regex)
        text = re.sub(r"/\*.*?\*/", "", text, flags=re.DOTALL)
        text = re.sub(r"\n[ \t]+", "\n", text)
        text = re.sub(r"\n{2,}", "\n", text)
        return text.strip()


def minify_html(text):
    try:
        import htmlmin
        return htmlmin.minify(
            text,
            remove_comments=True,
            remove_empty_space=True,
            remove_all_empty_space=False,
            reduce_boolean_attributes=True,
        )
    except ImportError:
        # Remove HTML comments
        text = re.sub(r"<!--.*?-->", "", text, flags=re.DOTALL)
        # Collapse runs of whitespace between tags
        text = re.sub(r">\s{2,}<", "> <", text)
        # Strip leading/trailing whitespace from lines
        text = "\n".join(line.strip() for line in text.splitlines())
        # Remove blank lines
        text = re.sub(r"\n{2,}", "\n", text)
        return text.strip()


# ---------------------------------------------------------------------------
# File processor
# ---------------------------------------------------------------------------

MINIFIERS = {
    ".css":  minify_css,
    ".js":   minify_js,
    ".html": minify_html,
}


def process(filename):
    src_path = os.path.join(SRC_DIR, filename)
    dst_path = os.path.join(DST_DIR, filename)

    with open(src_path, "r", encoding="utf-8") as f:
        original = f.read()

    ext = os.path.splitext(filename)[1].lower()
    minify_fn = MINIFIERS.get(ext)
    result = minify_fn(original) if minify_fn else original

    with open(dst_path, "w", encoding="utf-8", newline="") as f:
        f.write(result)

    orig_bytes = len(original.encode("utf-8"))
    mini_bytes = len(result.encode("utf-8"))
    pct = (1.0 - mini_bytes / orig_bytes) * 100.0 if orig_bytes else 0.0
    print(f"  {filename:<20s}  {orig_bytes:>6d} → {mini_bytes:>6d} B  ({pct:5.1f}% smaller)")
    return orig_bytes, mini_bytes


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main():
    if not os.path.isdir(SRC_DIR):
        print(f"ERROR: source directory not found:\n  {SRC_DIR}", file=sys.stderr)
        sys.exit(1)

    os.makedirs(DST_DIR, exist_ok=True)

    print("Installing/checking minifier packages...")
    ok = _install_packages()
    if not ok:
        print("  WARNING: package install failed — using built-in fallback minifiers.\n")
    else:
        print("  OK\n")

    print("Minifying:")
    total_orig = total_mini = 0
    for name in sorted(os.listdir(SRC_DIR)):
        src = os.path.join(SRC_DIR, name)
        if not os.path.isfile(src):
            continue
        o, m = process(name)
        total_orig += o
        total_mini += m

    if total_orig:
        saved = total_orig - total_mini
        pct   = (1.0 - total_mini / total_orig) * 100.0
        print(f"\n  Total: {total_orig} → {total_mini} B  ({pct:.1f}% smaller, saved {saved} B)")

    print(f"\nOutput written to:\n  {DST_DIR}")


if __name__ == "__main__":
    main()
