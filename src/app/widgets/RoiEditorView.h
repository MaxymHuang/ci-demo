#pragma once

#include "widgets/ImageView.h"

#include <QGraphicsRectItem>

namespace xri {

// ImageView with an ROI drawing mode. In Select mode mouse events go to the
// items (move/resize); in Draw mode the view rubber-bands a new box and emits
// roiDrawn() — the page decides what to do with it.
class RoiEditorView : public ImageView
{
    Q_OBJECT
public:
    enum class Mode { Select, Draw };

    explicit RoiEditorView(QWidget* parent = nullptr);

    Mode mode() const { return m_mode; }
    void setMode(Mode mode);

    QGraphicsScene* scene() { return imageScene(); }

signals:
    void roiDrawn(const QRectF& rectImageCoords);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    QRectF clampedToImage(const QRectF& rect) const;

    Mode m_mode = Mode::Select;
    QPointF m_dragAnchor;
    QGraphicsRectItem* m_rubberBand = nullptr;
};

} // namespace xri
