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

#ifndef DEBUGGER_LOGWINDOW_H
#define DEBUGGER_LOGWINDOW_H

#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE
class QCursor;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
QT_END_NAMESPACE

namespace Debugger {
namespace Internal {

class LogWindow : public QWidget
{
    Q_OBJECT

public:
    explicit LogWindow(QWidget *parent = 0);

    void setCursor(const QCursor &cursor);

    QString combinedContents() const;
    QString inputContents() const;

    static QString logTimeStamp();

    static bool writeLogContents(const QPlainTextEdit *editor, QWidget *parent = 0);

public slots:
    void clearContents();
    void sendCommand();
    void showOutput(int channel, const QString &output);
    void showInput(int channel, const QString &input);

signals:
    void showPage();
    void statusMessageRequested(const QString &msg, int);

private:
    QPlainTextEdit *m_combinedText;  // combined input/output
    QPlainTextEdit *m_inputText;     // scriptable input alone
    QLineEdit *m_commandEdit;
    QLabel *m_commandLabel;
};


} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_LOGWINDOW_H

