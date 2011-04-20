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

#ifndef HELPPLUGIN_H
#define HELPPLUGIN_H

#include <extensionsystem/iplugin.h>
#include <QtCore/QMap>
#include <QtCore/QStringList>

QT_FORWARD_DECLARE_CLASS(QAction)
QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(QMenu)
QT_FORWARD_DECLARE_CLASS(QToolButton)
QT_FORWARD_DECLARE_CLASS(QUrl)

namespace Core {
class ICore;
class IMode;
class MiniSplitter;
class SideBar;
class SideBarItem;
}   // Core

namespace Utils {
class StyledBar;
}   // Utils

namespace Help {
namespace Internal {
class CentralWidget;
class DocSettingsPage;
class ExternalHelpWindow;
class FilterSettingsPage;
class GeneralSettingsPage;
class HelpMode;
class HelpViewer;
class LocalHelpManager;
class OpenPagesManager;
class SearchWidget;

class HelpPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT

public:
    HelpPlugin();
    virtual ~HelpPlugin();

    bool initialize(const QStringList &arguments, QString *error_message);
    void extensionsInitialized();
    ShutdownFlag aboutToShutdown();

private slots:
    void showExternalWindow();
    void modeChanged(Core::IMode *mode, Core::IMode *old);

    void activateContext();
    void activateIndex();
    void activateContents();
    void activateSearch();
    void activateOpenPages();
    void activateBookmarks();

    void addBookmark();
    void updateFilterComboBox();
    void filterDocumentation(const QString &customFilter);

    void switchToHelpMode();
    void switchToHelpMode(const QUrl &source);
    void slotHideRightPane();
    void showHideSidebar();

    void updateSideBarSource();
    void updateSideBarSource(const QUrl &newUrl);

    void fontChanged();
    void contextHelpOptionChanged();

    void updateCloseButton();
    void setupHelpEngineIfNeeded();

    void highlightSearchTerms();
    void handleHelpRequest(const QUrl &url);

    void slotAboutToShowBackMenu();
    void slotAboutToShowNextMenu();
    void slotOpenActionUrl(QAction *action);

    void openFindToolBar();

    void scaleRightPaneUp();
    void scaleRightPaneDown();
    void resetRightPaneScale();

private:
    void setupUi();
    void resetFilter();
    void activateHelpMode();
    Utils::StyledBar *createWidgetToolBar();
    Utils::StyledBar *createIconToolBar(bool external);
    HelpViewer* viewerForContextMode();
    void createRightPaneContextViewer();

    void doSetupIfNeeded();
    int contextHelpOption() const;
    void connectExternalHelpWindow();
    void setupNavigationMenus(QAction *back, QAction *next, QWidget *parent);

private:
    HelpMode *m_mode;
    Core::ICore *m_core;
    CentralWidget *m_centralWidget;
    QWidget *m_rightPaneSideBarWidget;
    HelpViewer *m_helpViewerForSideBar;

    Core::SideBarItem *m_contentItem;
    Core::SideBarItem *m_indexItem;
    Core::SideBarItem *m_searchItem;
    Core::SideBarItem *m_bookmarkItem;
    Core::SideBarItem *m_openPagesItem;

    DocSettingsPage *m_docSettingsPage;
    FilterSettingsPage *m_filterSettingsPage;
    GeneralSettingsPage *m_generalSettingsPage;

    QComboBox *m_filterComboBox;
    Core::SideBar *m_sideBar;

    bool m_firstModeChange;
    LocalHelpManager *m_helpManager;
    OpenPagesManager *m_openPagesManager;
    Core::MiniSplitter *m_splitter;

    QToolButton *m_closeButton;

    QString m_oldAttrValue;
    QString m_styleProperty;
    QString m_idFromContext;

    Core::IMode* m_oldMode;
    bool m_connectWindow;
    ExternalHelpWindow *m_externalWindow;

    QMenu *m_backMenu;
    QMenu *m_nextMenu;
    Utils::StyledBar *m_internalHelpBar;
    Utils::StyledBar *m_externalHelpBar;
};

} // namespace Internal
} // namespace Help

#endif // HELPPLUGIN_H
