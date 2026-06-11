#include "core/SyntheticXraySource.h"

#include <algorithm>
#include <cmath>
#include <random>
#include <vector>

namespace xri {
namespace {

constexpr double kPi = 3.14159265358979323846;

// Deterministic RNG. Box-Muller over raw mt19937 output instead of
// std::normal_distribution, whose sequence differs between MSVC and libc++.
class Rng
{
public:
    explicit Rng(quint32 seed) : m_gen(seed) {}

    double uniform() // (0, 1)
    {
        return (static_cast<double>(m_gen()) + 1.0)
            / (static_cast<double>(std::mt19937::max()) + 2.0);
    }

    double uniform(double lo, double hi) { return lo + (hi - lo) * uniform(); }

    double gaussian()
    {
        const double u1 = uniform();
        const double u2 = uniform();
        return std::sqrt(-2.0 * std::log(u1)) * std::cos(2.0 * kPi * u2);
    }

private:
    std::mt19937 m_gen;
};

struct Ellipse {
    double cx = 0, cy = 0, rx = 1, ry = 1;
    double density = 0; // attenuation coefficient

    // Material thickness at (x, y), modelling the ellipse as an ellipsoid slice.
    double thicknessAt(double x, double y) const
    {
        const double nx = (x - cx) / rx;
        const double ny = (y - cy) / ry;
        const double d2 = nx * nx + ny * ny;
        return d2 < 1.0 ? std::sqrt(1.0 - d2) : 0.0;
    }
};

} // namespace

QImage SyntheticXraySource::generate(const ScanParams& p)
{
    const int w = std::max(1, p.width);
    const int h = std::max(1, p.height);
    QImage img(w, h, QImage::Format_Grayscale8);

    Rng rng(p.seed);

    std::vector<Ellipse> objects;
    objects.reserve(std::max(0, p.objectCount) + 1);
    for (int i = 0; i < p.objectCount; ++i) {
        Ellipse e;
        e.rx = rng.uniform(0.06, 0.16) * w;
        e.ry = rng.uniform(0.08, 0.18) * h;
        e.cx = rng.uniform(0.15, 0.85) * w;
        e.cy = rng.uniform(0.15, 0.85) * h;
        e.density = rng.uniform(0.4, 0.9);
        objects.push_back(e);
    }

    if (p.addDefect && !objects.empty()) {
        // Dense inclusion inside the first object: a small, very dark blob.
        const Ellipse& host = objects.front();
        Ellipse defect;
        defect.cx = host.cx + rng.uniform(-0.3, 0.3) * host.rx;
        defect.cy = host.cy + rng.uniform(-0.3, 0.3) * host.ry;
        defect.rx = std::max(4.0, 0.18 * host.rx);
        defect.ry = std::max(4.0, 0.18 * host.ry);
        defect.density = 6.0;
        objects.push_back(defect);
    }

    const double centerX = w / 2.0;
    const double centerY = h / 2.0;
    const double maxR2 = centerX * centerX + centerY * centerY;
    constexpr double kSourceIntensity = 235.0;

    for (int y = 0; y < h; ++y) {
        uchar* line = img.scanLine(y);
        for (int x = 0; x < w; ++x) {
            double transmission = 1.0;
            for (const Ellipse& e : objects)
                transmission *= std::exp(-e.density * e.thicknessAt(x, y));

            const double dx = x - centerX;
            const double dy = y - centerY;
            const double vignette = 1.0 - 0.25 * (dx * dx + dy * dy) / maxR2;

            double value = kSourceIntensity * transmission * vignette;
            value += p.noiseSigma * rng.gaussian();
            line[x] = static_cast<uchar>(std::clamp(value, 0.0, 255.0));
        }
    }
    return img;
}

} // namespace xri
