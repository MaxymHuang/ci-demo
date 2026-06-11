#include "core/Algorithms.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace xri {

QVariantMap IAlgorithm::defaultParams() const
{
    QVariantMap map;
    for (const ParamSpec& spec : paramSpecs())
        map.insert(spec.key, spec.defaultValue);
    return map;
}

double IAlgorithm::param(const QVariantMap& params, const QString& key) const
{
    if (params.contains(key))
        return params.value(key).toDouble();
    for (const ParamSpec& spec : paramSpecs()) {
        if (spec.key == key)
            return spec.defaultValue;
    }
    return 0.0;
}

namespace {

// All algorithms work on 8-bit grayscale.
QImage toGray(const QImage& image)
{
    return image.format() == QImage::Format_Grayscale8
        ? image
        : image.convertToFormat(QImage::Format_Grayscale8);
}

} // namespace

// --- MeanIntensityAlgorithm ---

QList<ParamSpec> MeanIntensityAlgorithm::paramSpecs() const
{
    return {
        { "minMean", "Min mean", 60.0, 0.0, 255.0, 1 },
        { "maxMean", "Max mean", 220.0, 0.0, 255.0, 1 },
    };
}

AlgorithmResult MeanIntensityAlgorithm::evaluate(const QImage& image, const QRect& roi,
                                                 const QVariantMap& params) const
{
    const QImage gray = toGray(image);
    const double minMean = param(params, "minMean");
    const double maxMean = param(params, "maxMean");

    qint64 sum = 0;
    for (int y = roi.top(); y <= roi.bottom(); ++y) {
        const uchar* line = gray.constScanLine(y);
        for (int x = roi.left(); x <= roi.right(); ++x)
            sum += line[x];
    }
    const double mean = static_cast<double>(sum) / (qint64(roi.width()) * roi.height());

    AlgorithmResult result;
    result.measuredValue = mean;
    result.unit = QStringLiteral("gray");
    result.pass = mean >= minMean && mean <= maxMean;
    result.detail = QStringLiteral("mean=%1, allowed=[%2, %3]")
                        .arg(mean, 0, 'f', 1).arg(minMean).arg(maxMean);
    return result;
}

// --- DensityRangeAlgorithm ---

QList<ParamSpec> DensityRangeAlgorithm::paramSpecs() const
{
    return {
        { "minAllowed", "Min pixel", 20.0, 0.0, 255.0, 0 },
        { "maxAllowed", "Max pixel", 250.0, 0.0, 255.0, 0 },
    };
}

AlgorithmResult DensityRangeAlgorithm::evaluate(const QImage& image, const QRect& roi,
                                                const QVariantMap& params) const
{
    const QImage gray = toGray(image);
    const int minAllowed = static_cast<int>(param(params, "minAllowed"));
    const int maxAllowed = static_cast<int>(param(params, "maxAllowed"));

    int minPixel = 255;
    int maxPixel = 0;
    for (int y = roi.top(); y <= roi.bottom(); ++y) {
        const uchar* line = gray.constScanLine(y);
        for (int x = roi.left(); x <= roi.right(); ++x) {
            minPixel = std::min(minPixel, int(line[x]));
            maxPixel = std::max(maxPixel, int(line[x]));
        }
    }

    AlgorithmResult result;
    result.measuredValue = minPixel;
    result.unit = QStringLiteral("gray");
    result.pass = minPixel >= minAllowed && maxPixel <= maxAllowed;
    result.detail = QStringLiteral("min=%1, max=%2, allowed=[%3, %4]")
                        .arg(minPixel).arg(maxPixel).arg(minAllowed).arg(maxAllowed);
    return result;
}

// --- BlobCountAlgorithm ---

QList<ParamSpec> BlobCountAlgorithm::paramSpecs() const
{
    return {
        { "threshold", "Dark threshold", 60.0, 0.0, 255.0, 0 },
        { "minBlobArea", "Min blob area", 12.0, 1.0, 100000.0, 0 },
        { "maxBlobs", "Max blobs", 0.0, 0.0, 1000.0, 0 },
    };
}

