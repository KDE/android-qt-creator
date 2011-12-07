/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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

#include "maemopublishedprojectmodel.h"

#include <QtCore/QFileInfo>

namespace Madde {
namespace Internal {
namespace {
const int IncludeColumn = 2;
} // anonymous namespace

MaemoPublishedProjectModel::MaemoPublishedProjectModel(QObject *parent)
    : QFileSystemModel(parent)
{
    setFilter(filter() | QDir::Hidden | QDir::System);
}

void MaemoPublishedProjectModel::initFilesToExclude()
{
    initFilesToExclude(rootPath());
}

void MaemoPublishedProjectModel::initFilesToExclude(const QString &filePath)
{
    QFileInfo fi(filePath);
    if (fi.isDir()) {
        const QStringList fileNames = QDir(filePath).entryList(QDir::Files
            | QDir::Dirs | QDir::NoDotAndDotDot | QDir::System | QDir::Hidden);
        foreach (const QString &fileName, fileNames)
            initFilesToExclude(filePath + QLatin1Char('/') + fileName);
    } else {
        const QString &fileName = fi.fileName();
        if (fi.isHidden() || fileName.endsWith(QLatin1String(".o"))
                || fileName == QLatin1String("Makefile")
                || fileName.contains(QLatin1String(".pro.user"))
                || fileName.contains(QLatin1String(".so"))
                || fileName.endsWith(QLatin1String(".a"))) {
            m_filesToExclude.insert(filePath);
        }
    }
}

int MaemoPublishedProjectModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return IncludeColumn + 1;
}

int MaemoPublishedProjectModel::rowCount(const QModelIndex &parent) const
{
    if (isDir(parent) && m_filesToExclude.contains(filePath(parent)))
        return 0;
    return QFileSystemModel::rowCount(parent);
}

QVariant MaemoPublishedProjectModel::headerData(int section,
    Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || section != IncludeColumn)
        return QFileSystemModel::headerData(section, orientation, role);
    return tr("Include in package");
}

Qt::ItemFlags MaemoPublishedProjectModel::flags(const QModelIndex &index) const
{
    if (index.column() != IncludeColumn)
        return QFileSystemModel::flags(index);
    return Qt::ItemIsEnabled | Qt::ItemIsUserCheckable;
}

QVariant MaemoPublishedProjectModel::data(const QModelIndex &index,
    int role) const
{
    if (index.column() != IncludeColumn)
        return QFileSystemModel::data(index, role);
    const bool include = !m_filesToExclude.contains(filePath(index));
    if (role == Qt::DisplayRole)
        return include ? tr("Include") : tr("Do not include");
    else if (role == Qt::CheckStateRole)
        return include ? Qt::Checked : Qt::Unchecked;
    else
        return QVariant();
}

bool MaemoPublishedProjectModel::setData(const QModelIndex &index,
    const QVariant &value, int role)
{
    if (index.column() != IncludeColumn)
        return QFileSystemModel::setData(index, value, role);
    if (role == Qt::CheckStateRole) {
        if (value == Qt::Checked) {
            m_filesToExclude.remove(filePath(index));
        } else {
            m_filesToExclude.insert(filePath(index));
        }
        if (isDir(index))
            emit layoutChanged();
        return true;
    }
    return false;
}


} // namespace Internal
} // namespace Madde
