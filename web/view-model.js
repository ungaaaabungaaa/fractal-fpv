const finite = Number.isFinite;
const fail = (message) => {
  throw new Error(message);
};
export function validateMesh(data) {
  const report = data?.report,
    p = data?.positions,
    n = data?.normals,
    uv = data?.uv,
    indices = data?.indices;
  if (
    data?.schemaVersion !== 1 ||
    data.kind !== "menger-mesh" ||
    !report ||
    report.positionUnit !== "m" ||
    ![0, 1, 2].includes(report.depth) ||
    !finite(report.sideM) ||
    report.sideM < 1 ||
    report.sideM > 100
  )
    fail("Unsupported mesh metadata.");
  if (
    ![p, n, uv, indices].every(Array.isArray) ||
    p.length === 0 ||
    p.length > 300000 ||
    p.length % 12 ||
    n.length !== p.length ||
    uv.length !== (p.length / 3) * 2 ||
    indices.length % 3 ||
    indices.length > 300000
  )
    fail("Invalid mesh attributes or work limits.");
  if (
    report.vertices !== p.length / 3 ||
    report.triangles !== indices.length / 3 ||
    report.exposedFaces !== p.length / 12 ||
    indices.length !== report.exposedFaces * 6
  )
    fail("Mesh metrics disagree with geometry.");
  if (
    !Number.isInteger(report.retainedCells) ||
    report.retainedCells !== 20 ** report.depth ||
    report.naiveTriangles !== report.retainedCells * 12 ||
    report.triangles > report.naiveTriangles ||
    report.vertices > report.retainedCells * 24 ||
    !finite(report.triangleReductionPercent) ||
    Math.abs(
      report.triangleReductionPercent -
        (100 * (report.naiveTriangles - report.triangles)) /
          report.naiveTriangles,
    ) > 0.00001 ||
    ![report.surfaceAreaM2, report.voxelVolumeM3].every(
      (v) => finite(v) && v > 0,
    )
  )
    fail("Mesh comparison metrics are invalid.");
  if (
    !/^[a-f0-9]{64}$/.test(report.coreSha256) ||
    !/^[a-f0-9]{64}$/.test(report.producerSha256)
  )
    fail("Mesh provenance is missing.");
  if (
    p.some((v) => !finite(v) || Math.abs(v) > report.sideM / 2 + 0.000002) ||
    n.some((v) => !finite(v) || Math.abs(v) > 1) ||
    uv.some((v) => !finite(v) || v < 0 || v > 1) ||
    indices.some((v) => !Number.isInteger(v) || v < 0 || v >= p.length / 3)
  )
    fail("Mesh contains invalid coordinates or indices.");
  for (let i = 0; i < n.length; i += 3)
    if (Math.abs(Math.hypot(n[i], n[i + 1], n[i + 2]) - 1) > 0.00001)
      fail("Mesh normals are not unit vectors.");
  return data;
}
export function validateTrack(track, report) {
  if (
    track?.schemaVersion !== 1 ||
    track.kind !== "virtual-camera-track" ||
    track.positionUnit !== "m" ||
    track.angleUnit !== "deg" ||
    track.collision !== false ||
    track.coreSha256 !== report.coreSha256 ||
    track.producerSha256 !== report.producerSha256 ||
    !Array.isArray(track.samples) ||
    track.samples.length < 2 ||
    track.samples.length > 2000 ||
    !finite(track.durationSeconds) ||
    track.durationSeconds <= 0 ||
    track.durationSeconds > 60
  )
    fail("Unsupported camera track or provenance.");
  let last = -1;
  for (const s of track.samples) {
    if (
      !finite(s.t) ||
      s.t <= last ||
      s.t < 0 ||
      s.t > track.durationSeconds ||
      !Array.isArray(s.position) ||
      s.position.length !== 3 ||
      s.position.some((v) => !finite(v) || Math.abs(v) > 10000) ||
      !finite(s.yawDeg) ||
      Math.abs(s.yawDeg) > 180 ||
      !finite(s.pitchDeg) ||
      Math.abs(s.pitchDeg) > 89
    )
      fail("Invalid camera sample.");
    last = s.t;
  }
  if (track.samples[0].t !== 0 || last !== track.durationSeconds)
    fail("Camera track time range is incomplete.");
  return track;
}
export function interpolatePose(track, time) {
  if (!finite(time)) fail("Camera time must be finite.");
  const samples = track.samples,
    t = Math.max(0, Math.min(track.durationSeconds, time));
  let lo = 0,
    hi = samples.length - 1;
  while (lo + 1 < hi) {
    const mid = (lo + hi) >> 1;
    if (samples[mid].t <= t) lo = mid;
    else hi = mid;
  }
  const a = samples[lo],
    b = samples[hi],
    mix = (t - a.t) / (b.t - a.t),
    delta = ((b.yawDeg - a.yawDeg + 540) % 360) - 180;
  return {
    position: a.position.map((v, i) => v + (b.position[i] - v) * mix),
    yawDeg: a.yawDeg + delta * mix,
    pitchDeg: a.pitchDeg + (b.pitchDeg - a.pitchDeg) * mix,
  };
}
export const subtract = (a, b) => a.map((v, i) => v - b[i]);
export const dot = (a, b) => a.reduce((sum, v, i) => sum + v * b[i], 0);
export const cross = (a, b) => [
  a[1] * b[2] - a[2] * b[1],
  a[2] * b[0] - a[0] * b[2],
  a[0] * b[1] - a[1] * b[0],
];
export function normalise(v) {
  const length = Math.hypot(...v);
  if (!finite(length) || length < 1e-10) fail("Degenerate view direction.");
  return v.map((value) => value / length);
}
export function lookAt(eye, target, up) {
  if (
    ![eye, target, up].every(
      (v) => Array.isArray(v) && v.length === 3 && v.every(finite),
    )
  )
    fail("View vectors must be finite.");
  // Left-handed camera frame: x right, y up, z forward. World poses stay unchanged.
  const z = normalise(subtract(target, eye)),
    x = normalise(cross(up, z)),
    y = cross(z, x);
  return new Float32Array([
    x[0],
    y[0],
    z[0],
    0,
    x[1],
    y[1],
    z[1],
    0,
    x[2],
    y[2],
    z[2],
    0,
    -dot(x, eye),
    -dot(y, eye),
    -dot(z, eye),
    1,
  ]);
}
export function perspective(fov, aspect, near, far) {
  if (
    ![fov, aspect, near, far].every(finite) ||
    fov < 10 ||
    fov > 130 ||
    aspect <= 0 ||
    near <= 0 ||
    far <= near
  )
    fail("Invalid projection.");
  const f = 1 / Math.tan((fov * Math.PI) / 360),
    range = far - near;
  return new Float32Array([
    f / aspect,
    0,
    0,
    0,
    0,
    f,
    0,
    0,
    0,
    0,
    (far + near) / range,
    1,
    0,
    0,
    (-2 * far * near) / range,
    0,
  ]);
}
