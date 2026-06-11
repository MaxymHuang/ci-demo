#pragma once

#include "core/InspectionSession.h"

#include <QHash>
#include <QUuid>
#include <QWidget>

class QListView;
class QToolButton;

namespace xri {

class RoiEditorView;
class RoiGraphicsItem;

// Inspection setup: draw, move, resize, rename and delete inspection boxes
// on top of the scan.
class SetupPage : public QWidget
{
    Q_OBJECT
public:
    explicit SetupPage(InspectionSession* session, QWidget* parent = nullptr);

signals:
    void statusMessage(const QString& message);

private:
    void addItemForRoi(int index);
    void rebuildItems();
    void deleteSelectedRoi();

    InspectionSession* m_session;
    RoiEditorView* m_editor;
    QListView* m_roiList;
    QToolButton* m_selectModeButton;
    QToolButton* m_drawModeButton;
    QHash<QUuid, RoiGraphicsItem*> m_items;
};

} // namespace xri
