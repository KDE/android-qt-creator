/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "editmode.h"
#include "editormanager.h"
#include "coreconstants.h"
#include "modemanager.h"
#include "minisplitter.h"
#include "findplaceholder.h"
#include "outputpane.h"
#include "navigationwidget.h"
#include "rightpane.h"
#include "ieditor.h"
#include "idocument.h"

#include <QLatin1String>
#include <QHBoxLayout>
#include <QWidget>
#include <QSplitter>
#include <QIcon>

using namespace Core;
using namespace Core::Internal;

EditMode::EditMode() :
    m_splitter(new MiniSplitter),
    m_rightSplitWidgetLayout(new QVBoxLayout)
{
    m_editorManager = EditorManager::instance();
    setObjectName(QLatin1String("EditMode"));
    setDisplayName(tr("Edit"));
    setIcon(QIcon(QLatin1String(":/fancyactionbar/images/mode_Edit.png")));
    setPriority(Constants::P_MODE_EDIT);
    setId(Constants::MODE_EDIT);
    setType(Constants::MODE_EDIT_TYPE);

    m_rightSplitWidgetLayout->setSpacing(0);
    m_rightSplitWidgetLayout->setMargin(0);
    QWidget *rightSplitWidget = new QWidget;
    rightSplitWidget->setLayout(m_rightSplitWidgetLayout);
    m_rightSplitWidgetLayout->insertWidget(0, new Core::EditorManagerPlaceHolder(this));

    MiniSplitter *rightPaneSplitter = new MiniSplitter;
    rightPaneSplitter->insertWidget(0, rightSplitWidget);
    rightPaneSplitter->insertWidget(1, new RightPanePlaceHolder(this));
    rightPaneSplitter->setStretchFactor(0, 1);
    rightPaneSplitter->setStretchFactor(1, 0);

    MiniSplitter *splitter = new MiniSplitter;
    splitter->setOrientation(Qt::Vertical);
    splitter->insertWidget(0, rightPaneSplitter);
    QWidget *outputPane = new Core::OutputPanePlaceHolder(this, splitter);
    outputPane->setObjectName(QLatin1String("EditModeOutputPanePlaceHolder"));
    splitter->insertWidget(1, outputPane);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 0);

    m_splitter->insertWidget(0, new NavigationWidgetPlaceHolder(this));
    m_splitter->insertWidget(1, splitter);
    m_splitter->setStretchFactor(0, 0);
    m_splitter->setStretchFactor(1, 1);

    connect(ModeManager::instance(), SIGNAL(currentModeChanged(Core::IMode*)),
            this, SLOT(grabEditorManager(Core::IMode*)));
    m_splitter->setFocusProxy(m_editorManager);

    setWidget(m_splitter);
    setContext(Context(Constants::C_EDIT_MODE,
                       Constants::C_EDITORMANAGER,
                       Constants::C_NAVIGATION_PANE));
}

EditMode::~EditMode()
{
    // Make sure the editor manager does not get deleted
    m_editorManager->setParent(0);
    delete m_splitter;
}

void EditMode::grabEditorManager(Core::IMode *mode)
{
    if (mode != this)
        return;

    if (EditorManager::currentEditor())
        EditorManager::currentEditor()->widget()->setFocus();
}
