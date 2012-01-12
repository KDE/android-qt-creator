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

#include "vcsplugin.h"
#include "diffhighlighter.h"
#include "commonsettingspage.h"
#include "nicknamedialog.h"
#include "vcsbaseoutputwindow.h"
#include "corelistener.h"

#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/id.h>
#include <coreplugin/mimedatabase.h>

#include <QtCore/QtPlugin>
#include <QtCore/QDebug>

namespace VcsBase {
namespace Internal {

VcsPlugin *VcsPlugin::m_instance = 0;

VcsPlugin::VcsPlugin() :
    m_settingsPage(0),
    m_nickNameModel(0),
    m_coreListener(0)
{
    m_instance = this;
}

VcsPlugin::~VcsPlugin()
{
    m_instance = 0;
}

bool VcsPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)

    Core::ICore *core = Core::ICore::instance();
    if (!core->mimeDatabase()->addMimeTypes(QLatin1String(":/vcsbase/VCSBase.mimetypes.xml"), errorMessage))
        return false;

    m_coreListener = new CoreListener;
    addAutoReleasedObject(m_coreListener);

    m_settingsPage = new CommonOptionsPage;
    addAutoReleasedObject(m_settingsPage);
    addAutoReleasedObject(VcsBaseOutputWindow::instance());
    connect(m_settingsPage, SIGNAL(settingsChanged(VcsBase::Internal::CommonVcsSettings)),
            this, SIGNAL(settingsChanged(VcsBase::Internal::CommonVcsSettings)));
    connect(m_settingsPage, SIGNAL(settingsChanged(VcsBase::Internal::CommonVcsSettings)),
            this, SLOT(slotSettingsChanged()));
    slotSettingsChanged();
    return true;
}

void VcsPlugin::extensionsInitialized()
{
}

VcsPlugin *VcsPlugin::instance()
{
    return m_instance;
}

CoreListener *VcsPlugin::coreListener() const
{
    return m_coreListener;
}

CommonVcsSettings VcsPlugin::settings() const
{
    return m_settingsPage->settings();
}

/* Delayed creation/update of the nick name model. */
QStandardItemModel *VcsPlugin::nickNameModel()
{
    if (!m_nickNameModel) {
        m_nickNameModel = NickNameDialog::createModel(this);
        populateNickNameModel();
    }
    return m_nickNameModel;
}

void VcsPlugin::populateNickNameModel()
{
    QString errorMessage;
    if (!NickNameDialog::populateModelFromMailCapFile(settings().nickNameMailMap,
                                                      m_nickNameModel,
                                                      &errorMessage)) {
        qWarning("%s", qPrintable(errorMessage));
    }
}

void VcsPlugin::slotSettingsChanged()
{
    if (m_nickNameModel)
        populateNickNameModel();
}

} // namespace Internal
} // namespace VcsBase

Q_EXPORT_PLUGIN(VcsBase::Internal::VcsPlugin)
