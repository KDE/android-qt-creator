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
#ifndef IPUBLISHING_WIZARD_FACTORY_H
#define IPUBLISHING_WIZARD_FACTORY_H

#include <projectexplorer/projectexplorer_export.h>

#include <extensionsystem/pluginmanager.h>

#include <QtCore/QList>
#include <QtGui/QWizard>

namespace ProjectExplorer {
class Project;

/*!
  \class ProjectExplorer::IPublishingWizardFactory

  \brief Provides an interface for creating wizards to publish a project.

  A class implementing this interface is used to create an associated wizard
  that allows users to publish their project to a remote facility, e.g. an
  app store.
  Such a wizard would typically transform the project's content into
  a form expected by that facility ("packaging") and also upload it, if possible.

  The factory objects have to be added to the global object pool via
  \c ExtensionSystem::PluginManager::addObject().
  \sa ExtensionSystem::PluginManager::addObject()
*/

class PROJECTEXPLORER_EXPORT IPublishingWizardFactory : public QObject
{
    Q_OBJECT

public:
    /*!
      A short, one-line description of the type of wizard that this
      factory can create.
    */
    virtual QString displayName() const = 0;

    /*!
      A longer description explaining the exact purpose of the wizard
      created by this factory.
    */
    virtual QString description() const = 0;

    /*!
      Returns true iff the type of wizard that this factory can create
      is available for the given project.
    */
    virtual bool canCreateWizard(const Project *project) const = 0;

    /*!
      Creates a wizard that can publish the given project.
      Behavior is undefined if canCreateWizard() returns false for
      that project.
      \return The newly created publishing wizard
      \sa canCreateWizard()
    */
    virtual QWizard *createWizard(const Project *project) const = 0;

protected:
    IPublishingWizardFactory(QObject *parent = 0) : QObject(parent) {}
};

} // namespace ProjectExplorer

#endif // IPUBLISHING_WIZARD_FACTORY_H
