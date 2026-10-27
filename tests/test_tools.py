from contextlib import redirect_stdout, redirect_stderr
import hashlib
import io
import json
from pathlib import Path
import tempfile
import unittest
from scripts.fractal import main


class ToolTests(unittest.TestCase):
    def invoke(self, *args):
        stdout, stderr = io.StringIO(), io.StringIO()
        with redirect_stdout(stdout), redirect_stderr(stderr):
            try:
                code = main(list(args))
            except SystemExit as error:
                code = error.code
        return code, stdout.getvalue(), stderr.getvalue()

    def test_mesh_bundle_has_real_counts_units_attributes_and_source_fingerprint(self):
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary) / "mesh"
            self.assertEqual(
                self.invoke(
                    "mesh", "--depth", "1", "--size-m", "12", "--out", str(output)
                )[0],
                0,
            )
            report = json.loads((output / "report.json").read_text())
            mesh = json.loads((output / "mesh.json").read_text())
            self.assertEqual(
                (report["retainedCells"], report["exposedFaces"], report["triangles"]),
                (20, 72, 144),
            )
            self.assertEqual(report["naiveTriangles"], 240)
            self.assertEqual(report["triangleReductionPercent"], 40)
            self.assertEqual(report["positionUnit"], "m")
            self.assertEqual(len(mesh["positions"]), 72 * 4 * 3)
            self.assertEqual(len(mesh["normals"]), len(mesh["positions"]))
            self.assertEqual(len(mesh["indices"]), 144 * 3)
            self.assertTrue(all(-6 <= value <= 6 for value in mesh["positions"]))
            source = (
                Path(__file__).parents[1]
                / "Source/FractalFPV/FractalCore/FractalCore.h"
            )
            self.assertEqual(
                report["coreSha256"], hashlib.sha256(source.read_bytes()).hexdigest()
            )
            original = (output / "mesh.json").read_bytes()
            self.assertEqual(
                self.invoke(
                    "mesh", "--depth", "1", "--size-m", "12", "--out", str(output)
                )[0],
                1,
            )
            self.assertEqual((output / "mesh.json").read_bytes(), original)
            self.assertIn(b"\nf ", (output / "mesh.obj").read_bytes())

    def test_recorded_track_is_actual_time_scaled_core_output_without_collision(self):
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary) / "track"
            self.assertEqual(self.invoke("track", "--out", str(output))[0], 0)
            track = json.loads((output / "camera-track.json").read_text())
            self.assertFalse(track["collision"])
            self.assertEqual(track["sampleRateHz"], 60)
            self.assertEqual(len(track["samples"]), 721)
            self.assertEqual(track["samples"][0]["position"], [-30, 0, 0])
            self.assertEqual(track["samples"][-1]["position"], [30, 0, 0])
            for sample in track["samples"]:
                self.assertAlmostEqual(
                    sample["position"][0], -30 + 5 * sample["t"], delta=3e-6
                )
                self.assertEqual(sample["position"][1:], [0, 0])

    def test_invalid_or_excessive_requests_do_not_create_an_output_directory(self):
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary) / "bad"
            for arguments in [
                ("4", "20"),
                ("-1", "20"),
                ("1", "nan"),
                ("1", "0"),
                ("1", "101"),
                ("1x", "20"),
            ]:
                self.assertEqual(
                    self.invoke(
                        "mesh",
                        "--depth",
                        arguments[0],
                        "--size-m",
                        arguments[1],
                        "--out",
                        str(output),
                    )[0],
                    2,
                )
                self.assertFalse(output.exists())


if __name__ == "__main__":
    unittest.main()
