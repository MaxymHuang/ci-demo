#pragma once

#include "core/Algorithm.h"

#include <memory>
#include <vector>

namespace xri {

// Central catalogue of available algorithms. The UI builds its combo boxes
// from all(), so new algorithms only need to be registered here.
class AlgorithmRegistry
{
public:
    static const AlgorithmRegistry& instance();

    QList<const IAlgorithm*> all() const;
    const IAlgorithm* byId(const QString& id) const;

private:
    AlgorithmRegistry();

    std::vector<std::unique_ptr<IAlgorithm>> m_algorithms;
};

} // namespace xri
