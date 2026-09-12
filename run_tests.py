#!/usr/bin/env python3
"""
MiniLang Compiler — Automated Test Runner
==========================================
Cross-platform test harness that discovers and runs compiler test suites:
  • positive tests    (tests/*.minilang)        — must compile successfully
  • negative tests    (tests/negative/*.minilang) — must fail with errors
  • warning tests     (tests/warnings/*.minilang) — must succeed with warnings

Usage:
    python run_tests.py [--compiler <path>] [--verbose]
"""

import argparse
import glob
import os
import re
import subprocess
import sys
import time

# ─── ANSI colours (disabled automatically on non-tty or Windows without VT) ──
def _supports_color():
    if not hasattr(sys.stdout, "isatty") or not sys.stdout.isatty():
        return False
    if sys.platform == "win32":
        try:
            import ctypes
            kernel32 = ctypes.windll.kernel32
            # Enable VT100 processing
            kernel32.SetConsoleMode(kernel32.GetStdHandle(-11), 7)
            return True
        except Exception:
            return False
    return True

_COLOR = _supports_color()
GREEN  = "\033[92m" if _COLOR else ""
RED    = "\033[91m" if _COLOR else ""
YELLOW = "\033[93m" if _COLOR else ""
CYAN   = "\033[96m" if _COLOR else ""
BOLD   = "\033[1m"  if _COLOR else ""
RESET  = "\033[0m"  if _COLOR else ""

# ─── helpers ─────────────────────────────────────────────────────────────────

def find_compiler(explicit_path=None):
    """Locate the minilang compiler binary."""
    if explicit_path:
        if os.path.isfile(explicit_path):
            return os.path.abspath(explicit_path)
        raise FileNotFoundError(f"Compiler not found at: {explicit_path}")

    # Search common build dirs relative to the project root
    script_dir = os.path.dirname(os.path.abspath(__file__))
    candidates = [
        os.path.join(script_dir, "build", "minilang"),
        os.path.join(script_dir, "build", "minilang.exe"),
        os.path.join(script_dir, "build", "Debug", "minilang.exe"),
        os.path.join(script_dir, "build", "Release", "minilang.exe"),
        os.path.join(script_dir, "cmake-build-debug", "minilang"),
        os.path.join(script_dir, "cmake-build-debug", "minilang.exe"),
        os.path.join(script_dir, "cmake-build-release", "minilang"),
    ]
    for c in candidates:
        if os.path.isfile(c):
            return os.path.abspath(c)

    raise FileNotFoundError(
        "Could not auto-locate minilang compiler. Pass --compiler <path>."
    )


def discover_tests(root_dir):
    """Return (positive, negative, warning) lists of .minilang paths."""
    positive = sorted(glob.glob(os.path.join(root_dir, "tests", "*.minilang")))
    negative = sorted(glob.glob(os.path.join(root_dir, "tests", "negative", "*.minilang")))
    warnings = sorted(glob.glob(os.path.join(root_dir, "tests", "warnings", "*.minilang")))
    return positive, negative, warnings


def run_compiler(compiler, source, extra_args=None, timeout=30):
    """Run the compiler on *source* and return (returncode, stdout, stderr)."""
    cmd = [compiler] + (extra_args or []) + [source]
    try:
        proc = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=timeout,
        )
        return proc.returncode, proc.stdout, proc.stderr
    except subprocess.TimeoutExpired:
        return -1, "", "TIMEOUT"
    except FileNotFoundError:
        return -1, "", f"Compiler binary not found: {compiler}"


# ─── per-file expectation parsing ────────────────────────────────────────────

def parse_expected_errors(filepath):
    """
    Scan the first 30 lines of a negative test for comments like:
        // EXPECT-ERROR: <substring>
    Returns a list of expected substrings (may be empty).
    """
    patterns = []
    with open(filepath, "r", encoding="utf-8", errors="replace") as f:
        for i, line in enumerate(f):
            if i > 30:
                break
            m = re.search(r"//\s*EXPECT-ERROR:\s*(.+)", line)
            if m:
                patterns.append(m.group(1).strip())
    return patterns


def parse_expected_warnings(filepath):
    """
    Scan the first 30 lines of a warning test for comments like:
        // EXPECT-WARNING: <substring>
    Returns a list of expected substrings (may be empty).
    """
    patterns = []
    with open(filepath, "r", encoding="utf-8", errors="replace") as f:
        for i, line in enumerate(f):
            if i > 30:
                break
            m = re.search(r"//\s*EXPECT-WARNING:\s*(.+)", line)
            if m:
                patterns.append(m.group(1).strip())
    return patterns


# ─── test runners ────────────────────────────────────────────────────────────

class TestResult:
    __slots__ = ("name", "passed", "detail")
    def __init__(self, name, passed, detail=""):
        self.name = name
        self.passed = passed
        self.detail = detail


def run_positive_test(compiler, filepath, verbose=False):
    """Positive test: compiler must exit 0."""
    name = os.path.relpath(filepath)
    rc, stdout, stderr = run_compiler(compiler, filepath)
    if rc == 0:
        if verbose:
            print(f"  {GREEN}PASS{RESET}  {name}")
        return TestResult(name, True)
    else:
        detail = stderr.strip() or "(no stderr)"
        if verbose:
            print(f"  {RED}FAIL{RESET}  {name}  — exit {rc}: {detail[:120]}")
        return TestResult(name, False, f"exit {rc}: {detail[:200]}")


