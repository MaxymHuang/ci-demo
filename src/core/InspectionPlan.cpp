#include "core/InspectionPlan.h"

#include <QJsonArray>

#include <algorithm>

namespace xri {

InspectionPlan::InspectionPlan(QObject* parent)
    : QObject(parent)
{
}

const Roi* InspectionPlan::roiById(const QUuid& id) const
{
    const int idx = indexOf(id);
    return idx >= 0 ? &m_rois[idx] : nullptr;
}

const Roi* InspectionPlan::roiAt(int index) const
{
    return index >= 0 && index < count() ? &m_rois[index] : nullptr;
}

int InspectionPlan::indexOf(const QUuid& id) const
{
    const auto it = std::find_if(m_rois.begin(), m_rois.end(),
                                 [&](const Roi& r) { return r.id == id; });
    return it == m_rois.end() ? -1 : static_cast<int>(it - m_rois.begin());
}

QUuid InspectionPlan::addRoi(const QRectF& rect)
{
    Roi roi;
    roi.name = QStringLiteral("ROI %1").arg(++m_nameCounter);
    roi.rect = rect.normalized();
    emit roiAboutToBeAdded(count());
    m_rois.push_back(roi);
    emit roiAdded(count() - 1);
    return roi.id;
}

bool InspectionPlan::removeRoi(const QUuid& id)
{
    const int idx = indexOf(id);
    if (idx < 0)
        return false;
    emit roiAboutToBeRemoved(idx);
    m_rois.erase(m_rois.begin() + idx);
    emit roiRemoved(idx);
    return true;
}

bool InspectionPlan::updateRoi(const Roi& roi)
{
    const int idx = indexOf(roi.id);
    if (idx < 0)
        return false;
    if (m_rois[idx] == roi)
        return true;
    m_rois[idx] = roi;
    emit roiChanged(idx);
    return true;
}

void InspectionPlan::clear()
{
    m_rois.clear();
    m_nameCounter = 0;
    emit planReset();
}

QJsonObject InspectionPlan::toJson() const
{
    QJsonArray rois;
    for (const Roi& roi : m_rois)
        rois.append(roiToJson(roi));
    return { { "rois", rois }, { "nameCounter", m_nameCounter } };
}

bool InspectionPlan::fromJson(const QJsonObject& obj)
{
    if (!obj.value("rois").isArray())
        return false;
    m_rois.clear();
    for (const QJsonValue& value : obj.value("rois").toArray())
        m_rois.push_back(roiFromJson(value.toObject()));
    m_nameCounter = obj.value("nameCounter").toInt(count());
    emit planReset();
    return true;
}

} // namespace xri
