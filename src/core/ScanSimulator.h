#pragma once

#include "core/SyntheticXraySource.h"

#include <QImage>
#include <QObject>
#include <QTimer>

namespace xri {

// Mimics a line-scan acquisition: the full image is generated up front and
// revealed band-by-band on a timer. setTickInterval(0) makes tests run at
// event-loop speed with no real-time waits.
class ScanSimulator : public QObject
{
    Q_OBJECT
public:
    explicit ScanSimulator(QObject* parent = nullptr);

    void setTickInterval(int ms) { m_tickIntervalMs = ms; }
    int tickInterval() const { return m_tickIntervalMs; }
    bool isRunning() const { return m_timer.isActive(); }

public slots:
    void start(const ScanParams& params);
    void cancel();

signals:
    void started();
    void progressChanged(int percent);
    void partialImage(const QImage& image);
    void finished(const QImage& image);
    void cancelled();

private:
    void tick();

    QTimer m_timer;
    QImage m_full;
    QImage m_partial;
    int m_revealedRows = 0;
    int m_rowsPerTick = 1;
    int m_tickIntervalMs = 30;
};

} // namespace xri
