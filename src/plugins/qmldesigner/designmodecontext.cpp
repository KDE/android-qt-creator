/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#include "designmodecontext.h"
#include "qmldesignerconstants.h"
#include "designmodewidget.h"
#include "formeditorwidget.h"
#include "navigatorwidget.h"

namespace QmlDesigner {
namespace Internal {

DesignModeContext::DesignModeContext(QWidget *widget)
  : IContext(widget)
{
    setWidget(widget);
    setContext(Core::Context(Constants::C_QMLDESIGNER, Constants::C_QT_QUICK_TOOLS_MENU));
}

QString DesignModeContext::contextHelpId() const
{
    return qobject_cast<DesignModeWidget *>(m_widget)->contextHelpId();
}


FormEditorContext::FormEditorContext(QWidget *widget)
  : IContext(widget)
{
    setWidget(widget);
    setContext(Core::Context(Constants::C_QMLFORMEDITOR, Constants::C_QT_QUICK_TOOLS_MENU));
}

QString FormEditorContext::contextHelpId() const
{
    return qobject_cast<FormEditorWidget *>(m_widget)->contextHelpId();
}

NavigatorContext::NavigatorContext(QWidget *widget)
  : IContext(widget)
{
    setWidget(widget);
    setContext(Core::Context(Constants::C_QMLNAVIGATOR, Constants::C_QT_QUICK_TOOLS_MENU));
}

QString NavigatorContext::contextHelpId() const
{
    return qobject_cast<NavigatorWidget *>(m_widget)->contextHelpId();
}

}
}

