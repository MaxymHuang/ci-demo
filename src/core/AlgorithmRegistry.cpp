#include "core/AlgorithmRegistry.h"

#include "core/Algorithms.h"

namespace xri {

const AlgorithmRegistry& AlgorithmRegistry::instance()
{
    static const AlgorithmRegistry registry;
    return registry;
}

AlgorithmRegistry::AlgorithmRegistry()
{
    m_algorithms.push_back(std::make_unique<MeanIntensityAlgorithm>());
    m_algorithms.push_back(std::make_unique<DensityRangeAlgorithm>());
    m_algorithms.push_back(std::make_unique<BlobCountAlgorithm>());
    m_algorithms.push_back(std::make_unique<EdgeStrengthAlgorithm>());
}

QList<const IAlgorithm*> AlgorithmRegistry::all() const
{
    QList<const IAlgorithm*> list;
    list.reserve(static_cast<int>(m_algorithms.size()));
    for (const auto& algorithm : m_algorithms)
        list.append(algorithm.get());
    return list;
}

const IAlgorithm* AlgorithmRegistry::byId(const QString& id) const
{
    for (const auto& algorithm : m_algorithms) {
        if (algorithm->id() == id)
            return algorithm.get();
    }
    return nullptr;
}

} // namespace xri
