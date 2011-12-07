/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nicolas Arnaud-Cormos.
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

#include "macrooptionspage.h"

#include "macromanager.h"
#include "macrosconstants.h"
#include "macrooptionswidget.h"

#include <texteditor/texteditorconstants.h>

#include <QtCore/QCoreApplication>
#include <QtGui/QWidget>
#include <QtGui/QIcon>

using namespace Macros;
using namespace Macros::Internal;


MacroOptionsPage::MacroOptionsPage(QObject *parent)
    : Core::IOptionsPage(parent)
{
}

MacroOptionsPage::~MacroOptionsPage()
{
}

QString MacroOptionsPage::id() const
{
    return Constants::M_OPTIONS_PAGE;
}

QString MacroOptionsPage::displayName() const
{
    return QCoreApplication::translate("Macros", Constants::M_OPTIONS_TR_PAGE);
}

QString MacroOptionsPage::category() const
{
    return TextEditor::Constants::TEXT_EDITOR_SETTINGS_CATEGORY;
}

QString MacroOptionsPage::displayCategory() const
{
    return QCoreApplication::translate("TextEditor",
                                       TextEditor::Constants::TEXT_EDITOR_SETTINGS_TR_CATEGORY);
}

QIcon MacroOptionsPage::categoryIcon() const
{
    return QIcon();
}

QWidget *MacroOptionsPage::createPage(QWidget *parent)
{
    m_widget = new MacroOptionsWidget(parent);
    return m_widget;
}

void MacroOptionsPage::apply()
{
    if (m_widget)
        m_widget->apply();
}

void MacroOptionsPage::finish()
{
    // Nothing to do
}
