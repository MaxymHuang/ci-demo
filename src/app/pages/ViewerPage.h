#pragma once

#include "core/InspectionSession.h"

#include <QWidget>

namespace xri {

class ImageView;

// Result image viewer with zoom/pan and a pixel readout.
class ViewerPage : public QWidget
{
    Q_OBJECT
public:
    explicit ViewerPage(InspectionSession* session, QWidget* parent = nullptr);

signals:
    void statusMessage(const QString& message);

private:
    ImageView* m_view;
};

} // namespace xri
