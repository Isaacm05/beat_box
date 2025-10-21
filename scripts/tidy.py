#!/usr/bin/env python3
"""
tidy.py — run clang-tidy on all C/C++ files

Usage:
    python3 tidy.py
"""

import subprocess
import os
import sys

EXTS = (".c", ".cpp", ".h", ".hpp")

def get_files(root="."):
    for dirpath, _, filenames in os.walk(root):
        if any(skip in dirpath for skip in [".git", ".pio", "build", "__pycache__"]):
            continue
        for f in filenames:
            if f.endswith(EXTS):
                yield os.path.join(dirpath, f)

def main():
    files = list(get_files())
    if not files:
        print("No source files found.")
        return

    print(f"Running clang-tidy on {len(files)} files...")

    try:
        # Adjust include paths and standard as needed
        cmd = [
            "clang-tidy",
            "--config-file=.clang-tidy",
            "--warnings-as-errors=*",
            "--quiet",
            "--extra-arg=-std=c11",
            "--extra-arg=-Iinclude",
        ] + files

        subprocess.run(cmd, check=True)
        print("✅ All files passed clang-tidy checks.")
    except FileNotFoundError:
        print("❌ clang-tidy not found. Install with `sudo apt install clang-tidy`.")
        sys.exit(1)
    except subprocess.CalledProcessError:
        print("❌ clang-tidy reported issues.")
        print("💡 Review the output above and fix the reported naming/style problems.")
        sys.exit(1)

if __name__ == "__main__":
    main()
