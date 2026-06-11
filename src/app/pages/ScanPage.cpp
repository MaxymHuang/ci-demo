#include "pages/ScanPage.h"

#include <QCheckBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

namespace xri {

ScanPage::ScanPage(InspectionSession* session, QWidget* parent)
    : QWidget(parent)
    , m_session(session)
{
    auto* form = new QFormLayout;

    m_seedSpin = new QSpinBox;
    m_seedSpin->setObjectName("seedSpin");
    m_seedSpin->setRange(0, 999999);
    m_seedSpin->setValue(42);
    form->addRow(tr("Seed"), m_seedSpin);

    m_objectCountSpin = new QSpinBox;
    m_objectCountSpin->setObjectName("objectCountSpin");
    m_objectCountSpin->setRange(1, 12);
    m_objectCountSpin->setValue(4);
    form->addRow(tr("Objects"), m_objectCountSpin);

    m_defectCheck = new QCheckBox(tr("Inject defect"));
    m_defectCheck->setObjectName("defectCheck");
    m_defectCheck->setChecked(true);
    form->addRow(QString(), m_defectCheck);

    auto* paramsBox = new QGroupBox(tr("Acquisition parameters"));
    paramsBox->setLayout(form);

    m_startButton = new QPushButton(tr("Start Scan"));
    m_startButton->setObjectName("startScanButton");

    m_progressBar = new QProgressBar;
    m_progressBar->setObjectName("scanProgress");
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);

    m_preview = new QLabel(tr("No scan acquired"));
    m_preview->setObjectName("scanPreview");
    m_preview->setAlignment(Qt::AlignCenter);
    m_preview->setMinimumSize(480, 360);
    m_preview->setFrameShape(QFrame::StyledPanel);

    auto* sideLayout = new QVBoxLayout;
    sideLayout->addWidget(paramsBox);
    sideLayout->addWidget(m_startButton);
    sideLayout->addWidget(m_progressBar);
    sideLayout->addStretch();

    auto* layout = new QHBoxLayout(this);
    layout->addLayout(sideLayout, 0);
    layout->addWidget(m_preview, 1);

    connect(m_startButton, &QPushButton::clicked, this, &ScanPage::startScan);
    connect(&m_simulator, &ScanSimulator::progressChanged, m_progressBar,
            &QProgressBar::setValue);
    connect(&m_simulator, &ScanSimulator::partialImage, this, &ScanPage::showPreview);
    connect(&m_simulator, &ScanSimulator::finished, this, [this](const QImage& image) {
        showPreview(image);
        m_startButton->setEnabled(true);
        m_session->setScanImage(image, m_currentParams);

        emit statusMessage(tr("Scan complete (%1×%2)").arg(image.width()).arg(image.height()));
        emit scanCompleted();
    });
}

void ScanPage::startScan()
{
    m_currentParams = ScanParams{};
    m_currentParams.seed = quint32(m_seedSpin->value());
    m_currentParams.objectCount = m_objectCountSpin->value();
    m_currentParams.addDefect = m_defectCheck->isChecked();

    m_startButton->setEnabled(false);
    m_progressBar->setValue(0);
    emit statusMessage(tr("Scanning…"));
    m_simulator.start(m_currentParams);
}

void ScanPage::showPreview(const QImage& image)
{
    m_preview->setPixmap(QPixmap::fromImage(image).scaled(
        m_preview->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

} // namespace xri
