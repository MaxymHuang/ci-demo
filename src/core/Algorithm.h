#pragma once

#include <QImage>
#include <QList>
#include <QRect>
#include <QString>
#include <QVariantMap>

namespace xri {

// Describes one numeric parameter of an algorithm; the UI generates spin
// boxes from these, so adding an algorithm requires no UI changes.
struct ParamSpec {
    QString key;
    QString label;
    double defaultValue = 0.0;
    double min = 0.0;
    double max = 255.0;
    int decimals = 0; // 0 = integer-valued
};

struct AlgorithmResult {
    bool pass = false;
    double measuredValue = 0.0;
    QString unit;
    QString detail;
};

class IAlgorithm
{
public:
    virtual ~IAlgorithm() = default;

    virtual QString id() const = 0;
    virtual QString displayName() const = 0;
    virtual QList<ParamSpec> paramSpecs() const = 0;

    // `roi` must already be clamped to the image bounds and non-empty.
    virtual AlgorithmResult evaluate(const QImage& image, const QRect& roi,
                                     const QVariantMap& params) const = 0;

    QVariantMap defaultParams() const;

protected:
    // Parameter lookup with fallback to the spec default.
    double param(const QVariantMap& params, const QString& key) const;
};

} // namespace xri
