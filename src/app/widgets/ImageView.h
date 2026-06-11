#pragma once

#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QImage>

namespace xri {

// Zoom/pan viewer for the scan image. Scene coordinates equal image pixel
// coordinates (the pixmap item sits at the origin).
class ImageView : public QGraphicsView
{
    Q_OBJECT
public:
    explicit ImageView(QWidget* parent = nullptr);

    void setImage(const QImage& image);
    bool hasImage() const { return !m_image.isNull(); }
    const QImage& image() const { return m_image; }
    QRectF imageRect() const { return QRectF(QPointF(0, 0), m_image.size()); }

public slots:
    void fitToWindow();
    void zoomIn();
    void zoomOut();

signals:
    void pixelHovered(const QPoint& pos, int value);
    void zoomChanged(double factor);

protected:
    void wheelEvent(QWheelEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

    QGraphicsScene* imageScene() { return &m_scene; }

private:
    void applyZoom(double factor, bool anchorUnderMouse);
    double currentScale() const;

    QGraphicsScene m_scene;
    QGraphicsPixmapItem* m_pixmapItem;
    QImage m_image;
    bool m_fitOnResize = true;
};

} // namespace xri
