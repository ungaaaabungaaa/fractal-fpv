"""Public bundle CLI. The tested C++ core owns all geometry and camera math."""

import argparse
import json
from pathlib import Path
import subprocess
import sys

if __package__ in (None, ""):
    sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from scripts.build_portable import ROOT, fingerprints


class ToolError(RuntimeError):
    pass


def tool_path():
    for name in [
        "fractal-export",
        "fractal-export.exe",
        "Release/fractal-export.exe",
        "Debug/fractal-export.exe",
    ]:
        candidate = ROOT / "build" / name
        if candidate.is_file():
            return candidate
    raise ToolError(
        "Build the portable exporter first; no Unreal installation is required."
    )


def native(tool, args):
    process = subprocess.run(
        [str(tool), *args], capture_output=True, timeout=30, cwd=ROOT
    )
    if process.returncode == 2:
        raise ValueError(process.stderr.decode("utf8", errors="replace")[:200].strip())
    if process.returncode != 0:
        raise ToolError("The portable exporter failed.")
    if len(process.stdout) > 16 * 1024 * 1024:
        raise ToolError("The portable exporter exceeded the 16 MiB output limit.")
    return process.stdout


def verified_tool():
    tool = tool_path()
    actual = json.loads(native(tool, ["version"]))
    core, producer = fingerprints()
    if actual != {"coreSha256": core, "producerSha256": producer}:
        raise ToolError(
            "The portable binary is stale. Rebuild it from the current source."
        )
    return tool


def write_bundle(destination, files):
    destination = Path(destination)
    destination.mkdir()
    owned = []
    try:
        for name, data in files.items():
            path = destination / name
            with path.open("xb") as stream:
                owned.append(path)
                stream.write(data)
    except Exception:
        for path in owned:
            path.unlink(missing_ok=True)
        try:
            destination.rmdir()
        except OSError:
            pass
        raise


def main(argv=None):
    parser = argparse.ArgumentParser(
        description="Portable fractal mesh and virtual camera exports; Unreal validation is separate."
    )
    commands = parser.add_subparsers(dest="command", required=True)
    mesh = commands.add_parser("mesh")
    mesh.add_argument("--depth", default="2")
    mesh.add_argument("--size-m", default="20")
    mesh.add_argument("--out", required=True)
    track = commands.add_parser("track")
    track.add_argument("--out", required=True)
    args = parser.parse_args(argv)
    try:
        tool = verified_tool()
        if args.command == "mesh":
            files = {
                f"mesh.{format}": native(
                    tool, ["mesh", args.depth, args.size_m, format]
                )
                for format in ["json", "obj"]
            }
            files["report.json"] = native(
                tool, ["mesh", args.depth, args.size_m, "report"]
            )
            report = json.loads(files["report.json"])
            write_bundle(args.out, files)
            print(
                f"Generated {report['retainedCells']} retained cells, {report['exposedFaces']} exposed faces, {report['triangles']} triangles. Geometry units: metres."
            )
        else:
            write_bundle(args.out, {"camera-track.json": native(tool, ["track"])})
            print(
                "Generated 721 actual core-camera samples at 60 Hz. Kinematic motion; collision and engine validation are absent."
            )
        return 0
    except ValueError as error:
        print(str(error), file=sys.stderr)
        return 2
    except (OSError, ToolError, subprocess.TimeoutExpired) as error:
        print(
            (
                str(error)
                if isinstance(error, ToolError)
                else "Export I/O failed; use a new destination whose parent exists."
            ),
            file=sys.stderr,
        )
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
