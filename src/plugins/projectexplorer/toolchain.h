/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef TOOLCHAIN_H
#define TOOLCHAIN_H

#include "projectexplorer_export.h"
#include "toolchaintype.h"

#include <utils/environment.h>

#include <QtCore/QString>
#include <QtCore/QPair>
#include <QtCore/QMetaType>

namespace ProjectExplorer {

class IOutputParser;
class Project;

class PROJECTEXPLORER_EXPORT HeaderPath
{
public:
    enum Kind {
        GlobalHeaderPath,
        UserHeaderPath,
        FrameworkHeaderPath
    };

    HeaderPath()
        : _kind(GlobalHeaderPath)
    { }

    HeaderPath(const QString &path, Kind kind)
        : _path(path), _kind(kind)
    { }

    QString path() const { return _path; }
    Kind    kind() const { return _kind; }

private:
    QString _path;
    Kind _kind;
};



class PROJECTEXPLORER_EXPORT ToolChain
{
public:
    virtual QByteArray predefinedMacros() = 0;
    virtual QList<HeaderPath> systemHeaderPaths() = 0;
    virtual void addToEnvironment(Utils::Environment &env) = 0;
    virtual ToolChainType type() const = 0;
    virtual QString makeCommand() const = 0;
    virtual IOutputParser *outputParser() const = 0;

    ToolChain();
    virtual ~ToolChain();

    static bool equals(const ToolChain *, const ToolChain *);
    // Factory methods
    static ToolChain *createGccToolChain(const QString &gcc);
    static ToolChain *createMinGWToolChain(const QString &gcc, const QString &mingwPath);
    static ToolChain *createLinuxIccToolChain();
    static ToolChain *createMSVCToolChain(const QString &name, bool amd64);
    static ToolChain *createWinCEToolChain(const QString &name, const QString &platform);
    static QStringList availableMSVCVersions();
    static QStringList availableMSVCVersions(bool amd64); // filter 32/64bit apart
    static QList<ToolChainType> supportedToolChains();

    static QString toolChainName(ToolChainType tc);

protected:
    virtual bool equals(const ToolChain *other) const = 0;
};

class PROJECTEXPLORER_EXPORT GccToolChain : public ToolChain
{
public:
    GccToolChain(const QString &gcc);
    virtual QByteArray predefinedMacros();
    virtual QList<HeaderPath> systemHeaderPaths();
    virtual void addToEnvironment(Utils::Environment &env);
    virtual ToolChainType type() const;
    virtual QString makeCommand() const;
    virtual IOutputParser *outputParser() const;

protected:
    virtual bool equals(const ToolChain *other) const;
    QByteArray m_predefinedMacros;
    QList<HeaderPath> m_systemHeaderPaths;
    QString gcc() const { return m_gcc; }

private:
    QString m_gcc;
};

// TODO this class needs to fleshed out more
class PROJECTEXPLORER_EXPORT MinGWToolChain : public GccToolChain
{
public:
    MinGWToolChain(const QString &gcc, const QString &mingwPath);
    virtual void addToEnvironment(Utils::Environment &env);
    virtual ToolChainType type() const;
    virtual QString makeCommand() const;
    virtual IOutputParser *outputParser() const;

protected:
    virtual bool equals(const ToolChain *other) const;

private:
    QString m_mingwPath;
};

class PROJECTEXPLORER_EXPORT LinuxIccToolChain : public GccToolChain
{
public:
    LinuxIccToolChain();
    virtual ToolChainType type() const;

    virtual IOutputParser *outputParser() const;
};

// TODO some stuff needs to be moved into this
class PROJECTEXPLORER_EXPORT MSVCToolChain : public ToolChain
{
    Q_DISABLE_COPY(MSVCToolChain)
public:
    // A MSVC installation (SDK or VS) with name and setup script with args
    struct Installation {
        enum Type { WindowsSDK, VS };
        enum Platform { s32, s64, ia64, amd64 };

        explicit Installation(Type t, const QString &name, Platform p,
                              const QString &varsBat,
                              const QString &varBatArg = QString());
        Installation();
        static QString platformName(Platform t);
        bool is64bit() const;

        Type type;
        QString name;
        Platform platform;
        QString varsBat;    // Script to setup environment
        QString varsBatArg; // Argument
    };
    // Find all installations
    typedef QList<Installation> InstallationList;
    static InstallationList installations();
    // Return matching installation or empty one
    static Installation findInstallationByName(bool is64Bit,
                                               const QString &name = QString(),
                                               bool excludeSDK = false);
    static Installation findInstallationByMkSpec(bool is64Bit,
                                                 const QString &mkSpec,
                                                 bool excludeSDK = false);

    static MSVCToolChain *create(const QString &name,
                                 bool amd64 = false);
    virtual QByteArray predefinedMacros();
    virtual QList<HeaderPath> systemHeaderPaths();
    virtual void addToEnvironment(Utils::Environment &env);
    virtual ToolChainType type() const;
    virtual QString makeCommand() const;
    virtual IOutputParser *outputParser() const;

protected:
    explicit MSVCToolChain(const Installation &in);

    typedef QPair<QString, QString> StringStringPair;
    typedef QList<StringStringPair> StringStringPairList;

    virtual bool equals(const ToolChain *other) const;
    static StringStringPairList readEnvironmentSetting(const QString &varsBat,
                                                       const QStringList &args,
                                                       const Utils::Environment &env);

    QByteArray m_predefinedMacros;
    const Installation m_installation;

private:
    static StringStringPairList readEnvironmentSettingI(const QString &varsBat,
                                                        const QStringList &args,
                                                        const Utils::Environment &env);

    mutable StringStringPairList m_values;
    mutable bool m_valuesSet;
    mutable Utils::Environment m_lastEnvironment;
};

PROJECTEXPLORER_EXPORT QDebug operator<<(QDebug in, const MSVCToolChain::Installation &i);

// TODO some stuff needs to be moved into here
class PROJECTEXPLORER_EXPORT WinCEToolChain : public MSVCToolChain
{
public:
    static WinCEToolChain *create(const QString &name, const QString &platform);

    virtual QByteArray predefinedMacros();
    virtual QList<HeaderPath> systemHeaderPaths();
    virtual void addToEnvironment(Utils::Environment &env);
    virtual ToolChainType type() const;

protected:
    explicit WinCEToolChain(const Installation &in, const QString &platform);
    virtual bool equals(const ToolChain *other) const;

private:
    const QString m_platform;
};

}

Q_DECLARE_METATYPE(ProjectExplorer::ToolChainType)

#endif // TOOLCHAIN_H
