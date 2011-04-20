/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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

#ifndef LOGGERWIDGET_H
#define LOGGERWIDGET_H

#include <QtGui/QMainWindow>
#include <QtCore/QMetaType>

QT_BEGIN_NAMESPACE

class QPlainTextEdit;
class QMenu;
class QAction;

class LoggerWidget : public QMainWindow {
    Q_OBJECT
public:
    LoggerWidget(QWidget *parent=0);

    enum Visibility { ShowWarnings, HideWarnings, AutoShowWarnings };

    Visibility defaultVisibility() const;
    void setDefaultVisibility(Visibility visibility);

    QMenu *preferencesMenu();
    QAction *showAction();

public slots:
    void append(const QString &msg);

private slots:
    void warningsPreferenceChanged(QAction *action);
    void readSettings();
    void saveSettings();

protected:
    void showEvent(QShowEvent *event);
    void closeEvent(QCloseEvent *event);

signals:
    void opened();
    void closed();

private:
    void setupPreferencesMenu();

    QMenu *m_preferencesMenu;
    QAction *m_showWidgetAction;
    QPlainTextEdit *m_plainTextEdit;

    enum ConfigOrigin { CommandLineOrigin, SettingsOrigin };
    ConfigOrigin m_visibilityOrigin;
    Visibility m_visibility;
};

QT_END_NAMESPACE

Q_DECLARE_METATYPE(LoggerWidget::Visibility)

#endif // LOGGERWIDGET_H
