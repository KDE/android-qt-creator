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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef SUBMITEDITORWIDGET_H
#define SUBMITEDITORWIDGET_H

#include "utils_global.h"

#include <QtGui/QWidget>
#include <QtGui/QAbstractItemView>

QT_BEGIN_NAMESPACE
class QTextEdit;
class QListWidgetItem;
class QAction;
class QAbstractItemModel;
class QModelIndex;
class QLineEdit;
QT_END_NAMESPACE

namespace Utils {

class SubmitFieldWidget;
struct SubmitEditorWidgetPrivate;

class QTCREATOR_UTILS_EXPORT SubmitEditorWidget : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY(SubmitEditorWidget)
    Q_PROPERTY(QString descriptionText READ descriptionText WRITE setDescriptionText DESIGNABLE true)
    Q_PROPERTY(int fileNameColumn READ fileNameColumn WRITE setFileNameColumn DESIGNABLE false)
    Q_PROPERTY(QAbstractItemView::SelectionMode fileListSelectionMode READ fileListSelectionMode WRITE setFileListSelectionMode DESIGNABLE true)
    Q_PROPERTY(bool lineWrap READ lineWrap WRITE setLineWrap DESIGNABLE true)
    Q_PROPERTY(int lineWrapWidth READ lineWrapWidth WRITE setLineWrapWidth DESIGNABLE true)
    Q_PROPERTY(bool emptyFileListEnabled READ isEmptyFileListEnabled WRITE setEmptyFileListEnabled DESIGNABLE true)

public:
    explicit SubmitEditorWidget(QWidget *parent = 0);
    virtual ~SubmitEditorWidget();

    // Register/Unregister actions that are managed by ActionManager with this widget.
    // The submit action should have Core::Command::CA_UpdateText set as its text will
    // be updated.
    void registerActions(QAction *editorUndoAction,  QAction *editorRedoAction,
                         QAction *submitAction = 0, QAction *diffAction = 0);
    void unregisterActions(QAction *editorUndoAction,  QAction *editorRedoAction,
                           QAction *submitAction = 0, QAction *diffAction = 0);

    QString descriptionText() const;
    void setDescriptionText(const QString &text);

    // 'Commit' action enabled despite empty file list
    bool isEmptyFileListEnabled() const;
    void setEmptyFileListEnabled(bool e);

    int fileNameColumn() const;
    void setFileNameColumn(int c);

    bool lineWrap() const;
    void setLineWrap(bool);

    int lineWrapWidth() const;
    void setLineWrapWidth(int);

    QAbstractItemView::SelectionMode fileListSelectionMode() const;
    void setFileListSelectionMode(QAbstractItemView::SelectionMode sm);

    void setFileModel(QAbstractItemModel *model);
    QAbstractItemModel *fileModel() const;

    // Files to be included in submit
    QStringList checkedFiles() const;

    // Selected files for diff
    QStringList selectedFiles() const;

    QTextEdit *descriptionEdit() const;

    void addDescriptionEditContextMenuAction(QAction *a);
    void insertDescriptionEditContextMenuAction(int pos, QAction *a);

    void addSubmitFieldWidget(SubmitFieldWidget *f);
    QList<SubmitFieldWidget *> submitFieldWidgets() const;

    virtual bool canSubmit() const;

signals:
    void diffSelected(const QStringList &);
    void fileSelectionChanged(bool someFileSelected);
    void submitActionTextChanged(const QString &);
    void submitActionEnabledChanged(const bool);

public slots:
    void checkAll();
    void uncheckAll();

protected:
    virtual QString cleanupDescription(const QString &) const;
    virtual void changeEvent(QEvent *e);
    void insertTopWidget(QWidget *w);

protected slots:
    void updateSubmitAction();

private slots:
    void triggerDiffSelected();
    void diffActivated(const QModelIndex &index);
    void diffActivatedDelayed();
    void updateActions();
    void updateDiffAction();
    void editorCustomContextMenuRequested(const QPoint &);
    void fileListCustomContextMenuRequested(const QPoint & pos);

private:
    bool hasSelection() const;
    unsigned checkedFilesCount() const;

    SubmitEditorWidgetPrivate *m_d;
};

} // namespace Utils

#endif // SUBMITEDITORWIDGET_H
