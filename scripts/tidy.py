#!/usr/bin/env python3
"""
tidy.py — run clang-tidy on all C/C++ files

Usage:
    python tidy.py

If clang-tidy is not found:

    On Windows:
        1. Go to https://github.com/llvm/llvm-project/releases
        2. Download the latest LLVM-x.y.z-win64.exe
        3. During installation, CHECK “Add LLVM to the system PATH”
        4. Reopen PowerShell and verify with: clang-tidy --version

    On macOS (Homebrew):
        brew install clang-tidy

    On Linux (Ubuntu/Debian):
        sudo apt install clang-tidy
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
    print("[INFO] Note: Missing header errors are expected for embedded projects without full SDK paths")

    # Get PlatformIO include paths
    pio_include = os.path.join(".pio", "libdeps", "pico", "**")

    try:
        cmd = [
            "clang-tidy",
            "--config-file=.clang-tidy",
            # Don't treat warnings as errors for embedded projects (missing headers are expected)
            # "--warnings-as-errors=*",
            "--extra-arg=-std=c11",
            "--extra-arg=-Iinclude",
            "--extra-arg=-Isrc",
            f"--extra-arg=-I{pio_include}",
            "--extra-arg=-DPICO_BOARD=pico",
        ] + files

        result = subprocess.run(cmd, capture_output=True, text=True)

        # Check for actual naming/style violations (not just missing headers)
        output = result.stderr + result.stdout

        # Look for readability-identifier-naming violations specifically
        has_naming_issues = "readability-identifier-naming" in output

        if has_naming_issues:
            # Print only lines related to naming issues
            for line in output.split('\n'):
                if any(x in line for x in ["readability-identifier-naming", "invalid case style"]):
                    print(line)
            print()
            print("[ERROR] Found naming convention violations.")
            print("[INFO] Fix the naming issues shown above.")
            sys.exit(1)
        else:
            print("[SUCCESS] No naming convention violations found.")
    except FileNotFoundError:
        print("[ERROR] clang-tidy not found. See installation instructions above.")
        sys.exit(1)


if __name__ == "__main__":
    main()
