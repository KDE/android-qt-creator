/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#include "qmlprojectplugin.h"
#include "qmlprojectmanager.h"
#include "qmlprojectapplicationwizard.h"
#include "qmlprojectconstants.h"
#include "qmlproject.h"
#include "qmlprojectrunconfigurationfactory.h"
#include "qmlprojectruncontrol.h"
#include "qmlprojecttarget.h"
#include "fileformat/qmlprojectfileformat.h"

#include <extensionsystem/pluginmanager.h>

#include <coreplugin/fileiconprovider.h>
#include <coreplugin/icore.h>
#include <coreplugin/mimedatabase.h>

#include <texteditor/texteditoractionhandler.h>

#include <projectexplorer/taskhub.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <qtsupport/qtsupportconstants.h>

#include <QtPlugin>

#include <QApplication>
#include <QMessageBox>
#include <QPushButton>

namespace QmlProjectManager {

QmlProjectPlugin::QmlProjectPlugin()
{ }

QmlProjectPlugin::~QmlProjectPlugin()
{
}

bool QmlProjectPlugin::initialize(const QStringList &, QString *errorMessage)
{
    using namespace Core;

    Core::MimeDatabase *mimeDB = Core::ICore::mimeDatabase();

    const QLatin1String mimetypesXml(":/qmlproject/QmlProject.mimetypes.xml");

    if (! mimeDB->addMimeTypes(mimetypesXml, errorMessage))
        return false;

    Internal::Manager *manager = new Internal::Manager;

    addAutoReleasedObject(manager);
    addAutoReleasedObject(new Internal::QmlProjectRunConfigurationFactory);
    addAutoReleasedObject(new Internal::QmlProjectRunControlFactory);
    addAutoReleasedObject(new Internal::QmlProjectApplicationWizard);
    addAutoReleasedObject(new Internal::QmlProjectTargetFactory);

    QmlProjectFileFormat::registerDeclarativeTypes();

    Core::FileIconProvider *iconProvider = Core::FileIconProvider::instance();
    iconProvider->registerIconOverlayForSuffix(QIcon(QLatin1String(":/qmlproject/images/qmlproject.png")), "qmlproject");
    return true;
}

void QmlProjectPlugin::extensionsInitialized()
{
}

void QmlProjectPlugin::showQmlObserverToolWarning()
{
    QMessageBox dialog(QApplication::activeWindow());
    QPushButton *qtPref = dialog.addButton(tr("Open Qt Versions"),
                                           QMessageBox::ActionRole);
    dialog.addButton(QMessageBox::Cancel);
    dialog.setDefaultButton(qtPref);
    dialog.setWindowTitle(tr("QML Observer Missing"));
    dialog.setText(tr("QML Observer could not be found for this Qt version."));
    dialog.setInformativeText(tr(
                                  "QML Observer is used to offer debugging features for "
                                  "Qt Quick UI projects in the Qt 4.7 series.\n\n"
                                  "To compile QML Observer, go to the Qt Versions page, "
                                  "select the current Qt version, "
                                  "and click Build in the Helpers section."));
    dialog.exec();
    if (dialog.clickedButton() == qtPref) {
        Core::ICore::showOptionsDialog(
                    ProjectExplorer::Constants::PROJECTEXPLORER_SETTINGS_CATEGORY,
                    QtSupport::Constants::QTVERSION_SETTINGS_PAGE_ID);
    }
}

} // namespace QmlProjectManager

Q_EXPORT_PLUGIN(QmlProjectManager::QmlProjectPlugin)
