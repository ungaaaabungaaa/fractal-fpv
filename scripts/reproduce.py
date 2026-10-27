"""Actual portable exports and an orthographic projection; never an Unreal render."""

from contextlib import redirect_stdout
import hashlib
import io
import json
import math
from pathlib import Path
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from scripts.fractal import main, native, verified_tool


def projection_svg(mesh):
    dot = lambda a, b: math.fsum(x * y for x, y in zip(a, b))
    cross = lambda a, b: [
        a[1] * b[2] - a[2] * b[1],
        a[2] * b[0] - a[0] * b[2],
        a[0] * b[1] - a[1] * b[0],
    ]

    def unit(v):
        length = math.sqrt(dot(v, v))
        return [x / length for x in v]

    eye = unit([1.6, -2.3, 1.5])
    right = unit(cross([0, 0, 1], eye))
    up = cross(eye, right)
    faces = []
    for start in range(0, len(mesh["positions"]), 12):
        normal = mesh["normals"][start : start + 3]
        if dot(normal, eye) <= 0:
            continue
        points = [mesh["positions"][start + i : start + i + 3] for i in range(0, 12, 3)]
        centre = [math.fsum(p[axis] for p in points) / 4 for axis in range(3)]
        projected = " ".join(
            f"{500+dot(p,right)*24:.3f},{342-dot(p,up)*24:.3f}" for p in points
        )
        shade = round(225 + 22 * max(0, dot(normal, unit([0.6, -0.8, 1.2]))))
        faces.append(
            (
                # Tie faces below drawing precision in their stable mesh order.
                round(dot(centre, eye), 9),
                f'<polygon points="{projected}" fill="rgb({shade},{shade},{shade})" stroke="#27382e" stroke-width="0.65" stroke-linejoin="round"/>',
            )
        )
    drawing = "".join(face for _, face in sorted(faces, key=lambda item: item[0]))
    report = mesh["report"]
    return f"""<svg xmlns="http://www.w3.org/2000/svg" width="1000" height="760" viewBox="0 0 1000 760" role="img" aria-labelledby="title desc"><title id="title">Actual portable depth-two mesh projection</title><desc id="desc">Orthographic projection of exported mesh faces, not an Unreal render. Core source {report['coreSha256']}.</desc><rect width="1000" height="760" fill="#f4f3ec"/>{drawing}<text x="40" y="707" font-family="sans-serif" font-size="21" fill="#27382e">Depth 2 · {report['retainedCells']} retained cells · {report['triangles']} exposed triangles</text><text x="40" y="738" font-family="sans-serif" font-size="14" fill="#566753">Portable mesh projection · units: metres · Unreal compilation and rendering unverified</text></svg>\n""".encode()


def run():
    artifacts = {}
    reports = []
    with tempfile.TemporaryDirectory(prefix="fractal-reproduction-") as temporary:
        temporary = Path(temporary)
        for depth in range(3):
            destination = temporary / f"depth-{depth}"
            with redirect_stdout(io.StringIO()):
                assert (
                    main(
                        [
                            "mesh",
                            "--depth",
                            str(depth),
                            "--size-m",
                            "20",
                            "--out",
                            str(destination),
                        ]
                    )
                    == 0
                )
            artifacts[f"web/assets/mesh-{depth}.json"] = (
                destination / "mesh.json"
            ).read_bytes()
            reports.append(json.loads((destination / "report.json").read_text()))
            if depth == 1:
                artifacts["docs/results/depth-1.obj"] = (
                    destination / "mesh.obj"
                ).read_bytes()
        reports.append(
            json.loads(native(verified_tool(), ["mesh", "3", "20", "report"]))
        )
        with redirect_stdout(io.StringIO()):
            assert main(["track", "--out", str(temporary / "track")]) == 0
        artifacts["web/assets/camera-track.json"] = (
            temporary / "track/camera-track.json"
        ).read_bytes()
    artifacts["docs/results/metrics.json"] = (
        json.dumps(reports, indent=2) + "\n"
    ).encode()
    artifacts["docs/results/projection.svg"] = projection_svg(
        json.loads(artifacts["web/assets/mesh-2.json"])
    )
    track = json.loads(artifacts["web/assets/camera-track.json"])
    samples = track["samples"]
    distance = sum(
        math.dist(a["position"], b["position"]) for a, b in zip(samples, samples[1:])
    )
    artifacts["docs/results/camera-summary.json"] = (
        json.dumps(
            {
                "kind": "core-camera-export-summary",
                "sampleCount": len(samples),
                "sampleRateHz": track["sampleRateHz"],
                "durationSeconds": track["durationSeconds"],
                "pathDistanceM": round(distance, 6),
                "start": samples[0]["position"],
                "end": samples[-1]["position"],
                "collision": False,
                "engineValidation": "pending",
            },
            indent=2,
        )
        + "\n"
    ).encode()
    sources = [
        "Source/FractalFPV/FractalCore/FractalCore.h",
        "tools/exporter.cpp",
        "scripts/build_portable.py",
        "scripts/fractal.py",
        "scripts/reproduce.py",
    ]
    sha = lambda data: hashlib.sha256(data).hexdigest()
    manifest = {
        "kind": "actual-portable-core-exports",
        "date": "2026-10-27",
        "source_files_sha256": {
            name: sha((ROOT / name).read_bytes()) for name in sources
        },
        "artifacts_sha256": {
            name: sha(data) for name, data in sorted(artifacts.items())
        },
        "unverified": [
            "Unreal compilation",
            "editor startup",
            "engine rendering and input",
            "game packaging",
            "engine frame profiling",
        ],
        "note": "Browser/portable outputs are distinct from Unreal acceptance. No engine was installed or agreement accepted.",
    }
    artifacts["docs/results/manifest.json"] = (
        json.dumps(manifest, indent=2) + "\n"
    ).encode()
    if sys.argv[1:] not in ([], ["--write"]):
        raise ValueError(
            "Use no arguments to verify, or --write to refresh reviewed outputs."
        )
    write = sys.argv[1:] == ["--write"]
    for name, data in artifacts.items():
        target = ROOT / name
        if write:
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(data)
        else:
            assert target.read_bytes() == data, f"Reproduction differs: {name}"
    print(
        f'{"Wrote" if write else "Reproduced byte-for-byte"} {len(artifacts)} actual portable artifacts. No Unreal renderer was invoked.'
    )


if __name__ == "__main__":
    run()