AlgorithmResult BlobCountAlgorithm::evaluate(const QImage& image, const QRect& roi,
                                             const QVariantMap& params) const
{
    const QImage gray = toGray(image);
    const int threshold = static_cast<int>(param(params, "threshold"));
    const int minBlobArea = static_cast<int>(param(params, "minBlobArea"));
    const int maxBlobs = static_cast<int>(param(params, "maxBlobs"));

    const int w = roi.width();
    const int h = roi.height();
    std::vector<uchar> dark(size_t(w) * h);
    for (int y = 0; y < h; ++y) {
        const uchar* line = gray.constScanLine(roi.top() + y);
        for (int x = 0; x < w; ++x)
            dark[size_t(y) * w + x] = line[roi.left() + x] < threshold ? 1 : 0;
    }

    // 4-connected flood fill over the dark mask.
    int blobCount = 0;
    std::vector<int> stack;
    for (int start = 0; start < w * h; ++start) {
        if (!dark[start])
            continue;
        int area = 0;
        stack.push_back(start);
        dark[start] = 0;
        while (!stack.empty()) {
            const int idx = stack.back();
            stack.pop_back();
            ++area;
            const int x = idx % w;
            const int y = idx / w;
            const int neighbors[4] = { x > 0 ? idx - 1 : -1, x < w - 1 ? idx + 1 : -1,
                                       y > 0 ? idx - w : -1, y < h - 1 ? idx + w : -1 };
            for (int n : neighbors) {
                if (n >= 0 && dark[n]) {
                    dark[n] = 0;
                    stack.push_back(n);
                }
            }
        }
        if (area >= minBlobArea)
            ++blobCount;
    }

    AlgorithmResult result;
    result.measuredValue = blobCount;
    result.unit = QStringLiteral("blobs");
    result.pass = blobCount <= maxBlobs;
    result.detail = QStringLiteral("found %1 blob(s) >= %2 px below %3, allowed max %4")
                        .arg(blobCount).arg(minBlobArea).arg(threshold).arg(maxBlobs);
    return result;
}

// --- EdgeStrengthAlgorithm ---

QList<ParamSpec> EdgeStrengthAlgorithm::paramSpecs() const
{
    return {
        { "minStrength", "Min edge strength", 8.0, 0.0, 1000.0, 1 },
    };
}

AlgorithmResult EdgeStrengthAlgorithm::evaluate(const QImage& image, const QRect& roi,
                                                const QVariantMap& params) const
{
    const QImage gray = toGray(image);
    const double minStrength = param(params, "minStrength");

    // Sobel over the ROI interior (skip the 1px border).
    double sum = 0.0;
    qint64 samples = 0;
    for (int y = roi.top() + 1; y <= roi.bottom() - 1; ++y) {
        const uchar* above = gray.constScanLine(y - 1);
        const uchar* line = gray.constScanLine(y);
        const uchar* below = gray.constScanLine(y + 1);
        for (int x = roi.left() + 1; x <= roi.right() - 1; ++x) {
            const double gx = (above[x + 1] + 2.0 * line[x + 1] + below[x + 1])
                - (above[x - 1] + 2.0 * line[x - 1] + below[x - 1]);
            const double gy = (below[x - 1] + 2.0 * below[x] + below[x + 1])
                - (above[x - 1] + 2.0 * above[x] + above[x + 1]);
            sum += std::sqrt(gx * gx + gy * gy);
            ++samples;
        }
    }
    const double mean = samples > 0 ? sum / samples : 0.0;

    AlgorithmResult result;
    result.measuredValue = mean;
    result.unit = QStringLiteral("grad");
    result.pass = mean >= minStrength;
    result.detail = QStringLiteral("mean gradient=%1, required >= %2")
                        .arg(mean, 0, 'f', 1).arg(minStrength);
    return result;
}

} // namespace xri
