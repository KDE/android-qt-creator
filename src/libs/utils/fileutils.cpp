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

#include "fileutils.h"
#include "savefile.h"

#include "qtcassert.h"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QTemporaryFile>
#include <QtCore/QDateTime>
#include <QtCore/QDataStream>
#include <QtCore/QTextStream>
#include <QtGui/QMessageBox>

namespace Utils {

/*! \class Utils::FileUtils

  \brief File- and directory-related convenience functions.

  File- and directory-related convenience functions.
*/

/*!
  Removes the directory \a filePath and its subdirectories recursively.

  \note The \a error parameter is optional.

  \return Whether the operation succeeded.
*/
bool FileUtils::removeRecursively(const QString &filePath, QString *error)
{
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists())
        return true;
    QFile::setPermissions(filePath, fileInfo.permissions() | QFile::WriteUser);
    if (fileInfo.isDir()) {
        QDir dir(filePath);
        dir = dir.canonicalPath();
        if (dir.isRoot()) {
            if (error) {
                *error = QCoreApplication::translate("Utils::FileUtils",
                    "Refusing to remove root directory.");
            }
            return false;
        }
        if (dir.path() == QDir::home().canonicalPath()) {
            if (error) {
                *error = QCoreApplication::translate("Utils::FileUtils",
                    "Refusing to remove your home directory.");
            }
            return false;
        }

        QStringList fileNames = dir.entryList(QDir::Files | QDir::Hidden
                                              | QDir::System | QDir::Dirs | QDir::NoDotAndDotDot);
        foreach (const QString &fileName, fileNames) {
            if (!removeRecursively(filePath + QLatin1Char('/') + fileName, error))
                return false;
        }
        if (!QDir::root().rmdir(dir.path())) {
            if (error) {
                *error = QCoreApplication::translate("Utils::FileUtils", "Failed to remove directory '%1'.")
                        .arg(QDir::toNativeSeparators(filePath));
            }
            return false;
        }
    } else {
        if (!QFile::remove(filePath)) {
            if (error) {
                *error = QCoreApplication::translate("Utils::FileUtils", "Failed to remove file '%1'.")
                        .arg(QDir::toNativeSeparators(filePath));
            }
            return false;
        }
    }
    return true;
}

/*!
  Copies the directory specified by \a srcFilePath recursively to \a tgtFilePath. \a tgtFilePath will contain
  the target directory, which will be created. Example usage:

  \code
    QString error;
    book ok = Utils::FileUtils::copyRecursively("/foo/bar", "/foo/baz", &error);
    if (!ok)
      qDebug() << error;
  \endcode

  This will copy the contents of /foo/bar into to the baz directory under /foo, which will be created in the process.

  \note The \a error parameter is optional.

  \return Whether the operation succeeded.
*/
bool FileUtils::copyRecursively(const QString &srcFilePath,
                     const QString &tgtFilePath, QString *error)
{
    QFileInfo srcFileInfo(srcFilePath);
    if (srcFileInfo.isDir()) {
        QDir targetDir(tgtFilePath);
        targetDir.cdUp();
        if (!targetDir.mkdir(QFileInfo(tgtFilePath).fileName())) {
            if (error) {
                *error = QCoreApplication::translate("Utils::FileUtils", "Failed to create directory '%1'.")
                        .arg(QDir::toNativeSeparators(tgtFilePath));
                return false;
            }
        }
        QDir sourceDir(srcFilePath);
        QStringList fileNames = sourceDir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);
        foreach (const QString &fileName, fileNames) {
            const QString newSrcFilePath
                    = srcFilePath + QLatin1Char('/') + fileName;
            const QString newTgtFilePath
                    = tgtFilePath + QLatin1Char('/') + fileName;
            if (!copyRecursively(newSrcFilePath, newTgtFilePath, error))
                return false;
        }
    } else {
        if (!QFile::copy(srcFilePath, tgtFilePath)) {
            if (error) {
                *error = QCoreApplication::translate("Utils::FileUtils", "Could not copy file '%1' to '%2'.")
                        .arg(QDir::toNativeSeparators(srcFilePath),
                             QDir::toNativeSeparators(tgtFilePath));
            }
            return false;
        }
    }
    return true;
}

/*!
  If \a filePath is a directory, the function will recursively check all files and return
  true if one of them is newer than \a timeStamp. If \a filePath is a single file, true will
  be returned if the file is newer than \timeStamp.

  \return Whether at least one file in \a filePath has a newer date than \a timeStamp.
*/
bool FileUtils::isFileNewerThan(const QString &filePath,
    const QDateTime &timeStamp)
{
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists() || fileInfo.lastModified() >= timeStamp)
        return true;
    if (fileInfo.isDir()) {
        const QStringList dirContents = QDir(filePath)
            .entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
        foreach (const QString &curFileName, dirContents) {
            const QString curFilePath
                = filePath + QLatin1Char('/') + curFileName;
            if (isFileNewerThan(curFilePath, timeStamp))
                return true;
        }
    }
    return false;
}

