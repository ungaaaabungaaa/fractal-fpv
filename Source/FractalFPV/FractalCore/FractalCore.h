#pragma once

// Original portable geometry/camera core. Metres, degrees and seconds.
// No Unreal dependency; invalid caller input returns an error without mutation.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace fractal
{
struct Vec3
{
    double x = 0, y = 0, z = 0;
    bool operator==(const Vec3 &b) const { return x == b.x && y == b.y && z == b.z; }
    Vec3 operator+(const Vec3 &b) const { return {x + b.x, y + b.y, z + b.z}; }
    Vec3 operator-(const Vec3 &b) const { return {x - b.x, y - b.y, z - b.z}; }
    Vec3 operator*(double s) const { return {x * s, y * s, z * s}; }
};
inline double dot(const Vec3 &a, const Vec3 &b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline Vec3 cross(const Vec3 &a, const Vec3 &b)
{
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
inline double length(const Vec3 &v) { return std::sqrt(dot(v, v)); }
inline Vec3 toCentimetres(const Vec3 &v) { return v * 100; }
struct UV
{
    double u = 0, v = 0;
};
struct Mesh
{
    int depth = 0, retainedCells = 0, exposedFaces = 0;
    double sideM = 0, cellSideM = 0, surfaceAreaM2 = 0, volumeM3 = 0;
    std::vector<Vec3> vertices, normals;
    std::vector<UV> uv;
    std::vector<std::uint32_t> indices;
};
struct MeshResult
{
    Mesh mesh;
    std::string error;
    bool ok() const { return error.empty(); }
};
inline bool survives(int x, int y, int z, int depth)
{
    for (int level = 0; level < depth; ++level)
    {
        if ((x % 3 == 1) + (y % 3 == 1) + (z % 3 == 1) >= 2)
            return false;
        x /= 3;
        y /= 3;
        z /= 3;
    }
    return true;
}
inline MeshResult generate(int depth, double sideM)
{
    if (depth < 0 || depth > 3 || !std::isfinite(sideM) || sideM < 1 || sideM > 100)
        return {{}, "Depth must be 0-3 and side length must be finite, from 1 to 100 metres."};
    int n = 1, maxCells = 1;
    for (int level = 0; level < depth; ++level)
    {
        n *= 3;
        maxCells *= 20;
    }
    const auto index = [n](int x, int y, int z)
    { return static_cast<std::size_t>(x + n * (y + n * z)); };
    std::vector<std::uint8_t> occupied(static_cast<std::size_t>(n * n * n), 0);
    for (int z = 0; z < n; ++z)
        for (int y = 0; y < n; ++y)
            for (int x = 0; x < n; ++x)
                occupied[index(x, y, z)] = static_cast<std::uint8_t>(survives(x, y, z, depth));
    const auto solid = [&](int x, int y, int z)
    {
        return x >= 0 && y >= 0 && z >= 0 && x < n && y < n && z < n &&
               occupied[index(x, y, z)] != 0;
    };
    struct Face
    {
        int nx, ny, nz;
        std::array<std::array<int, 3>, 4> corners;
    };
    const std::array<Face, 6> faces = {
        {{1, 0, 0, {{{1, 0, 0}, {1, 1, 0}, {1, 1, 1}, {1, 0, 1}}}},
         {-1, 0, 0, {{{0, 0, 0}, {0, 0, 1}, {0, 1, 1}, {0, 1, 0}}}},
         {0, 1, 0, {{{0, 1, 0}, {0, 1, 1}, {1, 1, 1}, {1, 1, 0}}}},
         {0, -1, 0, {{{0, 0, 0}, {1, 0, 0}, {1, 0, 1}, {0, 0, 1}}}},
         {0, 0, 1, {{{0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}}}},
         {0, 0, -1, {{{0, 0, 0}, {0, 1, 0}, {1, 1, 0}, {1, 0, 0}}}}}};
    Mesh mesh;
    mesh.depth = depth;
    mesh.sideM = sideM;
    mesh.cellSideM = sideM / n;
    const auto capacity = static_cast<std::size_t>(maxCells * 24);
    mesh.vertices.reserve(capacity);
    mesh.normals.reserve(capacity);
    mesh.uv.reserve(capacity);
    mesh.indices.reserve(static_cast<std::size_t>(maxCells * 36));
    const std::array<UV, 4> uv = {{{0, 0}, {1, 0}, {1, 1}, {0, 1}}};
    for (int z = 0; z < n; ++z)
        for (int y = 0; y < n; ++y)
            for (int x = 0; x < n; ++x)
            {
                if (!solid(x, y, z))
                    continue;
                ++mesh.retainedCells;
                for (const auto &face : faces)
                {
                    if (solid(x + face.nx, y + face.ny, z + face.nz))
                        continue;
                    const auto base = static_cast<std::uint32_t>(mesh.vertices.size());
                    for (std::size_t corner = 0; corner < 4; ++corner)
                    {
                        const auto &c = face.corners[corner];
                        mesh.vertices.push_back({-sideM / 2 + (x + c[0]) * mesh.cellSideM,
                                                 -sideM / 2 + (y + c[1]) * mesh.cellSideM,
                                                 -sideM / 2 + (z + c[2]) * mesh.cellSideM});
                        mesh.normals.push_back({static_cast<double>(face.nx),
                                                static_cast<double>(face.ny),
                                                static_cast<double>(face.nz)});
                        mesh.uv.push_back(uv[corner]);
                    }
                    for (const auto offset : {0U, 1U, 2U, 0U, 2U, 3U})
                        mesh.indices.push_back(base + offset);
                    ++mesh.exposedFaces;
                }
            }
    mesh.surfaceAreaM2 = mesh.exposedFaces * mesh.cellSideM * mesh.cellSideM;
    mesh.volumeM3 = mesh.retainedCells * mesh.cellSideM * mesh.cellSideM * mesh.cellSideM;
    return {std::move(mesh), {}};
}
inline std::vector<std::uint32_t> indexOrder(const Mesh &mesh, bool reverse)
{
    auto indices = mesh.indices;
    if (reverse)
        for (std::size_t t = 0; t < indices.size(); t += 3)
            std::swap(indices[t + 1], indices[t + 2]);
    return indices;
}

struct Camera
{
    Vec3 position{35, -35, 20};
    double yawDeg = 135, pitchDeg = -22, speedMps = 5;
};
struct Controls
{
    double forward = 0, right = 0, up = 0, yaw = 0, pitch = 0;
};
inline bool validCamera(const Camera &camera)
{
    const auto &p = camera.position;
    return std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z) &&
           std::abs(p.x) <= 10000 && std::abs(p.y) <= 10000 && std::abs(p.z) <= 10000 &&
           std::isfinite(camera.yawDeg) && std::abs(camera.yawDeg) <= 180 &&
           std::isfinite(camera.pitchDeg) && std::abs(camera.pitchDeg) <= 89 &&
           std::isfinite(camera.speedMps) && camera.speedMps >= .1 && camera.speedMps <= 100;
}
inline double wrapDegrees(double angle)
{
    double wrapped = std::fmod(angle + 180, 360);
    if (wrapped < 0)
        wrapped += 360;
    return wrapped - 180;
}
inline bool look(Camera &camera, double yawDelta, double pitchDelta, std::string &error)
{
    if (!validCamera(camera) || !std::isfinite(yawDelta) || !std::isfinite(pitchDelta) ||
        std::abs(yawDelta) > 180 || std::abs(pitchDelta) > 180)
    {
        error = "Invalid camera state or look delta; each look delta must be finite within +/-180 "
                "degrees.";
        return false;
    }
    camera.yawDeg = wrapDegrees(camera.yawDeg + yawDelta);
    camera.pitchDeg = std::clamp(camera.pitchDeg + pitchDelta, -89.0, 89.0);
    error.clear();
    return true;
}
inline bool step(Camera &camera, const Controls &controls, double dt, std::string &error)
{
    if (!validCamera(camera) || !std::isfinite(dt) || dt < 0 || dt > .1)
    {
        error = "Invalid camera state or timestep; use 0-0.1 seconds.";
        return false;
    }
    for (const auto value :
         {controls.forward, controls.right, controls.up, controls.yaw, controls.pitch})
    {
        if (!std::isfinite(value) || std::abs(value) > 1)
        {
            error = "Camera control values must be finite within [-1,1].";
            return false;
        }
    }
    auto next = camera;
    if (!look(next, controls.yaw * 90 * dt, controls.pitch * 90 * dt, error))
        return false;
    constexpr double rad = 3.1415926535897932384626433832795 / 180;
    const double yaw = next.yawDeg * rad, pitch = next.pitchDeg * rad;
    const Vec3 forward{std::cos(pitch) * std::cos(yaw), std::cos(pitch) * std::sin(yaw),
                       std::sin(pitch)},
        right{-std::sin(yaw), std::cos(yaw), 0};
    auto movement = forward * controls.forward + right * controls.right + Vec3{0, 0, controls.up};
    const double magnitude = length(movement);
    if (magnitude > 1)
        movement = movement * (1 / magnitude);
    next.position = next.position + movement * (next.speedMps * dt);
    if (!validCamera(next))
    {
        error = "Camera movement would exceed the +/-10000 metre world limit.";
        return false;
    }
    camera = next;
    error.clear();
    return true;
}
} // namespace fractal
