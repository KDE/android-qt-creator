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

#ifndef UNDO_COMMANDS_H
#define UNDO_COMMANDS_H

#include "resourceview.h"

#include <QtCore/QString>
#include <QtGui/QUndoCommand>

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

namespace SharedTools {

/*!
    \class ViewCommand

    Provides a base for \l ResourceView-related commands.
*/
class ViewCommand : public QUndoCommand
{
protected:
    ResourceView *m_view;

    ViewCommand(ResourceView *view);
    virtual ~ViewCommand();
};

/*!
    \class ModelIndexViewCommand

    Provides a mean to store/restore a \l QModelIndex as it cannot
    be stored safely in most cases. This is an abstract class.
*/
class ModelIndexViewCommand : public ViewCommand
{
    int m_prefixArrayIndex;
    int m_fileArrayIndex;

protected:
    ModelIndexViewCommand(ResourceView *view);
    virtual ~ModelIndexViewCommand();
    void storeIndex(const QModelIndex &index);
    QModelIndex makeIndex() const;
};

/*!
    \class ModifyPropertyCommand

    Modifies the name/prefix/language property of a prefix/file node.
*/
class ModifyPropertyCommand : public ModelIndexViewCommand
{
    ResourceView::NodeProperty m_property;
    QString m_before;
    QString m_after;
    int m_mergeId;

public:
    ModifyPropertyCommand(ResourceView *view, const QModelIndex &nodeIndex,
            ResourceView::NodeProperty property, const int mergeId, const QString &before,
            const QString &after = QString());

private:
    int id() const { return m_mergeId; }
    bool mergeWith(const QUndoCommand * command);
    void undo();
    void redo();
};

/*!
    \class RemoveEntryCommand

    Removes a \l QModelIndex including all children from a \l ResourceView.
*/
class RemoveEntryCommand : public ModelIndexViewCommand
{
    EntryBackup *m_entry;
    bool m_isExpanded;

public:
    RemoveEntryCommand(ResourceView *view, const QModelIndex &index);
    ~RemoveEntryCommand();

private:
    void redo();
    void undo();
    void freeEntry();
};

/*!
    \class AddFilesCommand

    Adds a list of files to a given prefix node.
*/
class AddFilesCommand : public ViewCommand
{
    int m_prefixIndex;
    int m_cursorFileIndex;
    int m_firstFile;
    int m_lastFile;
    const QStringList m_fileNames;

public:
    AddFilesCommand(ResourceView *view, int prefixIndex, int cursorFileIndex,
            const QStringList &fileNames);

private:
    void redo();
    void undo();
};

/*!
    \class AddEmptyPrefixCommand

    Adds a new, empty prefix node.
*/
class AddEmptyPrefixCommand : public ViewCommand
{
    int m_prefixArrayIndex;

public:
    AddEmptyPrefixCommand(ResourceView *view);

private:
    void redo();
    void undo();
};

} // namespace SharedTools

#endif // UNDO_COMMANDS_H
