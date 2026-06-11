#pragma once

#include "core/InspectionEngine.h"
#include "core/InspectionPlan.h"

#include <QAbstractListModel>
#include <QHash>

namespace xri {

// List adapter over InspectionPlan, shared by the Setup and Algorithms pages.
// Also carries the latest per-ROI verdict for decoration.
class RoiListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Role {
        IdRole = Qt::UserRole + 1,
        RectRole,
        AlgorithmIdRole,
        VerdictRole, // QVariant() = not evaluated, else bool pass
    };

    explicit RoiListModel(InspectionPlan* plan, QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    void setResults(const InspectionRunResult& result);
    void clearResults();

private:
    InspectionPlan* m_plan;
    QHash<QUuid, AlgorithmResult> m_verdicts;
};

} // namespace xri