/*!
  Recursively resolve possibly present symlinks in \a filePath.
  Unlike QFileInfo::canonicalFilePath(), this function will still return the expected target file
  even if the symlink is dangling.

  \note Maximum recursion depth == 16.

  return Symlink target file path.
*/
QString FileUtils::resolveSymlinks(const QString &path)
{
    QFileInfo f(path);
    int links = 16;
    while (links-- && f.isSymLink())
        f.setFile(f.symLinkTarget());
    if (links <= 0)
        return QString();
    return f.filePath();
}


QByteArray FileReader::fetchQrc(const QString &fileName)
{
    QTC_ASSERT(fileName.startsWith(QLatin1Char(':')), return QByteArray())
    QFile file(fileName);
    bool ok = file.open(QIODevice::ReadOnly);
    QTC_ASSERT(ok, qWarning() << fileName << "not there!"; return QByteArray())
    return file.readAll();
}

bool FileReader::fetch(const QString &fileName, QIODevice::OpenMode mode)
{
    QTC_ASSERT(!(mode & ~(QIODevice::ReadOnly | QIODevice::Text)), return false)

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | mode)) {
        m_errorString = tr("Cannot open %1 for reading: %2").arg(
                QDir::toNativeSeparators(fileName), file.errorString());
        return false;
    }
    m_data = file.readAll();
    if (file.error() != QFile::NoError) {
        m_errorString = tr("Cannot read %1: %2").arg(
                QDir::toNativeSeparators(fileName), file.errorString());
        return false;
    }
    return true;
}

bool FileReader::fetch(const QString &fileName, QIODevice::OpenMode mode, QString *errorString)
{
    if (fetch(fileName, mode))
        return true;
    if (errorString)
        *errorString = m_errorString;
    return false;
}

bool FileReader::fetch(const QString &fileName, QIODevice::OpenMode mode, QWidget *parent)
{
    if (fetch(fileName, mode))
        return true;
    if (parent)
        QMessageBox::critical(parent, tr("File Error"), m_errorString);
    return false;
}


FileSaverBase::FileSaverBase()
    : m_hasError(false)
{
}

FileSaverBase::~FileSaverBase()
{
    delete m_file;
}

bool FileSaverBase::finalize()
{
    m_file->close();
    setResult(m_file->error() == QFile::NoError);
    // We delete the object, so it is really closed even if it is a QTemporaryFile.
    delete m_file;
    m_file = 0;
    return !m_hasError;
}

bool FileSaverBase::finalize(QString *errStr)
{
    if (finalize())
        return true;
    if (errStr)
        *errStr = errorString();
    return false;
}

bool FileSaverBase::finalize(QWidget *parent)
{
    if (finalize())
        return true;
    QMessageBox::critical(parent, tr("File Error"), errorString());
    return false;
}

bool FileSaverBase::write(const char *data, int len)
{
    if (m_hasError)
        return false;
    return setResult(m_file->write(data, len) == len);
}

bool FileSaverBase::write(const QByteArray &bytes)
{
    if (m_hasError)
        return false;
    return setResult(m_file->write(bytes) == bytes.count());
}

bool FileSaverBase::setResult(bool ok)
{
    if (!ok && !m_hasError) {
        m_errorString = tr("Cannot write file %1. Disk full?").arg(
                QDir::toNativeSeparators(m_fileName));
        m_hasError = true;
    }
    return ok;
}

bool FileSaverBase::setResult(QTextStream *stream)
{
#if QT_VERSION >= QT_VERSION_CHECK(4, 8, 0)
    stream->flush();
    return setResult(stream->status() == QTextStream::Ok);
#else
    Q_UNUSED(stream)
    return true;
#endif
}

bool FileSaverBase::setResult(QDataStream *stream)
{
#if QT_VERSION >= QT_VERSION_CHECK(4, 8, 0)
    return setResult(stream->status() == QDataStream::Ok);
#else
    Q_UNUSED(stream)
    return true;
#endif
}

bool FileSaverBase::setResult(QXmlStreamWriter *stream)
{
#if QT_VERSION >= QT_VERSION_CHECK(4, 8, 0)
    return setResult(!stream->hasError());
#else
    Q_UNUSED(stream)
    return true;
#endif
}


FileSaver::FileSaver(const QString &filename, QIODevice::OpenMode mode)
{
    m_fileName = filename;
    if (mode & (QIODevice::ReadOnly | QIODevice::Append)) {
        m_file = new QFile(filename);
        m_isSafe = false;
    } else {
        m_file = new SaveFile(filename);
        m_isSafe = true;
    }
    if (!m_file->open(QIODevice::WriteOnly | mode)) {
        QString err = QFile::exists(filename) ?
                tr("Cannot overwrite file %1: %2") : tr("Cannot create file %1: %2");
        m_errorString = err.arg(QDir::toNativeSeparators(filename), m_file->errorString());
        m_hasError = true;
    }
}

