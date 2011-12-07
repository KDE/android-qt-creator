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

#ifndef MACROSPLUGIN_MACROOPTIONSPAGE_H
#define MACROSPLUGIN_MACROOPTIONSPAGE_H

#include "macros_global.h"

#include <coreplugin/dialogs/ioptionspage.h>

namespace Macros {
namespace Internal {

class MacroOptionsWidget;

class MACROS_EXPORT MacroOptionsPage : public Core::IOptionsPage
{
    Q_OBJECT

public:
    MacroOptionsPage(QObject *parent = 0);
    ~MacroOptionsPage();

    // IOptionsPage implementation
    QString id() const;
    QString displayName() const;
    QString category() const;
    QString displayCategory() const;
    QIcon categoryIcon() const;
    QWidget *createPage(QWidget *parent);

    void apply();
    void finish();

private:
    Internal::MacroOptionsWidget *m_widget;
};

} // namespace Internal
} // namespace Macros

#endif // MACROSPLUGIN_MACROOPTIONSPAGE_H
