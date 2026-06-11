#include "widgets/ImageView.h"

#include <QMouseEvent>
#include <QWheelEvent>

#include <algorithm>

namespace xri {

namespace {
constexpr double kMinScale = 0.1;
constexpr double kMaxScale = 20.0;
}

ImageView::ImageView(QWidget* parent)
    : QGraphicsView(parent)
{
    m_pixmapItem = m_scene.addPixmap(QPixmap());
    setScene(&m_scene);
    setDragMode(QGraphicsView::ScrollHandDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setRenderHint(QPainter::SmoothPixmapTransform);
    setMouseTracking(true);
    setBackgroundBrush(QColor(0x14, 0x17, 0x1c));
}

void ImageView::setImage(const QImage& image)
{
    m_image = image;
    m_pixmapItem->setPixmap(QPixmap::fromImage(image));
    m_scene.setSceneRect(imageRect());
    m_fitOnResize = true;
    fitToWindow();
}

void ImageView::fitToWindow()
{
    if (!hasImage())
        return;
    fitInView(imageRect(), Qt::KeepAspectRatio);
    m_fitOnResize = true;
    emit zoomChanged(currentScale());
}

void ImageView::zoomIn()
{
    applyZoom(1.25, false);
}

void ImageView::zoomOut()
{
    applyZoom(1.0 / 1.25, false);
}

void ImageView::wheelEvent(QWheelEvent* event)
{
    if (!hasImage()) {
        event->ignore();
        return;
    }
    const double factor = event->angleDelta().y() > 0 ? 1.15 : 1.0 / 1.15;
    applyZoom(factor, true);
    event->accept();
}

void ImageView::mouseMoveEvent(QMouseEvent* event)
{
    QGraphicsView::mouseMoveEvent(event);
    if (!hasImage())
        return;
    const QPointF scenePos = mapToScene(event->pos());
    const QPoint pixel(int(scenePos.x()), int(scenePos.y()));
    if (imageRect().toRect().contains(pixel))
        emit pixelHovered(pixel, qGray(m_image.pixel(pixel)));
}

void ImageView::resizeEvent(QResizeEvent* event)
{
    QGraphicsView::resizeEvent(event);
    if (m_fitOnResize && hasImage())
        fitInView(imageRect(), Qt::KeepAspectRatio);
}

void ImageView::applyZoom(double factor, bool anchorUnderMouse)
{
    const double clamped = std::clamp(currentScale() * factor, kMinScale, kMaxScale);
    factor = clamped / currentScale();
    setTransformationAnchor(anchorUnderMouse ? QGraphicsView::AnchorUnderMouse
                                             : QGraphicsView::AnchorViewCenter);
    scale(factor, factor);
    m_fitOnResize = false;
    emit zoomChanged(currentScale());
}

double ImageView::currentScale() const
{
    return transform().m11();
}

} // namespace xri
