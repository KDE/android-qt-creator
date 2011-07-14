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

#ifndef OPENEDITORSMODEL_H
#define OPENEDITORSMODEL_H

#include "../core_global.h"

#include <QtCore/QAbstractItemModel>
#include <QtCore/QScopedPointer>

QT_FORWARD_DECLARE_CLASS(QIcon)

namespace Core {

struct OpenEditorsModelPrivate;
class IEditor;
class IFile;

class CORE_EXPORT OpenEditorsModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit OpenEditorsModel(QObject *parent);
    virtual ~OpenEditorsModel();

    QIcon lockedIcon() const;
    QIcon unlockedIcon() const;

    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QModelIndex parent(const QModelIndex &/*index*/) const { return QModelIndex(); }
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QModelIndex index(int row, int column = 0, const QModelIndex &parent = QModelIndex()) const;

    void addEditor(IEditor *editor, bool isDuplicate = false);
    void addRestoredEditor(const QString &fileName, const QString &displayName, const QString &id);
    QModelIndex firstRestoredEditor() const;

    struct CORE_EXPORT Entry {
        Entry();
        IEditor *editor;
        QString fileName() const;
        QString displayName() const;
        QString id() const;
        QString m_fileName;
        QString m_displayName;
        QString m_id;
    };
    QList<Entry> entries() const;

    IEditor *editorAt(int row) const;

    void removeEditor(IEditor *editor);
    void removeEditor(const QModelIndex &index);
    void removeEditor(const QString &fileName);

    void removeAllRestoredEditors();
    void emitDataChanged(IEditor *editor);

    QList<IEditor *> editors() const;
    QList<Entry> restoredEditors() const;
    bool isDuplicate(IEditor *editor) const;
    QList<IEditor *> duplicatesFor(IEditor *editor) const;
    IEditor *originalForDuplicate(IEditor *duplicate) const;
    void makeOriginal(IEditor *duplicate);
    QModelIndex indexOf(IEditor *editor) const;

    QString displayNameForFile(IFile *file) const;

private slots:
    void itemChanged();

private:
    void addEntry(const Entry &entry);
    int findEditor(IEditor *editor) const;
    int findFileName(const QString &filename) const;
    void removeEditor(int idx);

    QScopedPointer<OpenEditorsModelPrivate> d;
};

} // namespace Core

#endif // OPENEDITORSMODEL_H
