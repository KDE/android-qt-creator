/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "formeditorwidget.h"
#include "qmldesignerplugin.h"
#include "designersettings.h"

#include <QWheelEvent>
#include <cmath>
#include <QCoreApplication>
#include <QPushButton>
#include <QFile>
#include <QVBoxLayout>
#include <QActionGroup>
#include <QGraphicsView>
#include <toolbox.h>
#include <zoomaction.h>
#include <formeditorgraphicsview.h>
#include <formeditorscene.h>
#include <formeditorview.h>

namespace QmlDesigner {

FormEditorWidget::FormEditorWidget(FormEditorView *view)
    : QWidget(),
    m_formEditorView(view)
{
    QFile file(":/qmldesigner/formeditorstylesheet.css");
    file.open(QFile::ReadOnly);
    QString styleSheet = QLatin1String(file.readAll());
    setStyleSheet(styleSheet);

    QVBoxLayout *fillLayout = new QVBoxLayout(this);
    fillLayout->setMargin(0);
    fillLayout->setSpacing(0);
    setLayout(fillLayout);

    QList<QAction*> upperActions;

    m_toolActionGroup = new QActionGroup(this);

    m_transformToolAction = m_toolActionGroup->addAction("Transform Tool (Press Key Q)");
    m_transformToolAction->setShortcut(Qt::Key_Q);
    m_transformToolAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    m_transformToolAction->setCheckable(true);
    m_transformToolAction->setChecked(true);
    m_transformToolAction->setIcon(QPixmap(":/icon/tool/transform.png"));
    connect(m_transformToolAction.data(), SIGNAL(triggered(bool)), SLOT(changeTransformTool(bool)));

    m_anchorToolAction = m_toolActionGroup->addAction("Anchor Tool (Press Key W)");
    m_anchorToolAction->setShortcut(Qt::Key_W);
    m_anchorToolAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    m_anchorToolAction->setCheckable(true);
    m_anchorToolAction->setIcon(QPixmap(":/icon/tool/anchor.png"));
    connect(m_anchorToolAction.data(), SIGNAL(triggered(bool)), SLOT(changeAnchorTool(bool)));

//    addActions(m_toolActionGroup->actions());
//    upperActions.append(m_toolActionGroup->actions());

    QActionGroup *layoutActionGroup = new QActionGroup(this);
    layoutActionGroup->setExclusive(false);
    m_snappingAction = layoutActionGroup->addAction(tr("Snap to guides (E)"));
    m_snappingAction->setShortcut(Qt::Key_E);
    m_snappingAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    m_snappingAction->setCheckable(true);
    m_snappingAction->setChecked(true);
    m_snappingAction->setIcon(QPixmap(":/icon/layout/snapping.png"));

    m_snappingAndAnchoringAction = layoutActionGroup->addAction("Toogle Snapping And Anchoring (Press Key R)");
    m_snappingAndAnchoringAction->setShortcut(Qt::Key_R);
    m_snappingAndAnchoringAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    m_snappingAndAnchoringAction->setCheckable(true);
    m_snappingAndAnchoringAction->setChecked(false);
    m_snappingAndAnchoringAction->setEnabled(false);
    m_snappingAndAnchoringAction->setVisible(false);
    m_snappingAndAnchoringAction->setIcon(QPixmap(":/icon/layout/snapping_and_anchoring.png"));

    addActions(layoutActionGroup->actions());
    upperActions.append(layoutActionGroup->actions());

    QAction *separatorAction = new QAction(this);
    separatorAction->setSeparator(true);
    addAction(separatorAction);
    upperActions.append(separatorAction);

    m_showBoundingRectAction = new QAction(tr("Show bounding rectangles (A)"), this);
    m_showBoundingRectAction->setShortcut(Qt::Key_A);
    m_showBoundingRectAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    m_showBoundingRectAction->setCheckable(true);
    m_showBoundingRectAction->setChecked(true);
    m_showBoundingRectAction->setIcon(QPixmap(":/icon/layout/boundingrect.png"));

    addAction(m_showBoundingRectAction.data());
    upperActions.append(m_showBoundingRectAction.data());

    m_selectOnlyContentItemsAction = new QAction(tr("Only select items with content (S)"), this);
    m_selectOnlyContentItemsAction->setShortcut(Qt::Key_S);
    m_selectOnlyContentItemsAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    m_selectOnlyContentItemsAction->setCheckable(true);
    m_selectOnlyContentItemsAction->setChecked(false);
    m_selectOnlyContentItemsAction->setIcon(QPixmap(":/icon/selection/selectonlycontentitems.png"));

    addAction(m_selectOnlyContentItemsAction.data());
    upperActions.append(m_selectOnlyContentItemsAction.data());


    m_toolBox = new ToolBox(this);
    fillLayout->addWidget(m_toolBox.data());
    m_toolBox->setLeftSideActions(upperActions);

    m_graphicsView = new FormEditorGraphicsView(this);
    fillLayout->addWidget(m_graphicsView.data());

    {
        QFile file(":/qmldesigner/scrollbar.css");
        file.open(QFile::ReadOnly);
        m_graphicsView.data()->setStyleSheet(file.readAll());
    }

    QList<QAction*> lowerActions;

    m_zoomAction = new ZoomAction(m_toolActionGroup.data());
    connect(m_zoomAction.data(), SIGNAL(zoomLevelChanged(double)), SLOT(setZoomLevel(double)));
    addAction(m_zoomAction.data());
    upperActions.append(m_zoomAction.data());
    m_toolBox->addRightSideAction(m_zoomAction.data());
}

void FormEditorWidget::enterEvent(QEvent *event)
{
    m_graphicsView->setFocus();
    QWidget::enterEvent(event);
}

void FormEditorWidget::changeTransformTool(bool checked)
{
    if (checked)

        m_formEditorView->changeToTransformTools();
}

void FormEditorWidget::changeAnchorTool(bool checked)
{
    if (checked && m_formEditorView->currentState().isBaseState())
        m_formEditorView->changeToAnchorTool();
}

void FormEditorWidget::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers().testFlag(Qt::ControlModifier)) {
        if (event->delta() > 0) {
            zoomAction()->zoomOut();
        } else {
            zoomAction()->zoomIn();
        }

        event->accept();
    } else {
        QWidget::wheelEvent(event);
    }

}

