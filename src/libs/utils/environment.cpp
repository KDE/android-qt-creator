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

#include "environment.h"

#include <QtCore/QDir>
#include <QtCore/QProcess>
#include <QtCore/QString>

namespace Utils {

QList<EnvironmentItem> EnvironmentItem::fromStringList(const QStringList &list)
{
    QList<EnvironmentItem> result;
    foreach (const QString &string, list) {
        int pos = string.indexOf(QLatin1Char('='));
        if (pos == -1) {
            EnvironmentItem item(string, QString());
            item.unset = true;
            result.append(item);
        } else {
            EnvironmentItem item(string.left(pos), string.mid(pos+1));
            result.append(item);
        }
    }
    return result;
}

QStringList EnvironmentItem::toStringList(const QList<EnvironmentItem> &list)
{
    QStringList result;
    foreach (const EnvironmentItem &item, list) {
        if (item.unset)
            result << QString(item.name);
        else
            result << QString(item.name + '=' + item.value);
    }
    return result;
}

Environment::Environment(const QStringList &env)
{
    foreach (const QString &s, env) {
        int i = s.indexOf(QLatin1Char('='));
        if (i >= 0) {
#ifdef Q_OS_WIN
            m_values.insert(s.left(i).toUpper(), s.mid(i+1));
#else
            m_values.insert(s.left(i), s.mid(i+1));
#endif
        }
    }
}

QStringList Environment::toStringList() const
{
    QStringList result;
    const QMap<QString, QString>::const_iterator end = m_values.constEnd();
    for (QMap<QString, QString>::const_iterator it = m_values.constBegin(); it != end; ++it) {
        QString entry = it.key();
        entry += QLatin1Char('=');
        entry += it.value();
        result.push_back(entry);
    }
    return result;
}

void Environment::set(const QString &key, const QString &value)
{
#ifdef Q_OS_WIN
    QString _key = key.toUpper();
#else
    const QString &_key = key;
#endif
    m_values.insert(_key, value);
}

void Environment::unset(const QString &key)
{
#ifdef Q_OS_WIN
    QString _key = key.toUpper();
#else
    const QString &_key = key;
#endif
    m_values.remove(_key);
}

void Environment::appendOrSet(const QString &key, const QString &value, const QString &sep)
{
#ifdef Q_OS_WIN
    QString _key = key.toUpper();
#else
    const QString &_key = key;
#endif
    QMap<QString, QString>::iterator it = m_values.find(key);
    if (it == m_values.end()) {
        m_values.insert(_key, value);
    } else {
        // Append unless it is already there
        const QString toAppend = sep + value;
        if (!it.value().endsWith(toAppend))
            it.value().append(toAppend);
    }
}

void Environment::prependOrSet(const QString&key, const QString &value, const QString &sep)
{
#ifdef Q_OS_WIN
    QString _key = key.toUpper();
#else
    const QString &_key = key;
#endif
    QMap<QString, QString>::iterator it = m_values.find(key);
    if (it == m_values.end()) {
        m_values.insert(_key, value);
    } else {
        // Prepend unless it is already there
        const QString toPrepend = value + sep;
        if (!it.value().startsWith(toPrepend))
            it.value().prepend(toPrepend);
    }
}

void Environment::appendOrSetPath(const QString &value)
{
#ifdef Q_OS_WIN
    const QChar sep = QLatin1Char(';');
#else
    const QChar sep = QLatin1Char(':');
#endif
    appendOrSet(QLatin1String("PATH"), QDir::toNativeSeparators(value), QString(sep));
}

void Environment::prependOrSetPath(const QString &value)
{
#ifdef Q_OS_WIN
    const QChar sep = QLatin1Char(';');
#else
    const QChar sep = QLatin1Char(':');
#endif
    prependOrSet(QLatin1String("PATH"), QDir::toNativeSeparators(value), QString(sep));
}

void Environment::prependOrSetLibrarySearchPath(const QString &value)
{
#ifdef Q_OS_MAC
    Q_UNUSED(value);
    // we could set DYLD_LIBRARY_PATH on Mac but it is unnecessary in practice
#elif defined(Q_OS_WIN)
    const QChar sep = QLatin1Char(';');
    const QLatin1String path("PATH");
    prependOrSet(path, QDir::toNativeSeparators(value), QString(sep));
#elif defined(Q_OS_UNIX)
    const QChar sep = QLatin1Char(':');
    const QLatin1String path("LD_LIBRARY_PATH");
    prependOrSet(path, QDir::toNativeSeparators(value), QString(sep));
#endif
}

Environment Environment::systemEnvironment()
{
    return Environment(QProcess::systemEnvironment());
}

void Environment::clear()
{
    m_values.clear();
}

QString Environment::searchInPath(const QString &executable,
                                  const QStringList &additionalDirs) const
{
    QStringList execs;
    execs << executable;
#ifdef Q_OS_WIN
    // Check all the executable extensions on windows:
    QStringList extensions = value(QLatin1String("PATHEXT")).split(QLatin1Char(';'));

    // .exe.bat is legal (and run when starting new.exe), so always go through the complete list once:
    foreach (const QString &ext, extensions)
        execs << executable + ext.toLower();
#endif
    return searchInPath(execs, additionalDirs);
}

QString Environment::searchInPath(const QStringList &executables,
                                  const QStringList &additionalDirs) const
{
    const QChar slash = QLatin1Char('/');
    foreach (const QString &executable, executables) {
        QString exec = QDir::cleanPath(expandVariables(executable));

        if (exec.isEmpty())
            continue;

        QFileInfo baseFi(exec);
        if (baseFi.isAbsolute() && baseFi.exists())
            return exec;

        // Check in directories:
        foreach (const QString &dir, additionalDirs) {
            if (dir.isEmpty())
                continue;
            QFileInfo fi(dir + QLatin1Char('/') + exec);
            if (fi.isFile() && fi.isExecutable())
                return fi.absoluteFilePath();
        }

        // Check in path:
        if (exec.indexOf(slash) != -1)
            continue;
        foreach (const QString &p, path()) {
            QString fp = p;
            fp += slash;
            fp += exec;
            const QFileInfo fi(fp);
            if (fi.exists() && fi.isExecutable() && !fi.isDir())
                return fi.absoluteFilePath();
        }
    }
    return QString();
}

QStringList Environment::path() const
{
#ifdef Q_OS_WIN
    const QChar sep = QLatin1Char(';');
#else
    const QChar sep = QLatin1Char(':');
#endif
    return m_values.value(QLatin1String("PATH")).split(sep, QString::SkipEmptyParts);
}

QString Environment::value(const QString &key) const
{
    return m_values.value(key);
}

QString Environment::key(Environment::const_iterator it) const
{
    return it.key();
}

QString Environment::value(Environment::const_iterator it) const
{
    return it.value();
}

Environment::const_iterator Environment::constBegin() const
{
    return m_values.constBegin();
}

Environment::const_iterator Environment::constEnd() const
{
    return m_values.constEnd();
}

Environment::const_iterator Environment::constFind(const QString &name) const
{
    QMap<QString, QString>::const_iterator it = m_values.constFind(name);
    if (it == m_values.constEnd())
        return constEnd();
    else
        return it;
}

int Environment::size() const
{
    return m_values.size();
}

void Environment::modify(const QList<EnvironmentItem> & list)
{
    Environment resultEnvironment = *this;
    foreach (const EnvironmentItem &item, list) {
        if (item.unset) {
            resultEnvironment.unset(item.name);
        } else {
            // TODO use variable expansion
            QString value = item.value;
            for (int i=0; i < value.size(); ++i) {
                if (value.at(i) == QLatin1Char('$')) {
                    if ((i + 1) < value.size()) {
                        const QChar &c = value.at(i+1);
                        int end = -1;
                        if (c == '(')
                            end = value.indexOf(')', i);
                        else if (c == '{')
                            end = value.indexOf('}', i);
                        if (end != -1) {
                            const QString &name = value.mid(i+2, end-i-2);
                            Environment::const_iterator it = constFind(name);
                            if (it != constEnd())
                                value.replace(i, end-i+1, it.value());
                        }
                    }
                }
            }
            resultEnvironment.set(item.name, value);
        }
    }
    *this = resultEnvironment;
}

QList<EnvironmentItem> Environment::diff(const Environment &other) const
{
    QMap<QString, QString>::const_iterator thisIt = constBegin();
    QMap<QString, QString>::const_iterator otherIt = other.constBegin();

    QList<EnvironmentItem> result;
    while (thisIt != constEnd() || otherIt != other.constEnd()) {
        if (thisIt == constEnd()) {
            result.append(Utils::EnvironmentItem(otherIt.key(), otherIt.value()));
            ++otherIt;
        } else if (otherIt == constEnd()) {
            Utils::EnvironmentItem item(thisIt.key(), QString());
            item.unset = true;
            result.append(item);
            ++thisIt;
        } else if (thisIt.key() < otherIt.key()) {
            Utils::EnvironmentItem item(thisIt.key(), QString());
            item.unset = true;
            result.append(item);
            ++thisIt;
        } else if (thisIt.key() > otherIt.key()) {
            result.append(Utils::EnvironmentItem(otherIt.key(), otherIt.value()));
            ++otherIt;
        } else {
            result.append(Utils::EnvironmentItem(otherIt.key(), otherIt.value()));
            ++otherIt;
            ++thisIt;
        }
    }
    return result;
}

bool Environment::hasKey(const QString &key)
{
    return m_values.contains(key);
}

bool Environment::operator!=(const Environment &other) const
{
    return !(*this == other);
}

bool Environment::operator==(const Environment &other) const
{
    return m_values == other.m_values;
}

/** Expand environment variables in a string.
 *
 * Environment variables are accepted in the following forms:
 * $SOMEVAR, ${SOMEVAR} on Unix and %SOMEVAR% on Windows.
 * No escapes and quoting are supported.
 * If a variable is not found, it is not substituted.
 */
QString Environment::expandVariables(const QString &input) const
{
    QString result = input;

#ifdef Q_OS_WIN
    for (int vStart = -1, i = 0; i < result.length(); ) {
        if (result.at(i++) == QLatin1Char('%')) {
            if (vStart > 0) {
                const_iterator it = m_values.constFind(result.mid(vStart, i - vStart - 1).toUpper());
                if (it != m_values.constEnd()) {
                    result.replace(vStart - 1, i - vStart + 1, *it);
                    i = vStart - 1 + it->length();
                    vStart = -1;
                } else {
                    vStart = i;
                }
            } else {
                vStart = i;
            }
        }
    }
#else
    enum { BASE, OPTIONALVARIABLEBRACE, VARIABLE, BRACEDVARIABLE } state = BASE;
    int vStart = -1;

    for (int i = 0; i < result.length();) {
        QChar c = result.at(i++);
        if (state == BASE) {
            if (c == QLatin1Char('$'))
                state = OPTIONALVARIABLEBRACE;
        } else if (state == OPTIONALVARIABLEBRACE) {
            if (c == QLatin1Char('{')) {
                state = BRACEDVARIABLE;
                vStart = i;
            } else if (c.isLetterOrNumber() || c == QLatin1Char('_')) {
                state = VARIABLE;
                vStart = i - 1;
            } else {
                state = BASE;
            }
        } else if (state == BRACEDVARIABLE) {
            if (c == QLatin1Char('}')) {
                const_iterator it = m_values.constFind(result.mid(vStart, i - 1 - vStart));
                if (it != constEnd()) {
                    result.replace(vStart - 2, i - vStart + 2, *it);
                    i = vStart - 2 + it->length();
                }
                state = BASE;
            }
        } else if (state == VARIABLE) {
            if (!c.isLetterOrNumber() && c != QLatin1Char('_')) {
                const_iterator it = m_values.constFind(result.mid(vStart, i - vStart - 1));
                if (it != constEnd()) {
                    result.replace(vStart - 1, i - vStart, *it);
                    i = vStart - 1 + it->length();
                }
                state = BASE;
            }
        }
    }
    if (state == VARIABLE) {
        const_iterator it = m_values.constFind(result.mid(vStart));
        if (it != constEnd())
            result.replace(vStart - 1, result.length() - vStart + 1, *it);
    }
#endif
    return result;
}

QStringList Environment::expandVariables(const QStringList &variables) const
{
    QStringList results;
    foreach (const QString &i, variables)
        results << expandVariables(i);
    return results;
}

} // namespace Utils
