/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "stateseditorwidget.h"
#include "stateseditormodel.h"
#include "stateseditorview.h"
#include "stateseditorimageprovider.h"

#include <qmlitemnode.h>
#include <invalidargumentexception.h>
#include <invalidqmlsourceexception.h>

#include <QtCore/QFile>
#include <qapplication.h>

#include <QtGui/QBoxLayout>
#include <QtGui/QListWidget>
#include <QtGui/QPushButton>
#include <QtGui/QMessageBox>

#include <QtDeclarative/QDeclarativeView>
#include <QtDeclarative/QDeclarativeContext>
#include <QtDeclarative/QDeclarativeEngine>
#include <QtDeclarative/QDeclarativeItem>

enum {
    debug = false
};

namespace QmlDesigner {

int StatesEditorWidget::currentStateInternalId() const
{
    Q_ASSERT(m_declarativeView->rootObject());
    Q_ASSERT(m_declarativeView->rootObject()->property("currentStateInternalId").isValid());

    return m_declarativeView->rootObject()->property("currentStateInternalId").toInt();
}

void StatesEditorWidget::setCurrentStateInternalId(int internalId)
{
    m_declarativeView->rootObject()->setProperty("currentStateInternalId", internalId);
}

void StatesEditorWidget::setNodeInstanceView(NodeInstanceView *nodeInstanceView)
{
    m_imageProvider->setNodeInstanceView(nodeInstanceView);
}

StatesEditorWidget::StatesEditorWidget(StatesEditorView *statesEditorView, StatesEditorModel *statesEditorModel):
        QWidget(),
    m_declarativeView(new QDeclarativeView(this)),
    m_statesEditorView(statesEditorView),
    m_imageProvider(0)
{
    m_imageProvider = new Internal::StatesEditorImageProvider;
    m_imageProvider->setNodeInstanceView(statesEditorView->nodeInstanceView());
    m_declarativeView->engine()->addImageProvider(
            QLatin1String("qmldesigner_stateseditor"), m_imageProvider);

    m_declarativeView->setAcceptDrops(false);

    QVBoxLayout *layout = new QVBoxLayout(this);
    setMinimumHeight(160);
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addWidget(m_declarativeView.data());

    m_declarativeView->setResizeMode(QDeclarativeView::SizeRootObjectToView);

    m_declarativeView->rootContext()->setContextProperty(QLatin1String("statesEditorModel"), statesEditorModel);
    QColor highlightColor = palette().highlight().color();
    if (0.5*highlightColor.saturationF()+0.75-highlightColor.valueF() < 0)
        highlightColor.setHsvF(highlightColor.hsvHueF(),0.1 + highlightColor.saturationF()*2.0, highlightColor.valueF());
    m_declarativeView->rootContext()->setContextProperty(QLatin1String("highlightColor"), highlightColor);

    // Work around ASSERT in the internal QGraphicsScene that happens when
    // the scene is created + items set dirty in one event loop run (BAUHAUS-459)
    //QApplication::processEvents();

    m_declarativeView->setSource(QUrl("qrc:/stateseditor/stateslist.qml"));

    if (!m_declarativeView->rootObject())
        throw InvalidQmlSourceException(__LINE__, __FUNCTION__, __FILE__);

    m_declarativeView->setFocusPolicy(Qt::ClickFocus);
    QApplication::sendEvent(m_declarativeView->scene(), new QEvent(QEvent::WindowActivate));

    connect(m_declarativeView->rootObject(), SIGNAL(currentStateInternalIdChanged()), statesEditorView, SLOT(synchonizeCurrentStateFromWidget()));
    connect(m_declarativeView->rootObject(), SIGNAL(createNewState()), statesEditorView, SLOT(createNewState()));
    connect(m_declarativeView->rootObject(), SIGNAL(deleteState(int)), statesEditorView, SLOT(removeState(int)));

    setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred));

    setWindowTitle(tr("States", "Title of Editor widget"));
}

StatesEditorWidget::~StatesEditorWidget()
{
}


QSize StatesEditorWidget::sizeHint() const
{
    return QSize(9999, 159);
}

}
