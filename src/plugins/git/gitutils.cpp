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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "gitutils.h"

#include <QtCore/QDebug>
#include <QtCore/QStringList>
#include <QtGui/QInputDialog>
#include <QtGui/QLineEdit>

namespace Git {
namespace Internal {

QDebug operator<<(QDebug d, const Stash &s)
{
    QDebug nospace = d.nospace();
    nospace << "name=" << s.name << " branch=" << s.branch << " message=" << s.message;
    return d;
}

void Stash::clear()
{
    name.clear();
    branch.clear();
    message.clear();
}

/* Parse a stash line in its 2 manifestations (with message/without message containing
 * <base_sha1>+subject):
\code
stash@{1}: WIP on <branch>: <base_sha1> <subject_base_sha1>
stash@{2}: On <branch>: <message>
\endcode */

bool Stash::parseStashLine(const QString &l)
{
    const QChar colon = QLatin1Char(':');
    const int branchPos = l.indexOf(colon);
    if (branchPos < 0)
        return false;
    const int messagePos = l.indexOf(colon, branchPos + 1);
    if (messagePos < 0)
        return false;
    // Branch spec
    const int onIndex = l.indexOf(QLatin1String("on "), branchPos + 2, Qt::CaseInsensitive);
    if (onIndex == -1 || onIndex >= messagePos)
        return false;
    // Happy!
    name = l.left(branchPos);
    branch = l.mid(onIndex + 3, messagePos - onIndex - 3);
    message = l.mid(messagePos + 2); // skip blank
    return true;
}

// Make QInputDialog  play nicely, widen it a bit.
bool inputText(QWidget *parent, const QString &title, const QString &prompt, QString *s)
{
    QInputDialog dialog(parent);
    dialog.setWindowFlags(dialog.windowFlags() & ~Qt::WindowContextHelpButtonHint);
    dialog.setWindowTitle(title);
    dialog.setLabelText(prompt);
    dialog.setTextValue(*s);
    // Nasty hack:
    if (QLineEdit *le = qFindChild<QLineEdit*>(&dialog))
        le->setMinimumWidth(500);
    if (dialog.exec() != QDialog::Accepted)
        return false;
    *s = dialog.textValue();
    return true;
}
} // namespace Internal
} // namespace Git
