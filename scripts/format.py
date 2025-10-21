#!/usr/bin/env python3
"""
format.py — auto-format all C/C++ files using clang-format

Usage:
    python3 format.py
"""

import subprocess
import os
import sys

# File extensions to format
EXTS = (".c", ".h", ".cpp", ".hpp")


def get_files(root="."):
    for dirpath, _, filenames in os.walk(root):
        # skip build, .pio, .git, and hidden dirs
        if any(x in dirpath for x in [".git", ".pio", "build", "__pycache__"]):
            continue
        for f in filenames:
            if f.endswith(EXTS):
                yield os.path.join(dirpath, f)


def main():
    files = list(get_files())
    if not files:
        print("No source files found.")
        return

    print(f"Formatting {len(files)} files...")
    try:
        subprocess.run(["clang-format", "-i"] + files, check=True)
        print("✅ All files formatted successfully.")
    except FileNotFoundError:
        print(
            "❌ clang-format not found. Please install it first (e.g. `sudo apt install clang-format`)."
        )
        sys.exit(1)
    except subprocess.CalledProcessError:
        print("❌ clang-format failed on some files.")
        sys.exit(1)


if __name__ == "__main__":
    main()
