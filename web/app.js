import {
  validateMesh,
  validateTrack,
  interpolatePose,
  cameraTrackStatus,
  lookAt,
  perspective,
} from "./view-model.js";
const $ = (id) => document.getElementById(id),
  canvas = $("canvas"),
  rad = Math.PI / 180;
const text = (id, value) => {
  $(id).textContent = value;
};
let mesh = null,
  track = null,
  loadVersion = 0,
  mode = "orbit",
  wire = false,
  time = 0,
  playing = false,
  frame = 0,
  start = 0;
let orbit = { yaw: -50, pitch: 32, radius: 56 },
  gl = null,
  program = null,
  vao = null,
  ready = false,
  buffers = [],
  indexBuffer = null,
  edgeBuffer = null,
  edgeCount = 0;
function status(message, error = false) {
  text("status", message);
  $("status").classList.toggle("error", error);
}
function shader(type, source) {
  const object = gl.createShader(type);
  gl.shaderSource(object, source);
  gl.compileShader(object);
  if (!gl.getShaderParameter(object, gl.COMPILE_STATUS)) {
    const message = gl.getShaderInfoLog(object);
    gl.deleteShader(object);
    throw new Error(message);
  }
  return object;
}
try {
  gl = canvas.getContext("webgl2", { alpha: false, antialias: true });
  if (!gl) throw new Error("WebGL 2 unavailable");
  const vertex = shader(
    gl.VERTEX_SHADER,
    `#version 300 es
 layout(location=0) in vec3 position;layout(location=1) in vec3 normal;uniform mat4 view;uniform mat4 projection;out vec3 worldNormal;
 void main(){worldNormal=normal;gl_Position=projection*view*vec4(position,1.0);}`,
  );
  const fragment = shader(
    gl.FRAGMENT_SHADER,
    `#version 300 es
 precision highp float;in vec3 worldNormal;uniform bool wire;out vec4 colour;
 void main(){float light=.32+.68*max(dot(normalize(worldNormal),normalize(vec3(.6,-.8,1.2))),0.0);colour=vec4(wire?vec3(.13,.22,.15):vec3(.73,.80,.66)*light,1.0);}`,
  );
  program = gl.createProgram();
  gl.attachShader(program, vertex);
  gl.attachShader(program, fragment);
  gl.linkProgram(program);
  gl.deleteShader(vertex);
  gl.deleteShader(fragment);
  if (!gl.getProgramParameter(program, gl.LINK_STATUS))
    throw new Error(gl.getProgramInfoLog(program));
  vao = gl.createVertexArray();
  gl.enable(gl.DEPTH_TEST);
  gl.enable(gl.CULL_FACE);
  gl.cullFace(gl.BACK);
  // The left-handed view/projection preserves core X-forward/Y-right/Z-up poses.
  // Unchanged cross-positive core indices then have clockwise screen front faces.
  gl.frontFace(gl.CW);
  ready = true;
} catch (error) {
  console.error(error);
  status(
    "The 3D preview needs WebGL 2. Mesh data and project source remain available.",
    true,
  );
}
async function readJSON(path) {
  const response = await fetch(path, { cache: "no-store" });
  if (!response.ok) throw new Error("Exported data is unavailable.");
  const body = await response.arrayBuffer();
  if (body.byteLength > 1048576) throw new Error("Preview data exceeds 1 MiB.");
  return JSON.parse(new TextDecoder("utf-8", { fatal: true }).decode(body));
}
function upload(data) {
  if (!ready) return;
  gl.bindVertexArray(vao);
  for (const buffer of buffers) gl.deleteBuffer(buffer);
  buffers = [];
  function buffer(target, values) {
    const value = gl.createBuffer();
    if (!value) throw new Error("GPU buffer unavailable");
    buffers.push(value);
    gl.bindBuffer(target, value);
    gl.bufferData(target, values, gl.STATIC_DRAW);
    return value;
  }
  buffer(gl.ARRAY_BUFFER, new Float32Array(data.positions));
  gl.enableVertexAttribArray(0);
  gl.vertexAttribPointer(0, 3, gl.FLOAT, false, 0, 0);
  buffer(gl.ARRAY_BUFFER, new Float32Array(data.normals));
  gl.enableVertexAttribArray(1);
  gl.vertexAttribPointer(1, 3, gl.FLOAT, false, 0, 0);
  indexBuffer = buffer(gl.ELEMENT_ARRAY_BUFFER, new Uint32Array(data.indices));
  const edges = [];
  for (let base = 0; base < data.positions.length / 3; base += 4)
    edges.push(
      base,
      base + 1,
      base + 1,
      base + 2,
      base + 2,
      base + 3,
      base + 3,
      base,
    );
  edgeBuffer = buffer(gl.ELEMENT_ARRAY_BUFFER, new Uint32Array(edges));
  edgeCount = edges.length;
  gl.bindVertexArray(null);
}
function stop() {
  playing = false;
  if (frame) cancelAnimationFrame(frame);
  frame = 0;
  text("play", "Play camera track ▷");
}
function resetOrbit() {
  stop();
  mode = "orbit";
  time = 0;
  $("time").value = "0";
  orbit = { yaw: -50, pitch: 32, radius: (mesh?.report.sideM || 20) * 2.8 };
  draw();
  if (ready && mesh)
    status(
      `Orbit view of the actual level ${mesh.report.depth} export. Unreal validation remains pending.`,
    );
}
function pose() {
  if (mode === "track") {
    const p = interpolatePose(track, time),
      yaw = p.yawDeg * rad,
      pitch = p.pitchDeg * rad,
      forward = [
        Math.cos(pitch) * Math.cos(yaw),
        Math.cos(pitch) * Math.sin(yaw),
        Math.sin(pitch),
      ];
    return {
      eye: p.position,
      target: p.position.map((value, i) => value + forward[i]),
    };
  }
  const yaw = orbit.yaw * rad,
    pitch = orbit.pitch * rad;
  return {
    eye: [
      Math.cos(pitch) * Math.cos(yaw) * orbit.radius,
      Math.cos(pitch) * Math.sin(yaw) * orbit.radius,
      Math.sin(pitch) * orbit.radius,
    ],
    target: [0, 0, 0],
  };
}
function draw() {
  if (!mesh || !track) return;
  const camera = pose();
  text(
    "camera-mode",
    mode === "track"
      ? time >= track.durationSeconds
        ? "TRACK COMPLETE"
        : "RECORDED TRACK"
      : "ORBIT VIEW",
  );
  text(
    "pose",
    `X ${camera.eye[0].toFixed(2)} / Y ${camera.eye[1].toFixed(2)} / Z ${camera.eye[2].toFixed(2)} m`,
  );
  text(
    "time-label",
    `${time.toFixed(2)} / ${track.durationSeconds.toFixed(2)} s`,
  );
  if (!ready || !canvas.width || !canvas.height) return;
  gl.viewport(0, 0, canvas.width, canvas.height);
  gl.clearColor(241 / 255, 241 / 255, 233 / 255, 1);
  gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
  gl.useProgram(program);
  gl.bindVertexArray(vao);
  gl.uniformMatrix4fv(
    gl.getUniformLocation(program, "view"),
    false,
    lookAt(camera.eye, camera.target, [0, 0, 1]),
  );
  gl.uniformMatrix4fv(
    gl.getUniformLocation(program, "projection"),
    false,
    perspective(70, canvas.width / canvas.height, 0.05, 2000),
  );
  gl.uniform1i(gl.getUniformLocation(program, "wire"), wire ? 1 : 0);
  gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, wire ? edgeBuffer : indexBuffer);
  gl.drawElements(
    wire ? gl.LINES : gl.TRIANGLES,
    wire ? edgeCount : mesh.indices.length,
    gl.UNSIGNED_INT,
    0,
  );
  gl.bindVertexArray(null);
}
function resize() {
  const rect = canvas.getBoundingClientRect(),
    ratio = Math.min(window.devicePixelRatio || 1, 2);
  canvas.width = Math.max(1, Math.round(rect.width * ratio));
  canvas.height = Math.max(1, Math.round(rect.height * ratio));
  draw();
}
new ResizeObserver(resize).observe(canvas);
async function load(depth) {
  const version = ++loadVersion;
  stop();
  $("play").disabled = true;
  $("time").disabled = true;
  status(`Loading actual exported level ${depth}…`);
  try {
    const [data, recorded] = await Promise.all([
      readJSON(`assets/mesh-${depth}.json`),
      readJSON("assets/camera-track.json"),
    ]);
    validateMesh(data);
    validateTrack(recorded, data.report);
    if (version !== loadVersion) return;
    upload(data);
    mesh = data;
    track = recorded;
    const r = mesh.report;
    text("side", `${r.sideM.toFixed(1)} m`);
    text("cells", r.retainedCells.toLocaleString());
    text("triangles", r.triangles.toLocaleString());
    text(
      "area",
      `${r.surfaceAreaM2.toLocaleString(undefined, { maximumFractionDigits: 2 })} m²`,
    );
    text(
      "volume",
      `${r.voxelVolumeM3.toLocaleString(undefined, { maximumFractionDigits: 2 })} m³`,
    );
    text("reduction", `${r.triangleReductionPercent.toFixed(1)}%`);
    text(
      "baseline",
      `${r.triangles.toLocaleString()} / ${r.naiveTriangles.toLocaleString()} unculled baseline triangles`,
    );
    $("download-mesh").href = `assets/mesh-${depth}.json`;
    $("time").max = String(track.durationSeconds);
    text(
      "track-detail",
      `${track.samples.length} actual core poses at ${track.sampleRateHz} samples/s. This is not a measured render frame rate.`,
    );
    $("play").disabled = !ready;
    $("time").disabled = !ready;
    resetOrbit();
    resize();
    status(
      ready
        ? `Level ${depth} loaded from the recorded core export. Browser rendering is separate from pending Unreal validation.`
        : "The 3D preview needs WebGL 2. Validated mesh data and project source remain available.",
      !ready,
    );
  } catch (error) {
    console.error(error);
    if (version !== loadVersion) return;
    if (mesh) $("depth").value = String(mesh.report.depth);
    status(
      "Preview data could not be loaded or validated. Reload the local workbench after checking its exported files.",
      true,
    );
  }
}
function orbitBy(yaw, pitch) {
  stop();
  mode = "orbit";
  orbit.yaw += yaw;
  orbit.pitch = Math.max(-80, Math.min(80, orbit.pitch + pitch));
  draw();
  if (ready && mesh)
    status(
      `Orbit view of the actual level ${mesh.report.depth} export. Unreal validation remains pending.`,
    );
}
function zoom(factor) {
  if (!mesh) return;
  orbitBy(0, 0);
  orbit.radius = Math.max(
    mesh.report.sideM * 0.6,
    Math.min(mesh.report.sideM * 8, orbit.radius * factor),
  );
  draw();
}
$("depth").addEventListener("change", () => load(Number($("depth").value)));
document.querySelectorAll("input[name=style]").forEach((input) =>
  input.addEventListener("change", () => {
    wire = input.value === "wire";
    draw();
  }),
);
for (const [id, yaw, pitch] of [
  ["left", -12, 0],
  ["right", 12, 0],
  ["up", 0, 8],
  ["down", 0, -8],
])
  $(id).addEventListener("click", () => orbitBy(yaw, pitch));