ZoomAction *FormEditorWidget::zoomAction() const
{
    return m_zoomAction.data();
}

QAction *FormEditorWidget::anchorToolAction() const
{
    return m_anchorToolAction.data();
}

QAction *FormEditorWidget::transformToolAction() const
{
    return m_transformToolAction.data();
}

QAction *FormEditorWidget::showBoundingRectAction() const
{
    return m_showBoundingRectAction.data();
}

QAction *FormEditorWidget::selectOnlyContentItemsAction() const
{
    return m_selectOnlyContentItemsAction.data();
}

QAction *FormEditorWidget::snappingAction() const
{
    return m_snappingAction.data();
}

QAction *FormEditorWidget::snappingAndAnchoringAction() const
{
    return m_snappingAndAnchoringAction.data();
}

void FormEditorWidget::setZoomLevel(double zoomLevel)
{
    m_graphicsView->resetTransform();

    m_graphicsView->scale(zoomLevel, zoomLevel);
}

void FormEditorWidget::setScene(FormEditorScene *scene)
{
    m_graphicsView->setScene(scene);
}

QActionGroup *FormEditorWidget::toolActionGroup() const
{
    return m_toolActionGroup.data();
}

ToolBox *FormEditorWidget::toolBox() const
{
     return m_toolBox.data();
}

double FormEditorWidget::spacing() const
{
    DesignerSettings settings = Internal::BauhausPlugin::pluginInstance()->settings();
    return settings.itemSpacing;
}

double FormEditorWidget::margins() const
{
    DesignerSettings settings = Internal::BauhausPlugin::pluginInstance()->settings();
    return settings.snapMargin;
}

void FormEditorWidget::setFeedbackNode(const QmlItemNode &node)
{
    m_graphicsView->setFeedbackNode(node);
}

QString FormEditorWidget::contextHelpId() const
{
    if (!m_formEditorView)
        return QString();

    QList<ModelNode> nodes = m_formEditorView->selectedModelNodes();
    QString helpId;
    if (!nodes.isEmpty()) {
        helpId = nodes.first().type();
        helpId.replace("Qt/", "QML.");
    }

    return helpId;
}

}


