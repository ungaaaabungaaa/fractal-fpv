#include "FractalCore.h"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

using namespace fractal;
static int groups = 0;
static void check(bool value, const char *message)
{
    if (!value)
        throw std::runtime_error(message);
}
static bool near(double a, double b, double tolerance = 1e-8)
{
    return std::abs(a - b) <= tolerance * std::max(1.0, std::abs(b));
}
static void passed(const char *name)
{
    ++groups;
    std::cout << "PASS " << name << '\n';
}

int main()
{
    try
    {
        const int cells[] = {1, 20, 400, 8000}, faces[] = {6, 72, 1056, 18048};
        for (int depth = 0; depth <= 3; ++depth)
        {
            const auto result = generate(depth, 20);
            check(result.ok(), "valid generation failed");
            const auto &mesh = result.mesh;
            check(mesh.retainedCells == cells[depth] && mesh.exposedFaces == faces[depth],
                  "canonical counts differ");
            check(mesh.vertices.size() == static_cast<std::size_t>(faces[depth] * 4),
                  "face vertex count");
            check(mesh.indices.size() == static_cast<std::size_t>(faces[depth] * 6),
                  "triangle count");
            check(mesh.normals.size() == mesh.vertices.size() &&
                      mesh.uv.size() == mesh.vertices.size(),
                  "attribute cardinality");
            check(near(mesh.volumeM3, 8000 * std::pow(20.0 / 27, depth)), "voxel volume units");
            double volume = 0, area = 0;
            for (std::size_t t = 0; t < mesh.indices.size(); t += 3)
            {
                for (int j = 0; j < 3; ++j)
                    check(mesh.indices[t + j] < mesh.vertices.size(),
                          "index outside vertex buffer");
                const auto a = mesh.vertices[mesh.indices[t]],
                           b = mesh.vertices[mesh.indices[t + 1]],
                           c = mesh.vertices[mesh.indices[t + 2]];
                const auto normal = cross(b - a, c - a);
                check(dot(normal, mesh.normals[mesh.indices[t]]) > 0,
                      "winding does not match outward normal");
                area += length(normal) / 2;
                volume += dot(a, cross(b, c)) / 6;
            }
            check(near(volume, mesh.volumeM3),
                  "signed mesh volume differs from independent voxel volume");
            check(near(area, mesh.surfaceAreaM2), "triangle area differs from face area");
            for (const auto &p : mesh.vertices)
                check(std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z) &&
                          std::abs(p.x) <= 10.00000001 && std::abs(p.y) <= 10.00000001 &&
                          std::abs(p.z) <= 10.00000001,
                      "bounds/nonfinite vertex");
        }
        passed("canonical occupancy, exposed faces, attributes, bounds, signed volume and area "
               "(depths 0-3)");
        for (const auto side : {0.0, 101.0, std::numeric_limits<double>::quiet_NaN(),
                                std::numeric_limits<double>::infinity()})
            check(!generate(1, side).ok(), "bad side accepted");
        check(!generate(-1, 20).ok() && !generate(4, 20).ok(), "bad depth accepted");
        passed("invalid input and work/output depth bounds");
        const auto a = generate(2, 12).mesh, b = generate(2, 24).mesh, c = generate(2, 12).mesh;
        check(a.indices == c.indices && a.vertices == c.vertices, "generation not deterministic");
        check(near(b.surfaceAreaM2, a.surfaceAreaM2 * 4) && near(b.volumeM3, a.volumeM3 * 8),
              "size scaling units");
        for (std::size_t i = 0; i < a.vertices.size(); ++i)
            check(b.vertices[i] == a.vertices[i] * 2, "coordinate scaling");
        passed("deterministic order and dimensional scaling");
        auto reversed = indexOrder(a, true);
        check(indexOrder(a, false) == a.indices, "identity triangle order");
        check(reversed.size() == a.indices.size(), "reversal cardinality");
        for (std::size_t t = 0; t < reversed.size(); t += 3)
            check(reversed[t] == a.indices[t] && reversed[t + 1] == a.indices[t + 2] &&
                      reversed[t + 2] == a.indices[t + 1],
                  "triangle reversal");
        check(toCentimetres({1, -2, .5}) == Vec3{100, -200, 50}, "centimetre conversion");
        passed("native index-order adapter and metre-to-centimetre conversion");
        Camera camera;
        camera.position = {0, 0, 0};
        camera.yawDeg = 0;
        camera.pitchDeg = 0;
        camera.speedMps = 10;
        Controls forward;
        forward.forward = 1;
        std::string error;
        auto split = camera, whole = camera;
        check(step(whole, forward, .1, error) && step(split, forward, .05, error) &&
                  step(split, forward, .05, error),
              "movement failed");
        check(near(whole.position.x, 1) && whole.position == split.position,
              "constant-orientation timestep scaling");
        Controls diagonal;
        diagonal.forward = diagonal.right = diagonal.up = 1;
        auto before = camera;
        check(step(camera, diagonal, .1, error), "diagonal failed");
        check(length(camera.position - before.position) <= 1.000000001, "diagonal speed boost");
        passed("time-scaled translation and diagonal speed cap");
        camera = before;
        check(look(camera, 170, 80, error) && look(camera, 30, 30, error), "look failed");
        check(near(camera.yawDeg, -160) && near(camera.pitchDeg, 89), "yaw wrap/pitch clamp");
        auto old = camera;
        Controls invalid;
        invalid.forward = 2;
        check(!step(camera, invalid, .1, error) && camera.position == old.position,
              "invalid controls mutated state");
        check(!step(camera, forward, .101, error) && !step(camera, forward, -.01, error),
              "dt bound");
        check(!look(camera, 181, 0, error) && near(camera.yawDeg, old.yawDeg),
              "invalid look mutated state");
        camera.position = {10000, 0, 0};
        camera.yawDeg = 0;
        camera.pitchDeg = 0;
        check(!step(camera, forward, .1, error) && camera.position.x == 10000, "world bound");
        passed("look limits, invalid-state preservation and world boundary");
        Camera pitched;
        pitched.position = {0, 0, 0};
        pitched.pitchDeg = 80;
        auto initial = pitched.position;
        check(step(pitched, diagonal, .1, error), "pitched diagonal failed");
        check(length(pitched.position - initial) <= pitched.speedMps * .1 + 1e-9,
              "pitched world-vertical diagonal boost");
        check(!step(pitched, forward, std::numeric_limits<double>::quiet_NaN(), error),
              "NaN dt accepted");
        passed("pitched movement normalization and nonfinite rejection");
        std::cout << groups << " test groups passed.\n";
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << "FAIL " << error.what() << '\n';
        return 1;
    }
}
