/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef CMAKEBUILDCONFIGURATION_H
#define CMAKEBUILDCONFIGURATION_H

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/toolchaintype.h>

namespace ProjectExplorer {
class ToolChain;
}

namespace CMakeProjectManager {
namespace Internal {

class CMakeTarget;
class CMakeBuildConfigurationFactory;

class CMakeBuildConfiguration : public ProjectExplorer::BuildConfiguration
{
    Q_OBJECT
    friend class CMakeBuildConfigurationFactory;

public:
    CMakeBuildConfiguration(CMakeTarget *parent);
    ~CMakeBuildConfiguration();

    CMakeTarget *cmakeTarget() const;

    virtual QString buildDirectory() const;

    ProjectExplorer::ToolChainType toolChainType() const;
    ProjectExplorer::ToolChain *toolChain() const;

    void setBuildDirectory(const QString &buildDirectory);

    QString msvcVersion() const;
    void setMsvcVersion(const QString &msvcVersion);

    QVariantMap toMap() const;

    ProjectExplorer::IOutputParser *createOutputParser() const;

    Utils::Environment baseEnvironment() const;

signals:
    void msvcVersionChanged();

protected:
    CMakeBuildConfiguration(CMakeTarget *parent, CMakeBuildConfiguration *source);
    virtual bool fromMap(const QVariantMap &map);

private:
    void updateToolChain() const;
    mutable ProjectExplorer::ToolChain *m_toolChain;
    QString m_buildDirectory;
    QString m_msvcVersion;
};

class CMakeBuildConfigurationFactory : public ProjectExplorer::IBuildConfigurationFactory
{
    Q_OBJECT

public:
    CMakeBuildConfigurationFactory(QObject *parent = 0);
    ~CMakeBuildConfigurationFactory();

    QStringList availableCreationIds(ProjectExplorer::Target *parent) const;
    QString displayNameForId(const QString &id) const;

    bool canCreate(ProjectExplorer::Target *parent, const QString &id) const;
    CMakeBuildConfiguration *create(ProjectExplorer::Target *parent, const QString &id);
    bool canClone(ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *source) const;
    CMakeBuildConfiguration *clone(ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *source);
    bool canRestore(ProjectExplorer::Target *parent, const QVariantMap &map) const;
    CMakeBuildConfiguration *restore(ProjectExplorer::Target *parent, const QVariantMap &map);
};

} // namespace Internal
} // namespace CMakeProjectManager

#endif // CMAKEBUILDCONFIGURATION_H
