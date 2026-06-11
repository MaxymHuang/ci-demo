#include "widgets/RoiGraphicsItem.h"

#include <QCursor>
#include <QGraphicsRectItem>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QPen>

#include <algorithm>

namespace xri {

namespace {

constexpr double kMinRoiSize = 8.0;
constexpr double kHandleSize = 9.0;

// Bottom-right resize grip.
class HandleItem : public QGraphicsRectItem
{
public:
    explicit HandleItem(RoiGraphicsItem* owner)
        : QGraphicsRectItem(QRectF(-kHandleSize / 2, -kHandleSize / 2, kHandleSize, kHandleSize),
                            owner)
        , m_owner(owner)
    {
        setBrush(QColor(0x58, 0xa6, 0xff));
        setPen(QPen(QColor(0x0d, 0x11, 0x17), 1));
        setCursor(Qt::SizeFDiagCursor);
    }

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override { event->accept(); }

    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override
    {
        const QPointF corner = m_owner->mapFromScene(event->scenePos());
        m_owner->resizeTo(QSizeF(corner.x(), corner.y()));
    }

    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override { event->accept(); }

private:
    RoiGraphicsItem* m_owner;
};

} // namespace

RoiGraphicsItem::RoiGraphicsItem(const QUuid& id, const QRectF& rect, const QString& name,
                                 QGraphicsItem* parent)
    : QGraphicsObject(parent)
    , m_id(id)
    , m_size(rect.size())
    , m_name(name)
{
    setFlags(ItemIsMovable | ItemIsSelectable | ItemSendsGeometryChanges);
    setPos(rect.topLeft());
    m_handle = new HandleItem(this);
    updateHandlePos();
}

void RoiGraphicsItem::setRoiRect(const QRectF& rect)
{
    if (roiRect() == rect)
        return;
    m_updating = true;
    prepareGeometryChange();
    setPos(rect.topLeft());
    m_size = rect.size();
    updateHandlePos();
    m_updating = false;
    update();
}

void RoiGraphicsItem::setRoiName(const QString& name)
{
    if (m_name == name)
        return;
    m_name = name;
    update();
}

void RoiGraphicsItem::resizeTo(const QSizeF& size)
{
    const QSizeF clamped(std::max(size.width(), kMinRoiSize),
                         std::max(size.height(), kMinRoiSize));
    if (clamped == m_size)
        return;
    prepareGeometryChange();
    m_size = clamped;
    updateHandlePos();
    update();
    if (!m_updating)
        emit roiEdited(m_id, roiRect());
}

QRectF RoiGraphicsItem::boundingRect() const
{
    return QRectF(QPointF(0, 0), m_size).adjusted(-2, -2, 2, 2);
}

void RoiGraphicsItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    const QColor color =
        isSelected() ? QColor(0xff, 0xd3, 0x3d) : QColor(0x58, 0xa6, 0xff);
    QPen pen(color, 2);
    pen.setCosmetic(true);
    painter->setPen(pen);
    painter->setBrush(QColor(color.red(), color.green(), color.blue(), 28));
    painter->drawRect(QRectF(QPointF(0, 0), m_size));

    QFont font = painter->font();
    font.setPointSizeF(8.5);
    font.setBold(true);
    painter->setFont(font);
    painter->setPen(color);
    painter->drawText(QPointF(3, 12), m_name);
}

QVariant RoiGraphicsItem::itemChange(GraphicsItemChange change, const QVariant& value)
{
    if (change == ItemPositionHasChanged && !m_updating)
        emit roiEdited(m_id, roiRect());
    return QGraphicsObject::itemChange(change, value);
}

void RoiGraphicsItem::updateHandlePos()
{
    m_handle->setPos(m_size.width(), m_size.height());
}

} // namespace xri
