#pragma once

#include <QImage>
#include <QtGlobal>

namespace xri {

// Parameters for one simulated 2D x-ray acquisition.
struct ScanParams {
    int width = 640;
    int height = 480;
    quint32 seed = 42;
    int objectCount = 4;
    double noiseSigma = 6.0;
    bool addDefect = true;

    bool operator==(const ScanParams& other) const = default;
};

// Procedurally generates a grayscale "radiograph": bright background (air),
// darker elliptical objects with Beer-Lambert-style attenuation, an optional
// dense inclusion defect, vignette and sensor noise. Same params => same image.
class SyntheticXraySource
{
public:
    static QImage generate(const ScanParams& params);
};

} // namespace xri
