#include "pages/AlgorithmsPage.h"

#include "core/AlgorithmRegistry.h"
#include "core/ReportExporter.h"

#include <QComboBox>
#include <QDateTime>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListView>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

namespace xri {

AlgorithmsPage::AlgorithmsPage(InspectionSession* session, QWidget* parent)
    : QWidget(parent)
    , m_session(session)
{
    m_roiList = new QListView;
    m_roiList->setObjectName("algoRoiList");
    m_roiList->setModel(session->roiModel());
    m_roiList->setEditTriggers(QListView::NoEditTriggers);

    auto* leftLayout = new QVBoxLayout;
    leftLayout->addWidget(new QLabel(tr("Inspection boxes")));
    leftLayout->addWidget(m_roiList, 1);

    auto* leftPanel = new QWidget;
    leftPanel->setLayout(leftLayout);
    leftPanel->setFixedWidth(260);

    m_selectionLabel = new QLabel(tr("Select an inspection box"));
    m_selectionLabel->setObjectName("selectionLabel");

    m_algorithmCombo = new QComboBox;
    m_algorithmCombo->setObjectName("algorithmCombo");
    m_algorithmCombo->addItem(tr("— none —"), QString());
    for (const IAlgorithm* algorithm : AlgorithmRegistry::instance().all())
        m_algorithmCombo->addItem(algorithm->displayName(), algorithm->id());
    m_algorithmCombo->setEnabled(false);

    m_paramContainer = new QWidget;
    m_paramForm = new QFormLayout(m_paramContainer);
    m_paramForm->setContentsMargins(0, 0, 0, 0);

    auto* assignForm = new QFormLayout;
    assignForm->addRow(tr("Algorithm"), m_algorithmCombo);
    auto* assignBox = new QGroupBox(tr("Algorithm assignment"));
    auto* assignLayout = new QVBoxLayout(assignBox);
    assignLayout->addWidget(m_selectionLabel);
    assignLayout->addLayout(assignForm);
    assignLayout->addWidget(m_paramContainer);

    m_runButton = new QPushButton(tr("Run Inspection"));
    m_runButton->setObjectName("runInspectionButton");

    m_overallLabel = new QLabel(tr("Not run yet"));
    m_overallLabel->setObjectName("overallResultLabel");
    m_overallLabel->setAlignment(Qt::AlignCenter);
    QFont overallFont = m_overallLabel->font();
    overallFont.setPointSizeF(overallFont.pointSizeF() + 4);
    overallFont.setBold(true);
    m_overallLabel->setFont(overallFont);

    m_exportButton = new QPushButton(tr("Export Results…"));
    m_exportButton->setObjectName("exportButton");
    m_exportButton->setEnabled(false);

    auto* rightLayout = new QVBoxLayout;
    rightLayout->addWidget(assignBox);
    rightLayout->addWidget(m_runButton);
    rightLayout->addWidget(m_overallLabel);
    rightLayout->addWidget(m_exportButton);
    rightLayout->addStretch();

    auto* layout = new QHBoxLayout(this);
    layout->addWidget(leftPanel);
    layout->addLayout(rightLayout, 1);

    connect(m_roiList->selectionModel(), &QItemSelectionModel::currentChanged, this,
            [this] { onRoiSelectionChanged(); });
    connect(m_algorithmCombo, &QComboBox::currentIndexChanged, this,
            &AlgorithmsPage::onAlgorithmChanged);
    connect(m_runButton, &QPushButton::clicked, this, &AlgorithmsPage::runInspection);
    connect(m_exportButton, &QPushButton::clicked, this, &AlgorithmsPage::exportResults);
    connect(session, &InspectionSession::resultsChanged, this,
            &AlgorithmsPage::updateOverallLabel);
}

QUuid AlgorithmsPage::selectedRoiId() const
{
    const QModelIndex current = m_roiList->currentIndex();
    return current.isValid()
        ? m_session->roiModel()->data(current, RoiListModel::IdRole).toUuid()
        : QUuid();
}

void AlgorithmsPage::onRoiSelectionChanged()
{
    const Roi* roi = m_session->plan()->roiById(selectedRoiId());
    m_algorithmCombo->setEnabled(roi != nullptr);
    if (!roi) {
        m_selectionLabel->setText(tr("Select an inspection box"));
        rebuildParamForm();
        return;
    }

    m_loading = true;
    m_selectionLabel->setText(tr("Configuring: %1").arg(roi->name));
    const int comboIndex = std::max(0, m_algorithmCombo->findData(roi->algorithmId));
    m_algorithmCombo->setCurrentIndex(comboIndex);
    m_loading = false;
    rebuildParamForm();
}

void AlgorithmsPage::onAlgorithmChanged(int comboIndex)
{
    if (m_loading)
        return;
    const Roi* existing = m_session->plan()->roiById(selectedRoiId());
    if (!existing)
        return;

    const QString algorithmId = m_algorithmCombo->itemData(comboIndex).toString();
    Roi updated = *existing;
    updated.algorithmId = algorithmId;
    const IAlgorithm* algorithm = AlgorithmRegistry::instance().byId(algorithmId);
    updated.params = algorithm ? algorithm->defaultParams() : QVariantMap();
    m_session->plan()->updateRoi(updated);
    rebuildParamForm();
}

void AlgorithmsPage::rebuildParamForm()
{
    while (m_paramForm->rowCount() > 0)
        m_paramForm->removeRow(0);

    const Roi* roi = m_session->plan()->roiById(selectedRoiId());
    const IAlgorithm* algorithm =
        roi ? AlgorithmRegistry::instance().byId(roi->algorithmId) : nullptr;
    if (!algorithm)
        return;

    for (const ParamSpec& spec : algorithm->paramSpecs()) {
        auto* spin = new QDoubleSpinBox;
        spin->setObjectName(QStringLiteral("param_%1").arg(spec.key));
        spin->setRange(spec.min, spec.max);
        spin->setDecimals(spec.decimals);
        spin->setValue(roi->params.value(spec.key, spec.defaultValue).toDouble());
        m_paramForm->addRow(spec.label, spin);

        const QString key = spec.key;
        connect(spin, &QDoubleSpinBox::valueChanged, this,
                [this, key](double value) { writeParam(key, value); });
    }
}

void AlgorithmsPage::writeParam(const QString& key, double value)
{
    if (m_loading)
        return;
    const Roi* existing = m_session->plan()->roiById(selectedRoiId());
    if (!existing)
        return;
    Roi updated = *existing;
    updated.params.insert(key, value);
    m_session->plan()->updateRoi(updated);
}

void AlgorithmsPage::runInspection()
{
    if (!m_session->hasScan()) {
        emit statusMessage(tr("No scan acquired — run a scan first"));
        return;
    }
    const InspectionRunResult& result = m_session->runInspection();
    emit statusMessage(tr("Inspection finished: %1 (%2/%3 passed)")
                           .arg(result.overallPass ? tr("PASS") : tr("FAIL"))
                           .arg(result.passCount())
                           .arg(result.evaluatedCount()));
}

void AlgorithmsPage::exportResults()
{
    const QString dir = QFileDialog::getExistingDirectory(this, tr("Export inspection results"));
    if (dir.isEmpty())
        return;

    const QString baseName = QStringLiteral("inspection_%1").arg(
        QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_hhmmss")));
    const ReportExporter::Outcome outcome = ReportExporter::exportAll(
        dir, baseName, m_session->scanImage(), m_session->scanParams(),
        m_session->lastResult());

    if (outcome.ok) {
        emit statusMessage(tr("Exported %1 files to %2").arg(outcome.writtenFiles.size()).arg(dir));
    } else {
        QMessageBox::warning(this, tr("Export failed"), outcome.error);
    }
}

void AlgorithmsPage::updateOverallLabel()
{
    m_exportButton->setEnabled(m_session->hasResult());
    if (!m_session->hasResult()) {
        m_overallLabel->setText(tr("Not run yet"));
        m_overallLabel->setStyleSheet(QString());
        return;
    }
    const InspectionRunResult& result = m_session->lastResult();
    if (result.overallPass) {
        m_overallLabel->setText(tr("PASS (%1/%2)")
                                    .arg(result.passCount()).arg(result.evaluatedCount()));
        m_overallLabel->setStyleSheet(QStringLiteral("color: #3fb950;"));
    } else {
        m_overallLabel->setText(tr("FAIL (%1/%2)")
                                    .arg(result.passCount()).arg(result.evaluatedCount()));
        m_overallLabel->setStyleSheet(QStringLiteral("color: #f85149;"));
    }
}

} // namespace xri
