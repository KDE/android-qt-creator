/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#include "bookmark.h"
#include "bookmarkmanager.h"

#include <QtCore/QDebug>
#include <QtGui/QTextBlock>

using namespace Bookmarks::Internal;

Bookmark::Bookmark(const QString& fileName, int lineNumber, BookmarkManager *manager) :
    BaseTextMark(fileName, lineNumber),
    m_manager(manager),
    m_fileInfo(fileName),
    m_fileName(fileName),
    m_onlyFile(m_fileInfo.fileName()),
    m_path(m_fileInfo.path()),
    m_lineNumber(lineNumber)
{
}

QIcon Bookmark::icon() const
{
    return m_manager->bookmarkIcon();
}

void Bookmark::removedFromEditor()
{
    m_manager->removeBookmark(this);
}

void Bookmark::updateLineNumber(int lineNumber)
{
    if (lineNumber != m_lineNumber) {
        m_lineNumber = lineNumber;
        m_manager->updateBookmark(this);
    }
}

void Bookmark::updateBlock(const QTextBlock &block)
{
    if (m_lineText != block.text()) {
        m_lineText = block.text();
        m_manager->updateBookmark(this);
    }
}

QString Bookmark::lineText() const
{
    return m_lineText;
}

QString Bookmark::filePath() const
{
    return m_fileName;
}

QString Bookmark::fileName() const
{
    return m_onlyFile;
}

QString Bookmark::path() const
{
    return m_path;
}
