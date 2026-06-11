#pragma once

#include "core/Roi.h"

#include <QJsonObject>
#include <QObject>

#include <vector>

namespace xri {

// Owns the list of inspection boxes. The single source of truth for ROIs;
// views (graphics items, list models) observe its signals.
class InspectionPlan : public QObject
{
    Q_OBJECT
public:
    explicit InspectionPlan(QObject* parent = nullptr);

    int count() const { return static_cast<int>(m_rois.size()); }
    const std::vector<Roi>& rois() const { return m_rois; }
    const Roi* roiById(const QUuid& id) const;
    const Roi* roiAt(int index) const;
    int indexOf(const QUuid& id) const;

    QUuid addRoi(const QRectF& rect);
    bool removeRoi(const QUuid& id);
    bool updateRoi(const Roi& roi); // matched by id
    void clear();

    QJsonObject toJson() const;
    bool fromJson(const QJsonObject& obj);

signals:
    // The aboutTo* signals fire before the container mutates so that list
    // models can call beginInsertRows/beginRemoveRows at the right moment.
    void roiAboutToBeAdded(int index);
    void roiAdded(int index);
    void roiAboutToBeRemoved(int index);
    void roiRemoved(int index);
    void roiChanged(int index);
    void planReset();

private:
    std::vector<Roi> m_rois;
    int m_nameCounter = 0;
};

} // namespace xri
