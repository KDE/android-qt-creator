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

#ifndef APPOUTPUTPANE_H
#define APPOUTPUTPANE_H

#include <coreplugin/outputwindow.h>
#include <coreplugin/ioutputpane.h>

QT_BEGIN_NAMESPACE
class QTabWidget;
class QToolButton;
class QAction;
QT_END_NAMESPACE

namespace ProjectExplorer {

class RunControl;
class Project;

namespace Internal {

class AppOutputPane : public Core::IOutputPane
{
    Q_OBJECT

public:
    enum CloseTabMode {
        CloseTabNoPrompt,
        CloseTabWithPrompt
    };

    AppOutputPane();
    virtual ~AppOutputPane();

    QWidget *outputWidget(QWidget *);
    QList<QWidget*> toolBarWidgets() const;
    QString displayName() const;
    int priorityInStatusBar() const;
    void clearContents();
    void visibilityChanged(bool);
    bool canFocus();
    bool hasFocus();
    void setFocus();

    bool canNext();
    bool canPrevious();
    void goToNext();
    void goToPrev();
    bool canNavigate();

    void createNewOutputWindow(RunControl *rc);
    void showTabFor(RunControl *rc);

    bool aboutToClose() const;
    bool closeTabs(CloseTabMode mode);

signals:
     void allRunControlsFinished();

public slots:
    // ApplicationOutput specifics
    void projectRemoved();

    void appendMessage(ProjectExplorer::RunControl *rc, const QString &out,
                       Utils::OutputFormat format);

private slots:
    void reRunRunControl();
    void stopRunControl();
    void attachToRunControl();
    bool closeTab(int index);
    void tabChanged(int);
    void runControlStarted();
    void runControlFinished();

    void aboutToUnloadSession();
    void updateFromSettings();
    void enableButtons();

private:
    void enableButtons(const RunControl *rc, bool isRunning);

    struct RunControlTab {
        explicit RunControlTab(RunControl *runControl = 0,
                               Core::OutputWindow *window = 0);
        RunControl* runControl;
        Core::OutputWindow *window;
        // Is the run control stopping asynchronously, close the tab once it finishes
        bool asyncClosing;
    };

    bool isRunning() const;
    bool closeTab(int index, CloseTabMode cm);
    bool optionallyPromptToStop(RunControl *runControl);

    int indexOf(const RunControl *) const;
    int indexOf(const QWidget *outputWindow) const;
    int currentIndex() const;
    RunControl *currentRunControl() const;
    int tabWidgetIndexOf(int runControlIndex) const;
    void handleOldOutput(Core::OutputWindow *window) const;

    QWidget *m_mainWidget;
    QTabWidget *m_tabWidget;
    QList<RunControlTab> m_runControlTabs;
    QAction *m_stopAction;
    QToolButton *m_reRunButton;
    QToolButton *m_stopButton;
    QToolButton *m_attachButton;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // APPOUTPUTPANE_H