$("zoom-in").addEventListener("click", () => zoom(0.85));
$("zoom-out").addEventListener("click", () => zoom(1.18));
$("reset").addEventListener("click", resetOrbit);
let drag = null;
canvas.addEventListener("pointerdown", (event) => {
  drag = [event.clientX, event.clientY];
  canvas.setPointerCapture(event.pointerId);
  canvas.classList.add("dragging");
});
canvas.addEventListener("pointermove", (event) => {
  if (!drag) return;
  orbitBy((event.clientX - drag[0]) * 0.3, (event.clientY - drag[1]) * 0.25);
  drag = [event.clientX, event.clientY];
});
for (const event of ["pointerup", "pointercancel"])
  canvas.addEventListener(event, () => {
    drag = null;
    canvas.classList.remove("dragging");
  });
canvas.addEventListener(
  "wheel",
  (event) => {
    event.preventDefault();
    zoom(Math.exp(Math.max(-100, Math.min(100, event.deltaY)) * 0.002));
  },
  { passive: false },
);
canvas.addEventListener("keydown", (event) => {
  const directions = {
    ArrowLeft: [-8, 0],
    ArrowRight: [8, 0],
    ArrowUp: [0, 6],
    ArrowDown: [0, -6],
  };
  if (directions[event.key]) {
    event.preventDefault();
    orbitBy(...directions[event.key]);
  }
});
function tick(now) {
  if (!playing) return;
  time = Math.min(track.durationSeconds, (now - start) / 1000);
  $("time").value = String(time);
  draw();
  if (time >= track.durationSeconds) {
    stop();
    status(cameraTrackStatus(time, track.durationSeconds, playing));
  } else frame = requestAnimationFrame(tick);
}
$("play").addEventListener("click", () => {
  if (playing) {
    stop();
    status(cameraTrackStatus(time, track.durationSeconds, playing));
    return;
  }
  if (!track || !ready) return;
  mode = "track";
  if (time >= track.durationSeconds) time = 0;
  playing = true;
  start = performance.now() - time * 1000;
  text("play", "Pause camera track Ⅱ");
  status(cameraTrackStatus(time, track.durationSeconds, playing));
  frame = requestAnimationFrame(tick);
});
$("time").addEventListener("input", () => {
  stop();
  if (!track) return;
  mode = "track";
  time = Number($("time").value);
  draw();
  status(cameraTrackStatus(time, track.durationSeconds, playing));
});
canvas.addEventListener("webglcontextlost", (event) => {
  event.preventDefault();
  stop();
  ready = false;
  $("play").disabled = true;
  status(
    "The graphics context was lost. Reload this page to restore the 3D preview.",
    true,
  );
});
resize();
await load(2);
