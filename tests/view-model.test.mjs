import test from "node:test";
import assert from "node:assert/strict";
import { readFile } from "node:fs/promises";
import {
  validateMesh,
  validateTrack,
  interpolatePose,
  perspective,
  lookAt,
} from "../web/view-model.js";
const mesh = JSON.parse(
  await readFile(new URL("../web/assets/mesh-2.json", import.meta.url), "utf8"),
);
const track = JSON.parse(
  await readFile(
    new URL("../web/assets/camera-track.json", import.meta.url),
    "utf8",
  ),
);
test("actual mesh counts, bounds, attributes and indices are checked before GPU allocation", () => {
  assert.equal(validateMesh(mesh).report.triangles, 2112);
  for (const mutate of [
    (value) => (value.positions[0] = Infinity),
    (value) => value.normals.pop(),
    (value) => (value.indices[0] = -1),
    (value) => (value.report.positionUnit = "cm"),
    (value) => (value.report.vertices = 1),
  ]) {
    const bad = structuredClone(mesh);
    mutate(bad);
    assert.throws(() => validateMesh(bad));
  }
});
test("recorded track uses matching provenance and interpolation of real core poses", () => {
  assert.equal(validateTrack(track, mesh.report).samples.length, 721);
  const pose = interpolatePose(track, 6);
  assert.deepEqual(pose.position, [0, 0, 0]);
  assert.equal(pose.yawDeg, 0);
  assert.deepEqual(interpolatePose(track, -1).position, [-30, 0, 0]);
  assert.deepEqual(interpolatePose(track, 99).position, [30, 0, 0]);
  const bad = structuredClone(track);
  bad.samples[1].t = 0;
  assert.throws(() => validateTrack(bad, mesh.report));
  assert.throws(() =>
    validateTrack({ ...track, coreSha256: "bad" }, mesh.report),
  );
});
test("view/projection matrices use finite metres and a predictable camera frame", () => {
  const matrix = lookAt([0, -10, 0], [0, 0, 0], [0, 0, 1]);
  assert.equal(matrix.length, 16);
  assert.ok([...matrix].every(Number.isFinite));
  // The target lies ten metres in front of the view origin, at view-space z = +10.
  assert.equal(matrix[14], 10);
  assert.ok([...perspective(70, 1.5, 0.05, 500)].every(Number.isFinite));
  assert.throws(() => perspective(0, 1, 0.05, 500));
  assert.throws(() => lookAt([0, 0, 0], [0, 0, 0], [0, 0, 1]));
});
test("the left-handed browser projection uses clockwise screen front faces for unchanged core indices", async () => {
  const cube = JSON.parse(
    await readFile(
      new URL("../web/assets/mesh-0.json", import.meta.url),
      "utf8",
    ),
  );
  const view = lookAt([30, 0, 0], [0, 0, 0], [0, 0, 1]),
    projection = perspective(70, 1, 0.05, 500);
  const multiply = (matrix, p) =>
    [0, 1, 2, 3].map((row) =>
      [0, 1, 2, 3].reduce(
        (sum, column) => sum + matrix[column * 4 + row] * p[column],
        0,
      ),
    );
  const points = cube.indices.slice(0, 3).map((index) => {
    const p = [...cube.positions.slice(index * 3, index * 3 + 3), 1],
      clip = multiply(projection, multiply(view, p));
    return [clip[0] / clip[3], clip[1] / clip[3]];
  });
  const [a, b, c] = points,
    area = (b[0] - a[0]) * (c[1] - a[1]) - (b[1] - a[1]) * (c[0] - a[0]);
  assert.ok(area < 0, "outside +X face must have clockwise screen area");
});
