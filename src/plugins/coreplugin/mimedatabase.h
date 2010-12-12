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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef MIMEDATABASE_H
#define MIMEDATABASE_H

#include <coreplugin/core_global.h>
#include <QtCore/QStringList>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QSharedPointer>
#include <QtCore/QByteArray>
#include <QtCore/QMutex>
#include <QtCore/QFileInfo>

QT_BEGIN_NAMESPACE
class QIODevice;
class QRegExp;
class QDebug;
class QFileInfo;
QT_END_NAMESPACE

namespace Core {

class MimeTypeData;
class MimeDatabasePrivate;

namespace Internal {
    class BaseMimeTypeParser;
    class FileMatchContext;
}

/* Magic (file contents) matcher interface. */
class CORE_EXPORT IMagicMatcher
{
    Q_DISABLE_COPY(IMagicMatcher)
protected:
    IMagicMatcher() {}
public:
    // Check for a match on contents of a file
    virtual bool matches(const QByteArray &data) const = 0;
    // Return a priority value from 1..100
    virtual int priority() const = 0;
    virtual ~IMagicMatcher() {}
};

/* Utility class: A standard Magic match rule based on contents. Currently there are
 * implementations for "string" and "byte". (Others like little16, big16, etc. can be
 * created whenever there is a need.) */
class CORE_EXPORT MagicRule
{
    Q_DISABLE_COPY(MagicRule)
public:
    MagicRule(int startPos, int endPos);
    virtual ~MagicRule();

    virtual bool matches(const QByteArray &data) const = 0;

protected:
    int startPos() const;
    int endPos() const;

private:
    const int m_startPos;
    const int m_endPos;
};

class CORE_EXPORT MagicStringRule : public MagicRule
{
public:
    MagicStringRule(const QString &s, int startPos, int endPos);
    virtual ~MagicStringRule();

    virtual bool matches(const QByteArray &data) const;

private:
    const QByteArray m_pattern;
};

class CORE_EXPORT MagicByteRule : public MagicRule
{
public:
    MagicByteRule(const QString &s, int startPos, int endPos);
    virtual ~MagicByteRule();

    virtual bool matches(const QByteArray &data) const;

private:
    QList<int> m_bytes;
    int m_bytesSize;
};

/* Utility class: A Magic matcher that checks a number of rules based on
 * operator "or". It is used for rules parsed from XML files. */
class CORE_EXPORT MagicRuleMatcher : public IMagicMatcher
{
    Q_DISABLE_COPY(MagicRuleMatcher)
public:
    typedef  QSharedPointer<MagicRule> MagicRuleSharedPointer;

    MagicRuleMatcher();
    void add(const MagicRuleSharedPointer &rule);
    virtual bool matches(const QByteArray &data) const;

    virtual int priority() const;
    void setPriority(int p);

private:
    typedef QList<MagicRuleSharedPointer> MagicRuleList;
    MagicRuleList m_list;
    int m_priority;
};

class CORE_EXPORT MimeGlobPattern
{
public:
    static const unsigned MaxWeight = 100;
    static const unsigned MinWeight = 1;

    explicit MimeGlobPattern(const QRegExp &regExp, unsigned weight = MaxWeight);
    ~MimeGlobPattern();

    const QRegExp &regExp() const;
    unsigned weight() const;

private:
    QRegExp m_regExp;
    int m_weight;
};

/* Mime type data used in Qt Creator. Contains most information from
 * standard mime type XML database files.
 * Omissions:
 * - Only magic of type "string" is supported. In addition, C++ classes
 *   derived from IMagicMatcher can be added to check on contents
 * - acronyms, language-specific comments
 * Extensions:
 * - List of suffixes and preferred suffix (derived from glob patterns).
 */
class CORE_EXPORT MimeType
{
public:
    MimeType();
    MimeType(const MimeType&);
    MimeType &operator=(const MimeType&);
    ~MimeType();

    void clear();
    bool isNull() const;
    operator bool() const;

    bool isTopLevel() const;

    QString type() const;
    void setType(const QString &type);

