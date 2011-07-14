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

#ifndef FINDPLUGIN_H
#define FINDPLUGIN_H

#include "find_global.h"
#include "textfindconstants.h"

#include <extensionsystem/iplugin.h>

#include <QtGui/QTextDocument>

QT_BEGIN_NAMESPACE
class QStringListModel;
QT_END_NAMESPACE

namespace Find {
class IFindFilter;
struct FindPluginPrivate;

namespace Internal {
class FindToolBar;
class FindToolWindow;
class CurrentDocumentFind;
} // namespace Internal

class FIND_EXPORT FindPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT

public:
    FindPlugin();
    virtual ~FindPlugin();
    static FindPlugin *instance();

    enum FindDirection {
        FindForward,
        FindBackward
    };

    // IPlugin
    bool initialize(const QStringList &arguments, QString *error_message);
    void extensionsInitialized();
    ShutdownFlag aboutToShutdown();

    Find::FindFlags findFlags() const;
    bool hasFindFlag(Find::FindFlag flag);
    void updateFindCompletion(const QString &text);
    void updateReplaceCompletion(const QString &text);
    QStringListModel *findCompletionModel() const;
    QStringListModel *replaceCompletionModel() const;
    void setUseFakeVim(bool on);
    void openFindToolBar(FindDirection direction);

public slots:
    void setCaseSensitive(bool sensitive);
    void setWholeWord(bool wholeOnly);
    void setBackward(bool backward);
    void setRegularExpression(bool regExp);

signals:
    void findFlagsChanged();

private slots:
    void filterChanged();
    void openFindFilter();

private:
    void setFindFlag(Find::FindFlag flag, bool enabled);
    void updateCompletion(const QString &text, QStringList &completions, QStringListModel *model);
    void setupMenu();
    void setupFilterMenuItems();
    void writeSettings();
    void readSettings();

    //variables
    FindPluginPrivate *d;
};

} // namespace Find

#endif // FINDPLUGIN_H
