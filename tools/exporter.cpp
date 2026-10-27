#include "FractalCore.h"
#include <iomanip>
#include <iostream>
#include <locale>
#include <sstream>
#include <string>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif
#ifndef FRACTAL_CORE_SHA256
#define FRACTAL_CORE_SHA256 "untracked"
#endif
#ifndef FRACTAL_PRODUCER_SHA256
#define FRACTAL_PRODUCER_SHA256 "untracked"
#endif

using namespace fractal;
static void number(std::ostream &out, double value)
{
    out << std::fixed << std::setprecision(6) << (std::abs(value) < .0000005 ? 0 : value);
}
static void vector(std::ostream &out, const Vec3 &v)
{
    out << '[';
    number(out, v.x);
    out << ',';
    number(out, v.y);
    out << ',';
    number(out, v.z);
    out << ']';
}
static void fingerprint(std::ostream &out)
{
    out << "\"coreSha256\":\"" << FRACTAL_CORE_SHA256 << "\",\"producerSha256\":\""
        << FRACTAL_PRODUCER_SHA256 << '"';
}
static void report(std::ostream &out, const Mesh &mesh)
{
    const auto triangles = mesh.indices.size() / 3,
               naive = static_cast<std::size_t>(mesh.retainedCells) * 12;
    out << "{\"schemaVersion\":1,\"kind\":\"menger-mesh-report\",\"positionUnit\":\"m\",\"depth\":"
        << mesh.depth << ",\"sideM\":";
    number(out, mesh.sideM);
    out << ",\"cellSideM\":";
    number(out, mesh.cellSideM);
    out << ",\"retainedCells\":" << mesh.retainedCells << ",\"exposedFaces\":" << mesh.exposedFaces
        << ",\"vertices\":" << mesh.vertices.size() << ",\"triangles\":" << triangles
        << ",\"naiveTriangles\":" << naive << ",\"triangleReductionPercent\":";
    number(out, 100.0 * static_cast<double>(naive - triangles) / static_cast<double>(naive));
    out << ",\"surfaceAreaM2\":";
    number(out, mesh.surfaceAreaM2);
    out << ",\"voxelVolumeM3\":";
    number(out, mesh.volumeM3);
    out << ",\"decimalPlaces\":6,\"normalConvention\":\"outward-cross-positive\","
           "\"engineValidation\":\"pending\",";
    fingerprint(out);
    out << '}';
}
static void meshJson(std::ostream &out, const Mesh &mesh)
{
    out << "{\"schemaVersion\":1,\"kind\":\"menger-mesh\",\"report\":";
    report(out, mesh);
    out << ",\"positions\":[";
    bool first = true;
    for (const auto &v : mesh.vertices)
        for (const double value : {v.x, v.y, v.z})
        {
            if (!first)
                out << ',';
            first = false;
            number(out, value);
        }
    out << "],\"normals\":[";
    first = true;
    for (const auto &n : mesh.normals)
        for (const double value : {n.x, n.y, n.z})
        {
            if (!first)
                out << ',';
            first = false;
            number(out, value);
        }
    out << "],\"uv\":[";
    first = true;
    for (const auto &uv : mesh.uv)
        for (const double value : {uv.u, uv.v})
        {
            if (!first)
                out << ',';
            first = false;
            number(out, value);
        }
    out << "],\"indices\":[";
    first = true;
    for (const auto index : mesh.indices)
    {
        if (!first)
            out << ',';
        first = false;
        out << index;
    }
    out << "]}";
}
static void obj(std::ostream &out, const Mesh &mesh)
{
    out << "# Fractal FPV portable mesh, metres, outward CCW triangles\n# core_sha256 "
        << FRACTAL_CORE_SHA256 << '\n';
    for (const auto &v : mesh.vertices)
    {
        out << "v ";
        number(out, v.x);
        out << ' ';
        number(out, v.y);
        out << ' ';
        number(out, v.z);
        out << '\n';
    }
    for (const auto &n : mesh.normals)
    {
        out << "vn ";
        number(out, n.x);
        out << ' ';
        number(out, n.y);
        out << ' ';
        number(out, n.z);
        out << '\n';
    }
    for (std::size_t t = 0; t < mesh.indices.size(); t += 3)
    {
        out << 'f';
        for (int i = 0; i < 3; ++i)
        {
            const auto index = mesh.indices[t + static_cast<std::size_t>(i)] + 1;
            out << ' ' << index << "//" << index;
        }
        out << '\n';
    }
}
template <class T> static bool parse(const char *input, T &result)
{
    const std::string text(input);
    if (text.empty() || text.size() > 128)
        return false;
    std::istringstream stream(text);
    stream.imbue(std::locale::classic());
    stream >> std::noskipws >> result;
    return !stream.fail() && stream.eof();
}
static bool track(std::ostream &out)
{
    Camera camera;
    camera.position = {-30, 0, 0};
    camera.yawDeg = 0;
    camera.pitchDeg = 0;
    camera.speedMps = 5;
    Controls controls;
    controls.forward = 1;
    std::string error;
    out << "{\"schemaVersion\":1,\"kind\":\"virtual-camera-track\",\"positionUnit\":\"m\","
           "\"angleUnit\":\"deg\",\"sampleRateHz\":60,\"durationSeconds\":12,\"collision\":false,";
    fingerprint(out);
    out << ",\"note\":\"Kinematic core camera only; no wall collision, drone physics, real flight "
           "or engine render.\",\"samples\":[";
    for (int tick = 0; tick <= 720; ++tick)
    {
        if (tick > 0)
        {
            if (!step(camera, controls, 1.0 / 60, error))
                return false;
            out << ',';
        }
        out << "{\"t\":";
        number(out, tick / 60.0);
        out << ",\"position\":";
        vector(out, camera.position);
        out << ",\"yawDeg\":";
        number(out, camera.yawDeg);
        out << ",\"pitchDeg\":";
        number(out, camera.pitchDeg);
        out << '}';
    }
    out << "]}";
    return true;
}
int main(int argc, char **argv)
{
#ifdef _WIN32
    _setmode(_fileno(stdout), _O_BINARY);
#endif
    std::cout.imbue(std::locale::classic());
    try
    {
        if (argc == 2 && std::string(argv[1]) == "version")
        {
            std::cout << '{';
            fingerprint(std::cout);
            std::cout << "}\n";
            return 0;
        }
        if (argc == 2 && std::string(argv[1]) == "track")
        {
            std::ostringstream out;
            out.imbue(std::locale::classic());
            if (!track(out))
            {
                std::cerr << "Camera track failed.\n";
                return 1;
            }
            std::cout << out.str() << '\n';
            return 0;
        }
        if (argc == 5 && std::string(argv[1]) == "mesh")
        {
            int depth = 0;
            double side = 0;
            if (!parse(argv[2], depth) || !parse(argv[3], side))
            {
                std::cerr << "Use a whole depth and a finite numeric side.\n";
                return 2;
            }
            const auto result = generate(depth, side);
            if (!result.ok())
            {
                std::cerr << result.error << '\n';
                return 2;
            }
            const std::string format(argv[4]);
            if (format == "json")
                meshJson(std::cout, result.mesh);
            else if (format == "obj")
                obj(std::cout, result.mesh);
            else if (format == "report")
                report(std::cout, result.mesh);
            else
            {
                std::cerr << "Use json, obj, or report format.\n";
                return 2;
            }
            if (format != "obj")
                std::cout << '\n';
            return 0;
        }
        if (argc == 2 && std::string(argv[1]) == "--help")
        {
            std::cout << "fractal-export version | mesh DEPTH SIDE_M json|obj|report | track\n";
            return 0;
        }
        std::cerr << "Use --help for the bounded portable exporter interface.\n";
        return 2;
    }
    catch (const std::exception &)
    {
        std::cerr << "Export failed.\n";
        return 1;
    }
}