    QStringList aliases() const;
    void setAliases(const QStringList &);

    QString comment() const;
    void setComment(const QString &comment);

    QString localeComment(const QString &locale = QString() /* en, de...*/) const;
    void setLocaleComment(const QString &locale, const QString &comment);

    QList<MimeGlobPattern> globPatterns() const;
    void setGlobPatterns(const QList<MimeGlobPattern> &);

    QStringList subClassesOf() const;
    void setSubClassesOf(const QStringList &);

    // Extension over standard mime data
    QStringList suffixes() const;
    void setSuffixes(const QStringList &);

    // Extension over standard mime data
    QString preferredSuffix() const;
    bool setPreferredSuffix(const QString&);

    // Check for type or one of the aliases
    bool matchesType(const QString &type) const;

    // Check glob patterns weights and magic priorities so the highest
    // value is returned. A 0 (zero) indicates no match.
    unsigned matchesFile(const QFileInfo &file) const;

    // Return a filter string usable for a file dialog
    QString filterString() const;

    // Add magic matcher
    void addMagicMatcher(const QSharedPointer<IMagicMatcher> &matcher);

    friend QDebug operator<<(QDebug d, const MimeType &mt);

    static QString formatFilterString(const QString &description, const QList<MimeGlobPattern> &globs);

private:
    explicit MimeType(const MimeTypeData &d);
    unsigned matchesFileBySuffix(Internal::FileMatchContext &c) const;
    unsigned matchesFileByContent(Internal::FileMatchContext &c) const;

    friend class Internal::BaseMimeTypeParser;
    friend class MimeDatabasePrivate;
    QSharedDataPointer<MimeTypeData> m_d;
};

/* A Mime data base to which the plugins can add the mime types they handle.
 * When adding a "text/plain" to it, the mimetype will receive a magic matcher
 * that checks for text files that do not match the globs by heuristics.
 * The class is protected by a QMutex and can therefore be accessed by threads.
 * A good testcase is to run it over '/usr/share/mime/<*>/<*>.xml' on Linux. */

class CORE_EXPORT MimeDatabase
{
    Q_DISABLE_COPY(MimeDatabase)
public:
    MimeDatabase();

    ~MimeDatabase();

    bool addMimeTypes(const QString &fileName, QString *errorMessage);
    bool addMimeTypes(QIODevice *device, QString *errorMessage);
    bool addMimeType(const  MimeType &mt);

    // Returns a mime type or Null one if none found
    MimeType findByType(const QString &type) const;

    // Returns a mime type or Null one if none found
    MimeType findByFile(const QFileInfo &f) const;
    // Convenience that mutex-locks the DB and calls a function
    // of the signature 'void f(const MimeType &, const QFileInfo &, const QString &)'
    // for each filename of a sequence. This avoids locking the DB for each
    // single file.
    template <class Iterator, typename Function>
        inline void findByFile(Iterator i1, const Iterator &i2, Function f) const;

    // Convenience
    QString preferredSuffixByType(const QString &type) const;
    QString preferredSuffixByFile(const QFileInfo &f) const;

    // Return all known suffixes
    QStringList suffixes() const;
    bool setPreferredSuffix(const QString &typeOrAlias, const QString &suffix);

    QStringList filterStrings() const;

    friend QDebug operator<<(QDebug d, const MimeDatabase &mt);

    // returns a string with all the possible file filters, for use with file dialogs
    QString allFiltersString(QString *allFilesFilter = 0) const;
private:
    MimeType findByFileUnlocked(const QFileInfo &f) const;

    MimeDatabasePrivate *m_d;
    mutable QMutex m_mutex;
};

template <class Iterator, typename Function>
    void MimeDatabase::findByFile(Iterator i1, const Iterator &i2, Function f) const
{
    m_mutex.lock();
    for ( ; i1 != i2; ++i1) {
        const QFileInfo fi(*i1);
        f(findByFileUnlocked(fi), fi, *i1);
    }
    m_mutex.unlock();
}

} // namespace Core

#endif // MIMEDATABASE_H
