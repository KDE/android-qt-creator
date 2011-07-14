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

#ifndef PROJECTEXPLORER_DEPLOYCONFIGURATION_H
#define PROJECTEXPLORER_DEPLOYCONFIGURATION_H

#include "projectexplorer_export.h"

#include "projectconfiguration.h"
#include "namedwidget.h"

#include <QtCore/QString>

QT_FORWARD_DECLARE_CLASS(QStringList)

namespace ProjectExplorer {

class BuildStepList;
class Target;
class DeployConfigurationFactory;
class DeployConfigurationWidget;

class PROJECTEXPLORER_EXPORT DeployConfiguration : public ProjectConfiguration
{
    Q_OBJECT

public:
    // ctors are protected
    virtual ~DeployConfiguration();

    BuildStepList *stepList() const;

    virtual QVariantMap toMap() const;

    virtual DeployConfigurationWidget *configurationWidget() const;

    Target *target() const;

protected:
    DeployConfiguration(Target *target, const QString &id);
    DeployConfiguration(Target *target, DeployConfiguration *source);

    void cloneSteps(DeployConfiguration *source);

    virtual bool fromMap(const QVariantMap &map);

private:
    friend class DeployConfigurationFactory;

    BuildStepList *m_stepList;
};

class PROJECTEXPLORER_EXPORT DeployConfigurationFactory :
    public QObject
{
    Q_OBJECT

public:
    explicit DeployConfigurationFactory(QObject *parent = 0);
    virtual ~DeployConfigurationFactory();

    // used to show the list of possible additons to a target, returns a list of types
    virtual QStringList availableCreationIds(Target *parent) const;
    // used to translate the types to names to display to the user
    virtual QString displayNameForId(const QString &id) const;

    virtual bool canCreate(Target *parent, const QString &id) const;
    virtual DeployConfiguration *create(Target *parent, const QString &id);
    // used to recreate the runConfigurations when restoring settings
    virtual bool canRestore(Target *parent, const QVariantMap &map) const;
    virtual DeployConfiguration *restore(Target *parent, const QVariantMap &map);
    virtual bool canClone(Target *parent, DeployConfiguration *product) const;
    virtual DeployConfiguration *clone(Target *parent, DeployConfiguration *product);

signals:
    void availableCreationIdsChanged();
};

class PROJECTEXPLORER_EXPORT DeployConfigurationWidget : public NamedWidget
{
    Q_OBJECT

public:
    explicit DeployConfigurationWidget(QWidget *parent = 0);

    // This is called to set up the config widget before showing it
    virtual void init(DeployConfiguration *dc) = 0;
};

} // namespace ProjectExplorer

Q_DECLARE_METATYPE(ProjectExplorer::DeployConfiguration *)

#endif // PROJECTEXPLORER_DEPLOYCONFIGURATION_H
