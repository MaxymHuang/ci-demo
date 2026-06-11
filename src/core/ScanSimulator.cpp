#include "core/ScanSimulator.h"

#include <algorithm>

namespace xri {

namespace {
constexpr int kTargetTickCount = 40;
}

ScanSimulator::ScanSimulator(QObject* parent)
    : QObject(parent)
{
    connect(&m_timer, &QTimer::timeout, this, &ScanSimulator::tick);
}

void ScanSimulator::start(const ScanParams& params)
{
    cancel();

    m_full = SyntheticXraySource::generate(params);
    m_partial = QImage(m_full.size(), m_full.format());
    m_partial.fill(0);
    m_revealedRows = 0;
    m_rowsPerTick = std::max(1, m_full.height() / kTargetTickCount);

    emit started();
    emit progressChanged(0);
    m_timer.start(m_tickIntervalMs);
}

void ScanSimulator::cancel()
{
    if (!m_timer.isActive())
        return;
    m_timer.stop();
    emit cancelled();
}

void ScanSimulator::tick()
{
    const int h = m_full.height();
    const int newRows = std::min(m_rowsPerTick, h - m_revealedRows);
    for (int y = m_revealedRows; y < m_revealedRows + newRows; ++y)
        std::copy_n(m_full.scanLine(y), m_full.bytesPerLine(), m_partial.scanLine(y));
    m_revealedRows += newRows;

    emit progressChanged(m_revealedRows * 100 / h);
    emit partialImage(m_partial);

    if (m_revealedRows >= h) {
        m_timer.stop();
        emit finished(m_full);
    }
}

} // namespace xri
