#!/usr/bin/env python3
"""
format.py — auto-format all C/C++ files using clang-format

Usage:
    python format.py

If clang-format is not found:
    On Windows:
        1. Go to https://github.com/llvm/llvm-project/releases
        2. Download and install the latest LLVM-x.y.z-win64.exe
        3. During installation, CHECK “Add LLVM to the system PATH”
        4. Reopen PowerShell and verify with: clang-format --version

    On macOS (Homebrew):
        brew install clang-format

    On Linux (Ubuntu/Debian):
        sudo apt install clang-format
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
        print("[SUCCESS] All files formatted successfully.")
    except FileNotFoundError:
        print("[ERROR] clang-format not found. See installation instructions above.")
        sys.exit(1)
    except subprocess.CalledProcessError:
        print("[ERROR] clang-format failed on some files.")
        sys.exit(1)


if __name__ == "__main__":
    main()
