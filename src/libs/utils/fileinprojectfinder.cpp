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

#include "fileinprojectfinder.h"
#include <utils/qtcassert.h>

#include <QtCore/QFileInfo>
#include <QtCore/QUrl>

namespace Utils {

/*!
  \class Utils::FileInProjectFinder

  \brief Helper class to find the 'original' file in the project directory for a given file url.

  Often files are copied in the build + deploy process. findFile() searches for an existing file
  in the project directory for a given file path:

  E.g. following file paths:
  \list
  \i C:/app-build-desktop/qml/app/main.qml (shadow build directory)
  \i C:/Private/e3026d63/qml/app/main.qml  (Application data folder on Symbian device)
  \i /Users/x/app-build-desktop/App.app/Contents/Resources/qml/App/main.qml (folder on Mac OS X)
  \endlist

 should all be mapped to $PROJECTDIR/qml/app/main.qml
*/

FileInProjectFinder::FileInProjectFinder()
{
}

static QString stripTrailingSlashes(const QString &path)
{
    QString newPath = path;
    while (newPath.endsWith(QLatin1Char('/')))
        newPath.remove(newPath.length() - 1, 1);
    return newPath;
}

void FileInProjectFinder::setProjectDirectory(const QString &absoluteProjectPath)
{
    const QString newProjectPath = stripTrailingSlashes(absoluteProjectPath);

    if (newProjectPath == m_projectDir)
        return;

    const QFileInfo infoPath(newProjectPath);
    QTC_CHECK(newProjectPath.isEmpty()
              || (infoPath.exists() && infoPath.isAbsolute()));

    m_projectDir = newProjectPath;
    m_cache.clear();
}

QString FileInProjectFinder::projectDirectory() const
{
    return m_projectDir;
}

void FileInProjectFinder::setProjectFiles(const QStringList &projectFiles)
{
    if (m_projectFiles == projectFiles)
        return;

    m_projectFiles = projectFiles;
    m_cache.clear();
}

void FileInProjectFinder::setSysroot(const QString &sysroot)
{
    QString newsys = sysroot;
    while (newsys.endsWith(QLatin1Char('/')))
        newsys.remove(newsys.length() - 1, 1);

    if (m_sysroot == newsys)
        return;

    m_sysroot = newsys;
    m_cache.clear();
}

/**
  Returns the best match for the given file url in the project directory.

  The method first checks whether the file inside the project directory exists.
  If not, the leading directory in the path is stripped, and the - now shorter - path is
  checked for existence, and so on. Second, it tries to locate the file in the sysroot
  folder specified. Third, we walk the list of project files, and search for a file name match
  there. If all fails, it returns the original path from the file url.
  */
QString FileInProjectFinder::findFile(const QUrl &fileUrl, bool *success) const
{
    QString originalPath = fileUrl.toLocalFile();
    if (originalPath.isEmpty()) // e.g. qrc://
        originalPath = fileUrl.path();

    if (originalPath.isEmpty()) {
        if (success)
            *success = false;
        return originalPath;
    }

    if (!m_projectDir.isEmpty()) {
        int prefixToIgnore = -1;
        const QChar separator = QLatin1Char('/');
        if (originalPath.startsWith(m_projectDir + separator)) {
#ifdef Q_OS_MAC
            // starting with the project path is not sufficient if the file was
            // copied in an insource build, e.g. into MyApp.app/Contents/Resources
            static const QString appResourcePath = QString::fromLatin1(".app/Contents/Resources");
            if (originalPath.contains(appResourcePath)) {
                // the path is inside the project, but most probably as a resource of an insource build
                // so ignore that path
                prefixToIgnore = originalPath.indexOf(appResourcePath) + appResourcePath.length();
            } else {
#endif
                if (success)
                    *success = true;
                return originalPath;
#ifdef Q_OS_MAC
            }
#endif
        }

        if (m_cache.contains(originalPath)) {
            // check if cached path is still there
            QString candidate = m_cache.value(originalPath);
            QFileInfo candidateInfo(candidate);
            if (candidateInfo.exists() && candidateInfo.isFile()) {
                if (success)
                    *success = true;
                return candidate;
            }
        }

        // Strip directories one by one from the beginning of the path,
        // and see if the new relative path exists in the build directory.
        if (prefixToIgnore < 0) {
            if (!QFileInfo(originalPath).isAbsolute()
                    && !originalPath.startsWith(separator)) {
                prefixToIgnore = 0;
            } else {
                prefixToIgnore = originalPath.indexOf(separator);
            }
        }
        while (prefixToIgnore != -1) {
            QString candidate = originalPath;
            candidate.remove(0, prefixToIgnore);
            candidate.prepend(m_projectDir);
            QFileInfo candidateInfo(candidate);
            if (candidateInfo.exists() && candidateInfo.isFile()) {
                if (success)
                    *success = true;

                m_cache.insert(originalPath, candidate);
                return candidate;
            }
            prefixToIgnore = originalPath.indexOf(separator, prefixToIgnore + 1);
        }
    }

    // find (solely by filename) in project files
    const QString fileName = QFileInfo(originalPath).fileName();
    foreach (const QString &f, m_projectFiles) {
        if (QFileInfo(f).fileName() == fileName) {
            m_cache.insert(originalPath, f);
            if (success)
                *success = true;
            return f;
        }
    }

    // check if absolute path is found in sysroot
    if (!m_sysroot.isEmpty()) {
        const QString sysrootPath = m_sysroot + originalPath;
        if (QFileInfo(sysrootPath).exists() && QFileInfo(sysrootPath).isFile()) {
            if (success)
                *success = true;
            m_cache.insert(originalPath, sysrootPath);
            return sysrootPath;
        }
    }

    if (success)
        *success = false;

    return originalPath;
}

} // namespace Utils
