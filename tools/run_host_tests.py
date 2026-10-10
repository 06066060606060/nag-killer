#!/usr/bin/env python3
from pathlib import Path
import subprocess
import sys
import tempfile


ROOT = Path(__file__).resolve().parents[1]
TESTS = ROOT / "tests"


def run(command: list[str], cwd: Path = ROOT) -> None:
    print("+", " ".join(command), flush=True)
    subprocess.run(command, cwd=cwd, check=True)


def main() -> int:
    cpp_tests = sorted(TESTS.rglob("test_*.cpp"))
    with tempfile.TemporaryDirectory(prefix="nag-sc-host-") as temp:
        build_dir = Path(temp)
        for source in cpp_tests:
            binary = build_dir / source.stem
            run([
                "/usr/bin/g++",
                "-std=c++17",
                "-Wall",
                "-Wextra",
                "-Werror",
                "-I",
                str(ROOT),
                "-I",
                str(source.parent),
                "-I",
                str(TESTS / "host_generated_header"),
                str(source),
                "-o",
                str(binary),
            ])
            run([str(binary)])

    run([
        sys.executable,
        "-m",
        "unittest",
        "discover",
        "-s",
        "tests",
        "-p",
        "test_*.py",
        "-v",
    ])
    print(f"PASS: {len(cpp_tests)} C++ test(s) and Python unittest suite")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
