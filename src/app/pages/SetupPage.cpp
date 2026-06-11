#include "pages/SetupPage.h"

#include "widgets/RoiEditorView.h"
#include "widgets/RoiGraphicsItem.h"

#include <QButtonGroup>
#include <QHBoxLayout>
#include <QLabel>
#include <QListView>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>

namespace xri {

SetupPage::SetupPage(InspectionSession* session, QWidget* parent)
    : QWidget(parent)
    , m_session(session)
{
    m_selectModeButton = new QToolButton;
    m_selectModeButton->setObjectName("selectModeButton");
    m_selectModeButton->setText(tr("Select / Move"));
    m_selectModeButton->setCheckable(true);
    m_selectModeButton->setChecked(true);

    m_drawModeButton = new QToolButton;
    m_drawModeButton->setObjectName("drawModeButton");
    m_drawModeButton->setText(tr("Draw ROI"));
    m_drawModeButton->setCheckable(true);

    auto* modeGroup = new QButtonGroup(this);
    modeGroup->addButton(m_selectModeButton);
    modeGroup->addButton(m_drawModeButton);
    modeGroup->setExclusive(true);

    auto* toolbar = new QHBoxLayout;
    toolbar->addWidget(m_selectModeButton);
    toolbar->addWidget(m_drawModeButton);
    toolbar->addStretch();

    m_editor = new RoiEditorView;
    m_editor->setObjectName("roiEditorView");

    auto* editorLayout = new QVBoxLayout;
    editorLayout->addLayout(toolbar);
    editorLayout->addWidget(m_editor, 1);

    m_roiList = new QListView;
    m_roiList->setObjectName("roiList");
    m_roiList->setModel(session->roiModel());
    m_roiList->setEditTriggers(QListView::DoubleClicked | QListView::EditKeyPressed);

    auto* deleteButton = new QPushButton(tr("Delete ROI"));
    deleteButton->setObjectName("deleteRoiButton");

    auto* hint = new QLabel(
        tr("Draw inspection boxes on the scan.\nDouble-click a name to rename."));
    hint->setWordWrap(true);

    auto* sideLayout = new QVBoxLayout;
    sideLayout->addWidget(new QLabel(tr("Inspection boxes")));
    sideLayout->addWidget(m_roiList, 1);
    sideLayout->addWidget(deleteButton);
    sideLayout->addWidget(hint);

    auto* sidePanel = new QWidget;
    sidePanel->setLayout(sideLayout);
    sidePanel->setFixedWidth(240);

    auto* layout = new QHBoxLayout(this);
    layout->addLayout(editorLayout, 1);
    layout->addWidget(sidePanel);

    connect(m_selectModeButton, &QToolButton::toggled, this, [this](bool checked) {
        if (checked)
            m_editor->setMode(RoiEditorView::Mode::Select);
    });
    connect(m_drawModeButton, &QToolButton::toggled, this, [this](bool checked) {
        if (checked)
            m_editor->setMode(RoiEditorView::Mode::Draw);
    });

    connect(m_editor, &RoiEditorView::roiDrawn, this, [this](const QRectF& rect) {
        m_session->plan()->addRoi(rect);
        emit statusMessage(tr("Added %1").arg(m_session->plan()->rois().back().name));
    });

    connect(deleteButton, &QPushButton::clicked, this, &SetupPage::deleteSelectedRoi);

    InspectionPlan* plan = m_session->plan();
    connect(plan, &InspectionPlan::roiAdded, this, &SetupPage::addItemForRoi);
    connect(plan, &InspectionPlan::roiRemoved, this, [this] { rebuildItems(); });
    connect(plan, &InspectionPlan::planReset, this, [this] { rebuildItems(); });
    connect(plan, &InspectionPlan::roiChanged, this, [this, plan](int index) {
        const Roi* roi = plan->roiAt(index);
        if (RoiGraphicsItem* item = roi ? m_items.value(roi->id) : nullptr) {
            item->setRoiRect(roi->rect);
            item->setRoiName(roi->name);
        }
    });

    connect(session, &InspectionSession::scanChanged, this, [this, session] {
        m_editor->setImage(session->scanImage());
        rebuildItems(); // setImage resets nothing, but keeps items above the new pixmap
    });
}

void SetupPage::addItemForRoi(int index)
{
    const Roi* roi = m_session->plan()->roiAt(index);
    if (!roi || m_items.contains(roi->id))
        return;
    auto* item = new RoiGraphicsItem(roi->id, roi->rect, roi->name);
    m_editor->scene()->addItem(item);
    m_items.insert(roi->id, item);

    connect(item, &RoiGraphicsItem::roiEdited, this,
            [this](const QUuid& id, const QRectF& rect) {
                if (const Roi* existing = m_session->plan()->roiById(id)) {
                    Roi updated = *existing;
                    updated.rect = rect;
                    m_session->plan()->updateRoi(updated);
                }
            });
}

void SetupPage::rebuildItems()
{
    for (RoiGraphicsItem* item : std::as_const(m_items)) {
        m_editor->scene()->removeItem(item);
        delete item;
    }
    m_items.clear();
    for (int i = 0; i < m_session->plan()->count(); ++i)
        addItemForRoi(i);
}

void SetupPage::deleteSelectedRoi()
{
    const QModelIndex current = m_roiList->currentIndex();
    if (!current.isValid())
        return;
    const QUuid id = m_session->roiModel()
                         ->data(current, RoiListModel::IdRole)
                         .toUuid();
    const QString name = m_session->plan()->roiById(id) ? m_session->plan()->roiById(id)->name
                                                        : QString();
    if (m_session->plan()->removeRoi(id))
        emit statusMessage(tr("Removed %1").arg(name));
}

} // namespace xri
