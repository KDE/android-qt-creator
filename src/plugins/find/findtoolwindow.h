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

#ifndef FINDTOOLWINDOW_H
#define FINDTOOLWINDOW_H

#include "ui_finddialog.h"
#include "ifindfilter.h"

#include <QtCore/QList>

QT_FORWARD_DECLARE_CLASS(QCompleter)

namespace Find {
class FindPlugin;

namespace Internal {

class FindToolWindow : public QDialog
{
    Q_OBJECT

public:
    explicit FindToolWindow(FindPlugin *plugin);
    ~FindToolWindow();

    void setFindFilters(const QList<IFindFilter *> &filters);

    void setFindText(const QString &text);
    void open(IFindFilter *filter);
    void readSettings();
    void writeSettings();

protected:
    bool eventFilter(QObject *obj, QEvent *event);

private slots:
    void search();
    void replace();
    void cancelSearch();
    void setCurrentFilter(int index);
    void updateButtonStates();

private:
    void acceptAndGetParameters(QString *term, IFindFilter **filter);

    Ui::FindDialog m_ui;
    FindPlugin *m_plugin;
    QList<IFindFilter *> m_filters;
    QCompleter *m_findCompleter;
    QWidgetList m_configWidgets;
    IFindFilter *m_currentFilter;
    QWidget *m_configWidget;
};

} // namespace Internal
} // namespace Find

#endif // FINDTOOLWINDOW_H
