#include "pages/ViewerPage.h"

#include "widgets/ImageView.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace xri {

ViewerPage::ViewerPage(InspectionSession* session, QWidget* parent)
    : QWidget(parent)
{
    auto* fitButton = new QPushButton(tr("Fit"));
    fitButton->setObjectName("fitButton");
    auto* zoomInButton = new QPushButton(tr("Zoom +"));
    zoomInButton->setObjectName("zoomInButton");
    auto* zoomOutButton = new QPushButton(tr("Zoom −"));
    zoomOutButton->setObjectName("zoomOutButton");
    auto* zoomLabel = new QLabel(tr("Zoom: —"));
    zoomLabel->setObjectName("zoomLabel");

    auto* toolbar = new QHBoxLayout;
    toolbar->addWidget(fitButton);
    toolbar->addWidget(zoomInButton);
    toolbar->addWidget(zoomOutButton);
    toolbar->addWidget(zoomLabel);
    toolbar->addStretch();

    m_view = new ImageView;
    m_view->setObjectName("viewerView");

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(toolbar);
    layout->addWidget(m_view, 1);

    connect(fitButton, &QPushButton::clicked, m_view, &ImageView::fitToWindow);
    connect(zoomInButton, &QPushButton::clicked, m_view, &ImageView::zoomIn);
    connect(zoomOutButton, &QPushButton::clicked, m_view, &ImageView::zoomOut);
    connect(m_view, &ImageView::zoomChanged, zoomLabel, [zoomLabel](double factor) {
        zoomLabel->setText(tr("Zoom: %1%").arg(qRound(factor * 100)));
    });
    connect(m_view, &ImageView::pixelHovered, this, [this](const QPoint& pos, int value) {
        emit statusMessage(tr("Pixel (%1, %2) = %3").arg(pos.x()).arg(pos.y()).arg(value));
    });

    connect(session, &InspectionSession::scanChanged, this,
            [this, session] { m_view->setImage(session->scanImage()); });
}

} // namespace xri
