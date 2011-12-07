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

#ifndef TOOLCHAIN_H
#define TOOLCHAIN_H

#include "projectexplorer_export.h"
#include "headerpath.h"

#include <utils/fileutils.h>

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QVariantMap>

namespace Utils {
class Environment;
}

namespace ProjectExplorer {

namespace Internal {
class ToolChainPrivate;
}

class Abi;
class IOutputParser;
class ToolChainConfigWidget;
class ToolChainFactory;
class ToolChainManager;

// --------------------------------------------------------------------------
// ToolChain (documentation inside)
// --------------------------------------------------------------------------

class PROJECTEXPLORER_EXPORT ToolChain
{
public:
    virtual ~ToolChain();

    QString displayName() const;
    void setDisplayName(const QString &name);

    bool isAutoDetected() const;
    QString id() const;

    virtual QString typeName() const = 0;
    virtual Abi targetAbi() const = 0;

    virtual bool isValid() const = 0;

    virtual QStringList restrictedToTargets() const;

    virtual QByteArray predefinedMacros() const = 0;
    virtual QList<HeaderPath> systemHeaderPaths() const = 0;
    virtual void addToEnvironment(Utils::Environment &env) const = 0;
    virtual QString makeCommand() const = 0;

    virtual Utils::FileName mkspec() const = 0;

    virtual QString debuggerCommand() const = 0;
    virtual QString defaultMakeTarget() const;
    virtual IOutputParser *outputParser() const = 0;

    virtual bool operator ==(const ToolChain &) const;

    virtual ToolChainConfigWidget *configurationWidget() = 0;
    virtual bool canClone() const;
    virtual ToolChain *clone() const = 0;

    // Used by the toolchainmanager to save user-generated tool chains.
    // Make sure to call this method when deriving!
    virtual QVariantMap toMap() const;

protected:
    ToolChain(const QString &id, bool autoDetect);
    explicit ToolChain(const ToolChain &);

    void setId(const QString &id);

    void toolChainUpdated();

    // Make sure to call this method when deriving!
    virtual bool fromMap(const QVariantMap &data);

private:
    void setAutoDetected(bool);

    Internal::ToolChainPrivate *const d;

    friend class ToolChainManager;
    friend class ToolChainFactory;
};

class PROJECTEXPLORER_EXPORT ToolChainFactory : public QObject
{
    Q_OBJECT

public:
    virtual QString displayName() const = 0;
    virtual QString id() const = 0;

    virtual QList<ToolChain *> autoDetect();

    virtual bool canCreate();
    virtual ToolChain *create();

    virtual bool canRestore(const QVariantMap &data);
    virtual ToolChain *restore(const QVariantMap &data);

    static QString idFromMap(const QVariantMap &data);
};

} // namespace ProjectExplorer

#endif // TOOLCHAIN_H
