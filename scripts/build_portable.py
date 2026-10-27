"""Small local Unix compiler route. CI also validates CMake/CTest on three OSes."""

import hashlib
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]
CORE = ROOT / "Source/FractalFPV/FractalCore/FractalCore.h"
TOOL = ROOT / "tools/exporter.cpp"


def fingerprints():
    core = hashlib.sha256(CORE.read_bytes()).hexdigest()
    tool = hashlib.sha256(TOOL.read_bytes()).hexdigest()
    return core, hashlib.sha256((core + tool).encode("ascii")).hexdigest()


def build():
    compiler = shutil.which("clang++") or shutil.which("g++")
    if not compiler:
        raise SystemExit(
            "Use a C++17 compiler, or build the supplied CMake project with your platform toolchain."
        )
    destination = ROOT / "build"
    destination.mkdir(exist_ok=True)
    core, producer = fingerprints()
    common = [
        compiler,
        "-std=c++17",
        "-O2",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-pedantic",
        "-I",
        str(CORE.parent),
    ]
    subprocess.run(
        common
        + [
            str(TOOL),
            f'-DFRACTAL_CORE_SHA256="{core}"',
            f'-DFRACTAL_PRODUCER_SHA256="{producer}"',
            "-o",
            str(destination / "fractal-export"),
        ],
        check=True,
    )
    subprocess.run(
        common
        + [str(ROOT / "tests/core_tests.cpp"), "-o", str(destination / "core-tests")],
        check=True,
    )
    subprocess.run([str(destination / "core-tests")], check=True)
    print("Built the portable tools from current source. Unreal was not invoked.")


if __name__ == "__main__":
    build()