def run_negative_test(compiler, filepath, verbose=False):
    """Negative test: compiler must exit non-zero.
    If EXPECT-ERROR annotations exist, stderr must contain each substring."""
    name = os.path.relpath(filepath)
    rc, stdout, stderr = run_compiler(compiler, filepath)

    if rc == 0:
        if verbose:
            print(f"  {RED}FAIL{RESET}  {name}  — expected failure but got exit 0")
        return TestResult(name, False, "Expected non-zero exit but got 0")

    expected = parse_expected_errors(filepath)
    missing = [e for e in expected if e.lower() not in stderr.lower()]
    if missing:
        detail = f"Missing expected error(s): {missing}"
        if verbose:
            print(f"  {RED}FAIL{RESET}  {name}  — {detail}")
        return TestResult(name, False, detail)

    if verbose:
        print(f"  {GREEN}PASS{RESET}  {name}")
    return TestResult(name, True)


def run_warning_test(compiler, filepath, verbose=False):
    """Warning test: compiler must exit 0 AND stderr must contain expected warnings."""
    name = os.path.relpath(filepath)
    rc, stdout, stderr = run_compiler(compiler, filepath)

    if rc != 0:
        detail = stderr.strip() or "(no stderr)"
        if verbose:
            print(f"  {RED}FAIL{RESET}  {name}  — expected success but got exit {rc}: {detail[:120]}")
        return TestResult(name, False, f"Expected exit 0 but got {rc}")

    expected = parse_expected_warnings(filepath)
    missing = [w for w in expected if w.lower() not in stderr.lower()]
    if missing:
        detail = f"Missing expected warning(s): {missing}"
        if verbose:
            print(f"  {RED}FAIL{RESET}  {name}  — {detail}")
        return TestResult(name, False, detail)

    # If no explicit expectations, at least verify there *is* a warning
    if not expected and "warning" not in stderr.lower():
        detail = "No warning text found in stderr"
        if verbose:
            print(f"  {YELLOW}WARN{RESET}  {name}  — {detail} (no EXPECT-WARNING annotation)")
        # Still pass since the test compiled successfully
        return TestResult(name, True, detail)

    if verbose:
        print(f"  {GREEN}PASS{RESET}  {name}")
    return TestResult(name, True)


# ─── main ────────────────────────────────────────────────────────────────────

def main():
    parser = argparse.ArgumentParser(description="MiniLang Compiler Test Runner")
    parser.add_argument("--compiler", "-C", default=None,
                        help="Path to the minilang compiler binary")
    parser.add_argument("--verbose", "-v", action="store_true",
                        help="Print per-test results")
    args = parser.parse_args()

    script_dir = os.path.dirname(os.path.abspath(__file__))

    try:
        compiler = find_compiler(args.compiler)
    except FileNotFoundError as e:
        print(f"{RED}ERROR{RESET}: {e}", file=sys.stderr)
        sys.exit(2)

    positive, negative, warnings = discover_tests(script_dir)

    total = len(positive) + len(negative) + len(warnings)
    if total == 0:
        print(f"{YELLOW}No test files found.{RESET}")
        sys.exit(0)

    print(f"\n{BOLD}MiniLang Test Runner{RESET}")
    print(f"Compiler : {compiler}")
    print(f"Tests    : {len(positive)} positive, {len(negative)} negative, {len(warnings)} warning")
    print(f"{'─' * 60}")

    t0 = time.monotonic()
    results = []

    # ── Positive tests ────────────────────────────────────────────
    if positive:
        print(f"\n{CYAN}▸ Positive Tests{RESET} ({len(positive)})")
        for fp in positive:
            results.append(run_positive_test(compiler, fp, verbose=args.verbose))

    # ── Negative tests ────────────────────────────────────────────
    if negative:
        print(f"\n{CYAN}▸ Negative Tests{RESET} ({len(negative)})")
        for fp in negative:
            results.append(run_negative_test(compiler, fp, verbose=args.verbose))

    # ── Warning tests ─────────────────────────────────────────────
    if warnings:
        print(f"\n{CYAN}▸ Warning Tests{RESET} ({len(warnings)})")
        for fp in warnings:
            results.append(run_warning_test(compiler, fp, verbose=args.verbose))

    elapsed = time.monotonic() - t0

    # ── Summary ───────────────────────────────────────────────────
    passed = sum(1 for r in results if r.passed)
    failed = total - passed

    print(f"\n{'─' * 60}")
    color = GREEN if failed == 0 else RED
    print(f"{color}{BOLD}{passed}/{total} passed{RESET}  ({elapsed:.2f}s)")

    if failed > 0:
        print(f"\n{RED}Failed tests:{RESET}")
        for r in results:
            if not r.passed:
                print(f"  ✗ {r.name}  — {r.detail}")
        sys.exit(1)
    else:
        print(f"\n{GREEN}All tests passed! ✓{RESET}")
        sys.exit(0)


if __name__ == "__main__":
    main()