bool FileSaver::finalize()
{
    if (!m_isSafe)
        return FileSaverBase::finalize();

    SaveFile *sf = static_cast<SaveFile *>(m_file);
    if (m_hasError)
        sf->rollback();
    else
        setResult(sf->commit());
    delete sf;
    m_file = 0;
    return !m_hasError;
}

TempFileSaver::TempFileSaver(const QString &templ)
    : m_autoRemove(true)
{
    QTemporaryFile *tempFile = new QTemporaryFile();
    if (!templ.isEmpty())
        tempFile->setFileTemplate(templ);
    tempFile->setAutoRemove(false);
    if (!tempFile->open()) {
        m_errorString = tr("Cannot create temporary file in %1: %2").arg(
                QDir::toNativeSeparators(QFileInfo(tempFile->fileTemplate()).absolutePath()),
                tempFile->errorString());
        m_hasError = true;
    }
    m_file = tempFile;
    m_fileName = tempFile->fileName();
}

TempFileSaver::~TempFileSaver()
{
    delete m_file;
    m_file = 0;
    if (m_autoRemove)
        QFile::remove(m_fileName);
}

/*! \class Utils::FileName

    \brief A light-weight convenience class for filenames

    On windows filenames are compared case insensitively.
*/


#ifdef Q_OS_WIN
Qt::CaseSensitivity FileName::cs = Qt::CaseInsensitive;
#else
Qt::CaseSensitivity FileName::cs = Qt::CaseSensitive;
#endif

FileName::FileName()
    : QString()
{

}

/// Constructs a FileName from \a info
FileName::FileName(const QFileInfo &info)
    : QString(info.absoluteFilePath())
{
}

/// \returns a QFileInfo
QFileInfo FileName::toFileInfo() const
{
    return QFileInfo(*this);
}

/// \returns a QString for passing on to QString based APIs
QString FileName::toString() const
{
    return QString(*this);
}

/// \returns a QString to display to the user
/// Converts the separators to the native format
QString FileName::toUserOutput() const
{
    return QDir::toNativeSeparators(toString());
}

/// Constructs a FileName from \a fileName
/// \a fileName is not checked for validity.
FileName FileName::fromString(const QString &filename)
{
    return FileName(filename);
}

/// Constructs a FileName from \a fileName
/// \a fileName is only passed through QDir::cleanPath
/// and QDir::fromNativeSeparators
FileName FileName::fromUserInput(const QString &filename)
{
    return FileName(QDir::cleanPath(QDir::fromNativeSeparators(filename)));
}

FileName::FileName(const QString &string)
    : QString(string)
{

}

bool FileName::operator==(const FileName &other) const
{
    return QString::compare(*this, other, cs) == 0;
}

bool FileName::operator!=(const FileName &other) const
{
    return !(*this == other);
}

bool FileName::operator<(const FileName &other) const
{
    return QString::compare(*this, other, cs) < 0;
}

bool FileName::operator<=(const FileName &other) const
{
    return QString::compare(*this, other, cs) <= 0;
}

bool FileName::operator>(const FileName &other) const
{
    return other < *this;
}

bool FileName::operator>=(const FileName &other) const
{
    return other <= *this;
}

/// \returns whether FileName is a child of \a s
bool FileName::isChildOf(const FileName &s) const
{
    if (!QString::startsWith(s, cs))
        return false;
    if (size() <= s.size())
        return false;
    return at(s.size()) == QLatin1Char('/');
}

/// \returns whether FileName endsWith \a s
bool FileName::endsWith(const QString &s) const
{
    return QString::endsWith(s, cs);
}

/// \returns the relativeChildPath of FileName to parent if FileName is a child of parent
/// \note returns a empty FileName if FileName is not a child of parent
/// That is, this never returns a path starting with "../"
FileName FileName::relativeChildPath(const FileName &parent) const
{
    if (!isChildOf(parent))
        return Utils::FileName();
    return FileName(QString::mid(parent.size() + 1, -1));
}

/// Appends \a s, ensuring a / between the parts
FileName &FileName::appendPath(const QString &s)
{
    if (QString::endsWith(QLatin1Char('/')))
        append(QLatin1Char('/'));
    append(s);
    return *this;
}

FileName &FileName::append(const QString &str)
{
    QString::append(str);
    return *this;
}

FileName &FileName::append(QChar str)
{
    QString::append(str);
    return *this;
}

} // namespace Utils

QT_BEGIN_NAMESPACE
uint qHash(const Utils::FileName &a)
{
#ifdef Q_OS_WIN
    return qHash(a.toString().toUpper());
#else
    return qHash(a.toString());
#endif
}
QT_END_NAMESPACE
