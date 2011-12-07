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

#ifndef CUSTOMPROJECTWIZARD_H
#define CUSTOMPROJECTWIZARD_H

#include "../projectexplorer_export.h"

#include <coreplugin/basefilewizard.h>

#include <QtCore/QSharedPointer>
#include <QtCore/QList>
#include <QtCore/QMap>

QT_BEGIN_NAMESPACE
class QDir;
QT_END_NAMESPACE

namespace Utils {
    class Wizard;
}

namespace ProjectExplorer {
class CustomWizard;
struct CustomWizardPrivate;
class BaseProjectWizardDialog;

namespace Internal {
    struct CustomWizardParameters;
    struct CustomWizardContext;
}

// Documentation inside.
class ICustomWizardFactory {
public:
    virtual CustomWizard *create(const Core::BaseFileWizardParameters& baseFileParameters,
                                 QObject *parent = 0) const = 0;
    virtual ~ICustomWizardFactory() {}
};

// Convenience template to create wizard factory classes.
template <class Wizard> class CustomWizardFactory : public ICustomWizardFactory {
    virtual CustomWizard *create(const Core::BaseFileWizardParameters& baseFileParameters,
                                 QObject *parent = 0) const
    { return new Wizard(baseFileParameters, parent); }
};

// Documentation inside.
class PROJECTEXPLORER_EXPORT CustomWizard : public Core::BaseFileWizard
{
    Q_OBJECT
public:
    typedef QMap<QString, QString> FieldReplacementMap;
    typedef QSharedPointer<ICustomWizardFactory> ICustomWizardFactoryPtr;

    explicit CustomWizard(const Core::BaseFileWizardParameters& baseFileParameters,
                          QObject *parent = 0);
    virtual ~CustomWizard();

    // Can be reimplemented to create custom wizards. initWizardDialog() needs to be
    // called.
    virtual QWizard *createWizardDialog(QWidget *parent,
                                        const QString &defaultPath,
                                        const WizardPageList &extensionPages) const;

    virtual Core::GeneratedFiles generateFiles(const QWizard *w, QString *errorMessage) const;


    // Register a factory for a derived custom widget
    static void registerFactory(const QString &name, const ICustomWizardFactoryPtr &f);
    template <class Wizard> static void registerFactory(const QString &name)
        { registerFactory(name, ICustomWizardFactoryPtr(new CustomWizardFactory<Wizard>)); }

    // Create all wizards. As other plugins might register factories for derived
    // classes, call it in extensionsInitialized().
    static QList<CustomWizard*> createWizards();

    static void setVerbose(int);
    static int verbose();

protected:
    typedef QSharedPointer<Internal::CustomWizardParameters> CustomWizardParametersPtr;
    typedef QSharedPointer<Internal::CustomWizardContext> CustomWizardContextPtr;

    void initWizardDialog(Utils::Wizard *w, const QString &defaultPath,
                          const WizardPageList &extensionPages) const;

    // generate files in path
    Core::GeneratedFiles generateWizardFiles(QString *errorMessage) const;
    // Create replacement map as static base fields + QWizard fields
    FieldReplacementMap replacementMap(const QWizard *w) const;
    virtual bool writeFiles(const Core::GeneratedFiles &files, QString *errorMessage);

    CustomWizardParametersPtr parameters() const;
    CustomWizardContextPtr context() const;

private:
    void setParameters(const CustomWizardParametersPtr &p);

    static CustomWizard *createWizard(const CustomWizardParametersPtr &p, const Core::BaseFileWizardParameters &b);
    CustomWizardPrivate *d;
};

// Documentation inside.
class PROJECTEXPLORER_EXPORT CustomProjectWizard : public CustomWizard
{
    Q_OBJECT
public:
    explicit CustomProjectWizard(const Core::BaseFileWizardParameters& baseFileParameters,
                                 QObject *parent = 0);

    virtual QWizard *createWizardDialog(QWidget *parent,
                                        const QString &defaultPath,
                                        const WizardPageList &extensionPages) const;

    virtual Core::GeneratedFiles generateFiles(const QWizard *w, QString *errorMessage) const;

    static bool postGenerateOpen(const Core::GeneratedFiles &l, QString *errorMessage = 0);

signals:
    void projectLocationChanged(const QString &path);

protected:
    virtual bool postGenerateFiles(const QWizard *w, const Core::GeneratedFiles &l, QString *errorMessage);

    void initProjectWizardDialog(BaseProjectWizardDialog *w, const QString &defaultPath,
                                 const WizardPageList &extensionPages) const;

private slots:
    void projectParametersChanged(const QString &project, const QString &path);
};

} // namespace ProjectExplorer

#endif // CUSTOMPROJECTWIZARD_H
