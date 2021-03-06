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

#include "qmakeglobals.h"

#include "qmakeevaluator.h"
#include "ioutils.h"

#include <QByteArray>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QList>
#include <QRegExp>
#include <QSet>
#include <QStack>
#include <QString>
#include <QStringList>
#include <QTextStream>
#ifdef PROEVALUATOR_THREAD_SAFE
# include <QThreadPool>
#endif

#ifdef Q_OS_UNIX
#include <unistd.h>
#include <sys/utsname.h>
#else
#include <Windows.h>
#endif
#include <stdio.h>
#include <stdlib.h>

#ifdef Q_OS_WIN32
#define QT_POPEN _popen
#define QT_PCLOSE _pclose
#else
#define QT_POPEN popen
#define QT_PCLOSE pclose
#endif

QT_BEGIN_NAMESPACE

#define fL1S(s) QString::fromLatin1(s)

namespace { // MSVC doesn't seem to know the semantics of "static" ...

static struct {
    QRegExp reg_variableName;
} statics;

}

static void initStatics()
{
    if (!statics.reg_variableName.isEmpty())
        return;

    statics.reg_variableName.setPattern(QLatin1String("\\$\\(.*\\)"));
    statics.reg_variableName.setMinimal(true);
}

QMakeGlobals::QMakeGlobals()
{
    initStatics();

    do_cache = true;

#ifdef Q_OS_WIN
    dirlist_sep = QLatin1Char(';');
    dir_sep = QLatin1Char('\\');
#else
    dirlist_sep = QLatin1Char(':');
    dir_sep = QLatin1Char('/');
#endif
    qmakespec = getEnv(QLatin1String("QMAKESPEC"));
}

QMakeGlobals::~QMakeGlobals()
{
    qDeleteAll(baseEnvs);
}

void QMakeGlobals::setCommandLineArguments(const QStringList &args)
{
    QStringList _precmds, _preconfigs, _postcmds, _postconfigs;
    bool after = false;

    bool isConf = false;
    foreach (const QString &arg, args) {
        if (isConf) {
            isConf = false;
            if (after)
                _postconfigs << arg;
            else
                _preconfigs << arg;
        } else if (arg.startsWith(QLatin1Char('-'))) {
            if (arg == QLatin1String("-after")) {
                after = true;
            } else if (arg == QLatin1String("-config")) {
                isConf = true;
            } else if (arg == QLatin1String("-win32")) {
                dir_sep = QLatin1Char('\\');
            } else if (arg == QLatin1String("-unix")) {
                dir_sep = QLatin1Char('/');
            }
        } else if (arg.contains(QLatin1Char('='))) {
            if (after)
                _postcmds << arg;
            else
                _precmds << arg;
        }
    }

    if (!_preconfigs.isEmpty())
        _precmds << (fL1S("CONFIG += ") + _preconfigs.join(fL1S(" ")));
    precmds = _precmds.join(fL1S("\n"));
    if (!_postconfigs.isEmpty())
        _postcmds << (fL1S("CONFIG += ") + _postconfigs.join(fL1S(" ")));
    postcmds = _postcmds.join(fL1S("\n"));
}

QString QMakeGlobals::getEnv(const QString &var) const
{
#ifndef QT_BOOTSTRAPPED
    if (!environment.isEmpty())
        return environment.value(var);
#endif
    return QString::fromLocal8Bit(qgetenv(var.toLocal8Bit().constData()));
}

QStringList QMakeGlobals::getPathListEnv(const QString &var) const
{
    QStringList ret;
    QString val = getEnv(var);
    if (!val.isEmpty()) {
        QStringList vals = val.split(dirlist_sep);
        ret.reserve(vals.length());
        foreach (const QString &it, vals)
            ret << QDir::cleanPath(it);
    }
    return ret;
}

QString QMakeGlobals::expandEnvVars(const QString &str) const
{
    QString string = str;
    int rep;
    QRegExp reg_variableName = statics.reg_variableName; // Copy for thread safety
    while ((rep = reg_variableName.indexIn(string)) != -1)
        string.replace(rep, reg_variableName.matchedLength(),
                       getEnv(string.mid(rep + 2, reg_variableName.matchedLength() - 3)));
    return string;
}

#ifdef PROEVALUATOR_INIT_PROPS
bool QMakeGlobals::initProperties()
{
    QByteArray data;
#ifndef QT_BOOTSTRAPPED
    QProcess proc;
    proc.start(qmake_abslocation, QStringList() << QLatin1String("-query"));
    if (!proc.waitForFinished())
        return false;
    data = proc.readAll();
#else
    if (FILE *proc = QT_POPEN(QString(IoUtils::shellQuote(qmake_abslocation) + QLatin1String(" -query"))
                              .toLocal8Bit(), "r")) {
        char buff[1024];
        while (!feof(proc))
            data.append(buff, int(fread(buff, 1, 1023, proc)));
        QT_PCLOSE(proc);
    }
#endif
    foreach (QByteArray line, data.split('\n'))
        if (!line.startsWith("QMAKE_")) {
            int off = line.indexOf(':');
            if (off < 0) // huh?
                continue;
            if (line.endsWith('\r'))
                line.chop(1);
            QString name = QString::fromLatin1(line.left(off));
            ProString value = ProString(QDir::fromNativeSeparators(
                        QString::fromLocal8Bit(line.mid(off + 1))), ProStringConstants::NoHash);
            properties.insert(ProString(name), value);
            if (name.startsWith(QLatin1String("QT_")) && !name.contains(QLatin1Char('/'))) {
                if (name.startsWith(QLatin1String("QT_INSTALL_"))) {
                    properties.insert(ProString(name + QLatin1String("/raw")), value);
                    properties.insert(ProString(name + QLatin1String("/get")), value);
                    if (name == QLatin1String("QT_INSTALL_PREFIX")
                        || name == QLatin1String("QT_INSTALL_DATA")
                        || name == QLatin1String("QT_INSTALL_BINS")) {
                        name.replace(3, 7, QLatin1String("HOST"));
                        properties.insert(ProString(name), value);
                        properties.insert(ProString(name + QLatin1String("/get")), value);
                    }
                } else if (name.startsWith(QLatin1String("QT_HOST_"))) {
                    properties.insert(ProString(name + QLatin1String("/get")), value);
                }
            }
        }
    properties.insert(ProString("QMAKE_VERSION"), ProString("2.01a", ProStringConstants::NoHash));
    return true;
}
#else
void QMakeGlobals::setProperties(const QHash<QString, QString> &props)
{
    QHash<QString, QString>::ConstIterator it = props.constBegin(), eit = props.constEnd();
    for (; it != eit; ++it)
        properties.insert(ProString(it.key()), ProString(it.value(), ProStringConstants::NoHash));
}
#endif

QT_END_NAMESPACE
