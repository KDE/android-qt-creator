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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef ANDROIDMANAGER_H
#define ANDROIDMANAGER_H

#include <QtCore/QObject>

namespace ProjectExplorer {
    class ToolChain;
}
using ProjectExplorer::ToolChain;

namespace Qt4ProjectManager {
    class QtVersion;
namespace Internal {

class AndroidDeployStepFactory;
class AndroidPackageCreationFactory;
class AndroidRunControlFactory;
class AndroidRunConfigurationFactory;
class AndroidSettingsPage;
class AndroidQemuManager;

class AndroidManager : public QObject
{
    Q_OBJECT

public:
    AndroidManager();
    ~AndroidManager();
    static AndroidManager &instance();

    bool isValidAndroidQtVersion(const Qt4ProjectManager::QtVersion *version) const;
    ToolChain *androidToolChain() const;

    AndroidSettingsPage *settingsPage() const { return m_settingsPage; }

private:
    static AndroidManager *m_instance;

    AndroidRunControlFactory *m_runControlFactory;
    AndroidRunConfigurationFactory *m_runConfigurationFactory;
    AndroidPackageCreationFactory *m_packageCreationFactory;
    AndroidDeployStepFactory *m_deployStepFactory;
    AndroidSettingsPage *m_settingsPage;
    AndroidQemuManager *m_qemuRuntimeManager;
};

    } // namespace Internal
} // namespace Qt4ProjectManager

#endif // ANDROIDMANAGER_H
