import json
import math
from pathlib import Path
import unittest
from unittest.mock import patch
from scripts import reproduce


class ProjectionTests(unittest.TestCase):
    def test_equal_depth_face_order_is_independent_of_python_float_sum_version(self):
        mesh = json.loads(
            (Path(__file__).parents[1] / "web/assets/mesh-2.json").read_text()
        )

        def classic_sum(values):
            total = 0
            for value in values:
                total += value
            return total

        # Python 3.12 changed builtin sum(float). Emulate both runtime behaviours.
        with patch.object(reproduce, "sum", classic_sum, create=True):
            classic = reproduce.projection_svg(mesh)
        with patch.object(reproduce, "sum", math.fsum, create=True):
            accurate = reproduce.projection_svg(mesh)
        self.assertEqual(classic, accurate)


if __name__ == "__main__":
    unittest.main()
