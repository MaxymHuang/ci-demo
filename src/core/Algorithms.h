#pragma once

#include "core/Algorithm.h"

namespace xri {

// Mean gray level inside the ROI must fall within [minMean, maxMean].
class MeanIntensityAlgorithm : public IAlgorithm
{
public:
    QString id() const override { return QStringLiteral("mean_intensity"); }
    QString displayName() const override { return QStringLiteral("Mean intensity band"); }
    QList<ParamSpec> paramSpecs() const override;
    AlgorithmResult evaluate(const QImage& image, const QRect& roi,
                             const QVariantMap& params) const override;
};

// Every pixel inside the ROI must stay within [minAllowed, maxAllowed].
class DensityRangeAlgorithm : public IAlgorithm
{
public:
    QString id() const override { return QStringLiteral("density_range"); }
    QString displayName() const override { return QStringLiteral("Density range check"); }
    QList<ParamSpec> paramSpecs() const override;
    AlgorithmResult evaluate(const QImage& image, const QRect& roi,
                             const QVariantMap& params) const override;
};

// Counts dark connected components (pixels < threshold, 4-connected, area >=
// minBlobArea); passes when the count does not exceed maxBlobs.
class BlobCountAlgorithm : public IAlgorithm
{
public:
    QString id() const override { return QStringLiteral("blob_count"); }
    QString displayName() const override { return QStringLiteral("Dark blob count"); }
    QList<ParamSpec> paramSpecs() const override;
    AlgorithmResult evaluate(const QImage& image, const QRect& roi,
                             const QVariantMap& params) const override;
};

// Mean Sobel gradient magnitude must reach minStrength (a structure must be
// present in the ROI).
class EdgeStrengthAlgorithm : public IAlgorithm
{
public:
    QString id() const override { return QStringLiteral("edge_strength"); }
    QString displayName() const override { return QStringLiteral("Edge strength"); }
    QList<ParamSpec> paramSpecs() const override;
    AlgorithmResult evaluate(const QImage& image, const QRect& roi,
                             const QVariantMap& params) const override;
};

} // namespace xri
