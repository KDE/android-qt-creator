/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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

#ifndef MAEMOPUBLISHEDPROJECTMODEL_H
#define MAEMOPUBLISHEDPROJECTMODEL_H

#include <QtCore/QSet>
#include <QtCore/QStringList>
#include <QtGui/QFileSystemModel>

namespace RemoteLinux {
namespace Internal {

class MaemoPublishedProjectModel : public QFileSystemModel
{
    Q_OBJECT
public:
    explicit MaemoPublishedProjectModel(QObject *parent = 0);
    void initFilesToExclude();
    QStringList filesToExclude() const { return m_filesToExclude.toList(); }

private:
    virtual int columnCount(const QModelIndex &parent) const;
    virtual int rowCount(const QModelIndex &parent) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation,
        int role) const;
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual bool setData(const QModelIndex &index, const QVariant &value,
        int role);
    virtual Qt::ItemFlags flags(const QModelIndex &index) const;

    void initFilesToExclude(const QString &filePath);

    QSet<QString> m_filesToExclude;
};

} // namespace Internal
} // namespace RemoteLinux

#endif // MAEMOPUBLISHEDPROJECTMODEL_H
