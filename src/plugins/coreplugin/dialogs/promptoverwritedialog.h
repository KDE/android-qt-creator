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

#ifndef PROMPTOVERWRITEDIALOG_H
#define PROMPTOVERWRITEDIALOG_H

#include <QtGui/QDialog>

QT_BEGIN_NAMESPACE
class QTreeView;
class QStandardItemModel;
class QStandardItem;
class QLabel;
QT_END_NAMESPACE

namespace Core {
namespace Internal {

// Documentation inside.
class PromptOverwriteDialog : public QDialog
{
    Q_OBJECT
public:
    explicit PromptOverwriteDialog(QWidget *parent = 0);

    void setFiles(const QStringList &);

    void setFileEnabled(const QString &f, bool e);
    bool isFileEnabled(const QString &f) const;

    void setFileChecked(const QString &f, bool e);
    bool isFileChecked(const QString &f) const;

    QStringList checkedFiles() const   { return files(Qt::Checked); }
    QStringList uncheckedFiles() const { return files(Qt::Unchecked); }

private:
    QStandardItem *itemForFile(const QString &f) const;
    QStringList files(Qt::CheckState cs) const;

    QLabel *m_label;
    QTreeView *m_view;
    QStandardItemModel *m_model;
};

} // namespace Internal
} // namespace Core

#endif // PROMPTOVERWRITEDIALOG_H
