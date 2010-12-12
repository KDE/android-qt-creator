/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef LOCATORWIDGET_H
#define LOCATORWIDGET_H

#include "locatorplugin.h"

#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE
class QAction;
class QLabel;
class QLineEdit;
class QMenu;
class QTreeView;
QT_END_NAMESPACE

namespace Utils {
    class FilterLineEdit;
}

namespace Locator {
namespace Internal {

class LocatorModel;
class CompletionList;

class LocatorWidget
  : public QWidget
{
    Q_OBJECT

public:
    explicit LocatorWidget(LocatorPlugin *qop);

    void updateFilterList();

    void show(const QString &text, int selectionStart = -1, int selectionLength = 0);

private slots:
    void showPopup();
    void showPopupNow();
    void acceptCurrentEntry();
    void filterSelected();
    void showConfigureDialog();
    void updateEntries();

private:
    bool eventFilter(QObject *obj, QEvent *event);

    void showEvent(QShowEvent *e);

    void showCompletionList();
    void updateCompletionList(const QString &text);
    QList<ILocatorFilter*> filtersFor(const QString &text, QString &searchText);

    LocatorPlugin *m_locatorPlugin;
    LocatorModel *m_locatorModel;

    CompletionList *m_completionList;
    QMenu *m_filterMenu;
    QAction *m_refreshAction;
    QAction *m_configureAction;
    Utils::FilterLineEdit *m_fileLineEdit;
    QTimer *m_showPopupTimer;
    QFutureWatcher<FilterEntry> *m_entriesWatcher;
    QMap<QString, QAction *> m_filterActionMap;
};

} // namespace Internal
} // namespace Locator

#endif // LOCATORWIDGET_H
