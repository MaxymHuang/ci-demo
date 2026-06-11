#pragma once

#include <QGraphicsObject>
#include <QUuid>

namespace xri {

// Movable, resizable inspection box on the scene. Position/size live in image
// pixel coordinates. The item never owns ROI truth: user edits are reported
// via roiEdited() and written back to the InspectionPlan by the SetupPage.
class RoiGraphicsItem : public QGraphicsObject
{
    Q_OBJECT
public:
    RoiGraphicsItem(const QUuid& id, const QRectF& rect, const QString& name,
                    QGraphicsItem* parent = nullptr);

    QUuid roiId() const { return m_id; }
    QRectF roiRect() const { return QRectF(pos(), m_size); }

    // Programmatic updates from the model; do not re-emit roiEdited.
    void setRoiRect(const QRectF& rect);
    void setRoiName(const QString& name);

    void resizeTo(const QSizeF& size); // called by the handle while dragging

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
               QWidget* widget) override;

signals:
    void roiEdited(const QUuid& id, const QRectF& newRect);

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

private:
    void updateHandlePos();

    QUuid m_id;
    QSizeF m_size;
    QString m_name;
    QGraphicsRectItem* m_handle;
    bool m_updating = false;
};

} // namespace xri
