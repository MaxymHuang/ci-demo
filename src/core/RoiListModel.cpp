#include "core/RoiListModel.h"

#include "core/AlgorithmRegistry.h"

#include <QColor>

namespace xri {

RoiListModel::RoiListModel(InspectionPlan* plan, QObject* parent)
    : QAbstractListModel(parent)
    , m_plan(plan)
{
    connect(plan, &InspectionPlan::roiAboutToBeAdded, this,
            [this](int index) { beginInsertRows({}, index, index); });
    connect(plan, &InspectionPlan::roiAdded, this, [this](int) { endInsertRows(); });
    connect(plan, &InspectionPlan::roiAboutToBeRemoved, this,
            [this](int index) { beginRemoveRows({}, index, index); });
    connect(plan, &InspectionPlan::roiRemoved, this, [this](int) { endRemoveRows(); });
    connect(plan, &InspectionPlan::roiChanged, this, [this](int index) {
        const QModelIndex mi = this->index(index);
        emit dataChanged(mi, mi);
    });
    connect(plan, &InspectionPlan::planReset, this, [this] {
        beginResetModel();
        m_verdicts.clear();
        endResetModel();
    });
}

int RoiListModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : m_plan->count();
}

QVariant RoiListModel::data(const QModelIndex& index, int role) const
{
    const Roi* roi = m_plan->roiAt(index.row());
    if (!index.isValid() || !roi)
        return {};

    switch (role) {
    case Qt::DisplayRole: {
        const IAlgorithm* algorithm = AlgorithmRegistry::instance().byId(roi->algorithmId);
        return algorithm ? QStringLiteral("%1 — %2").arg(roi->name, algorithm->displayName())
                         : roi->name;
    }
    case Qt::EditRole:
        return roi->name;
    case Qt::DecorationRole: {
        const auto it = m_verdicts.constFind(roi->id);
        if (it == m_verdicts.constEnd())
            return QColor(QStringLiteral("#5c6470"));
        return it->pass ? QColor(QStringLiteral("#3fb950")) : QColor(QStringLiteral("#f85149"));
    }
    case Qt::ToolTipRole: {
        const auto it = m_verdicts.constFind(roi->id);
        return it != m_verdicts.constEnd() ? it->detail : QVariant();
    }
    case IdRole:
        return roi->id;
    case RectRole:
        return roi->rect;
    case AlgorithmIdRole:
        return roi->algorithmId;
    case VerdictRole: {
        const auto it = m_verdicts.constFind(roi->id);
        return it != m_verdicts.constEnd() ? QVariant(it->pass) : QVariant();
    }
    default:
        return {};
    }
}

bool RoiListModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    const Roi* existing = m_plan->roiAt(index.row());
    if (!index.isValid() || !existing || role != Qt::EditRole)
        return false;
    const QString name = value.toString().trimmed();
    if (name.isEmpty())
        return false;
    Roi updated = *existing;
    updated.name = name;
    return m_plan->updateRoi(updated); // roiChanged signal emits dataChanged
}

Qt::ItemFlags RoiListModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
}

void RoiListModel::setResults(const InspectionRunResult& result)
{
    m_verdicts.clear();
    for (const RoiResult& r : result.roiResults) {
        if (r.evaluated)
            m_verdicts.insert(r.roiId, r.result);
    }
    if (rowCount() > 0)
        emit dataChanged(index(0), index(rowCount() - 1));
}

void RoiListModel::clearResults()
{
    m_verdicts.clear();
    if (rowCount() > 0)
        emit dataChanged(index(0), index(rowCount() - 1));
}

} // namespace xri
