#include "widgets/RoiEditorView.h"

#include <QMouseEvent>
#include <QPen>

namespace xri {

namespace {
constexpr double kMinDrawSize = 8.0; // ignore accidental click-drags
}

RoiEditorView::RoiEditorView(QWidget* parent)
    : ImageView(parent)
{
    // Items must receive mouse events in Select mode, so no hand-drag here;
    // panning is done via scrollbars and wheel zoom.
    setDragMode(QGraphicsView::NoDrag);
}

void RoiEditorView::setMode(Mode mode)
{
    m_mode = mode;
    setCursor(mode == Mode::Draw ? Qt::CrossCursor : Qt::ArrowCursor);
}

void RoiEditorView::mousePressEvent(QMouseEvent* event)
{
    if (m_mode != Mode::Draw || event->button() != Qt::LeftButton || !hasImage()) {
        ImageView::mousePressEvent(event);
        return;
    }
    m_dragAnchor = mapToScene(event->pos());
    m_rubberBand = imageScene()->addRect(QRectF(m_dragAnchor, QSizeF(0, 0)));
    QPen pen(QColor(0x58, 0xa6, 0xff), 1, Qt::DashLine);
    pen.setCosmetic(true);
    m_rubberBand->setPen(pen);
    event->accept();
}

void RoiEditorView::mouseMoveEvent(QMouseEvent* event)
{
    if (!m_rubberBand) {
        ImageView::mouseMoveEvent(event);
        return;
    }
    m_rubberBand->setRect(
        clampedToImage(QRectF(m_dragAnchor, mapToScene(event->pos())).normalized()));
    event->accept();
}

void RoiEditorView::mouseReleaseEvent(QMouseEvent* event)
{
    if (!m_rubberBand) {
        ImageView::mouseReleaseEvent(event);
        return;
    }
    const QRectF rect = m_rubberBand->rect();
    imageScene()->removeItem(m_rubberBand);
    delete m_rubberBand;
    m_rubberBand = nullptr;

    if (rect.width() >= kMinDrawSize && rect.height() >= kMinDrawSize)
        emit roiDrawn(rect);
    event->accept();
}

QRectF RoiEditorView::clampedToImage(const QRectF& rect) const
{
    return rect.intersected(imageRect());
}

} // namespace xri
