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

#ifndef VCSBASEOUTPUTWINDOW_H
#define VCSBASEOUTPUTWINDOW_H

#include "vcsbase_global.h"

#include  <coreplugin/ioutputpane.h>

namespace VcsBase {

struct VcsBaseOutputWindowPrivate;

class VCSBASE_EXPORT VcsBaseOutputWindow : public Core::IOutputPane
{
    Q_OBJECT
    Q_PROPERTY(QString repository READ repository WRITE setRepository)

public:
    ~VcsBaseOutputWindow();

    QWidget *outputWidget(QWidget *parent);
    QList<QWidget *> toolBarWidgets() const;
    QString displayName() const;

    int priorityInStatusBar() const;

    void clearContents();
    void visibilityChanged(bool visible);

    void setFocus();
    bool hasFocus() const;
    bool canFocus() const;

    bool canNavigate() const;
    bool canNext() const;
    bool canPrevious() const;
    void goToNext();
    void goToPrev();

    static VcsBaseOutputWindow *instance();

    QString repository() const;

    // Helper to consistently format log entries for commands as
    // 'Executing <dir>: <cmd> <args>'. Hides well-known password option
    // arguments.
    static QString msgExecutionLogEntry(const QString &workingDir,
                                        const QString &executable,
                                        const QStringList &arguments);

public slots:
    void setRepository(const QString &);
    void clearRepository();

    // Set the whole text.
    void setText(const QString &text);
    // Set text from QProcess' output data using the Locale's converter.
    void setData(const QByteArray &data);

    // Append text and pop up.
    void append(const QString &text);
    // Append data using the Locale's converter and pop up.
    void appendData(const QByteArray &data);

    // Silently append text, do not pop up.
    void appendSilently(const QString &text);
    // Silently append data using the Locale's converter, do not pop up.
    void appendDataSilently(const QByteArray &data);

    // Append red error text and pop up.
    void appendError(const QString &text);

    // Append dark-yellow warning text and pop up.
    void appendWarning(const QString &text);

    // Append a command, prepended by a log time stamp. "Executing: vcs -diff"
    // will result in "10:00 Executing: vcs -diff" in bold
    void appendCommand(const QString &text);
    // Append a standard-formatted entry for command execution
    // (see msgExecutionLogEntry).
    void appendCommand(const QString &workingDirectory,
                       const QString &binary,
                       const QStringList &args);

private:
    VcsBaseOutputWindow();

    VcsBaseOutputWindowPrivate *d;
};

} // namespace VcsBase

#endif // VCSBASEOUTPUTWINDOW_H
