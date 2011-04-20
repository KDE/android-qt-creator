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

#ifndef CDBSETTINGSPAGE_H
#define CDBSETTINGSPAGE_H

#include "cdboptions.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include "ui_cdboptionspagewidget.h"

#include <QtGui/QWidget>
#include <QtCore/QPointer>
#include <QtCore/QSharedPointer>

QT_BEGIN_NAMESPACE
class QCheckBox;
QT_END_NAMESPACE

namespace Debugger {
namespace Internal {

// Widget displaying a list of break events for the 'sxe' command
// with a checkbox to enable 'break' and optionally a QLineEdit for
// events with parameters (like 'out:Needle').
class CdbBreakEventWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CdbBreakEventWidget(QWidget *parent = 0);

    void setBreakEvents(const QStringList &l);
    QStringList breakEvents() const;

private:
    QString filterText(int i) const;
    void clear();

    QList<QCheckBox*> m_checkBoxes;
    QList<QLineEdit*> m_lineEdits;
};

class CdbOptionsPageWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CdbOptionsPageWidget(QWidget *parent);

    void setOptions(CdbOptions &o);
    CdbOptions options() const;

    QString searchKeywords() const;

private:
    QStringList symbolPaths() const;
    void setSymbolPaths(const QStringList &s);
    inline QString path() const;

    Ui::CdbOptionsPageWidget m_ui;
    CdbBreakEventWidget *m_breakEventWidget;
};

class CdbOptionsPage : public Core::IOptionsPage
{
    Q_DISABLE_COPY(CdbOptionsPage)
    Q_OBJECT
public:
    explicit CdbOptionsPage();
    virtual ~CdbOptionsPage();

    static CdbOptionsPage *instance();

    // IOptionsPage
    virtual QString id() const { return settingsId(); }
    virtual QString displayName() const;
    virtual QString category() const;
    virtual QString displayCategory() const;
    QIcon categoryIcon() const;

    virtual QWidget *createPage(QWidget *parent);
    virtual void apply();
    virtual void finish();
    virtual bool matches(const QString &) const;

    static QString settingsId();

    QSharedPointer<CdbOptions> options() const { return m_options; }

private:
    static CdbOptionsPage *m_instance;
    const QSharedPointer<CdbOptions> m_options;
    QPointer<CdbOptionsPageWidget> m_widget;
    QString m_searchKeywords;
};

} // namespace Internal
} // namespace Debugger

#endif // CDBSETTINGSPAGE_H
