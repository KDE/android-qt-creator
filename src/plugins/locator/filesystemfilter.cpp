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

#include "filesystemfilter.h"
#include "locatorwidget.h"
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/editormanager/editormanager.h>

#include <QtCore/QDir>

using namespace Core;
using namespace Locator;
using namespace Locator::Internal;

FileSystemFilter::FileSystemFilter(EditorManager *editorManager, LocatorWidget *locatorWidget)
        : m_editorManager(editorManager), m_locatorWidget(locatorWidget), m_includeHidden(true)
{
    setShortcutString(QString(QLatin1Char('f')));
    setIncludedByDefault(false);
}

QList<FilterEntry> FileSystemFilter::matchesFor(QFutureInterface<Locator::FilterEntry> &future, const QString &entry)
{
    QList<FilterEntry> value;
    QFileInfo entryInfo(entry);
    QString name = entryInfo.fileName();
    QString directory = entryInfo.path();
    QString filePath = entryInfo.filePath();
    if (entryInfo.isRelative()) {
        if (filePath.startsWith(QLatin1String("~/"))) {
            directory.replace(0, 1, QDir::homePath());
        } else {
            IEditor *editor = m_editorManager->currentEditor();
            if (editor && !editor->file()->fileName().isEmpty()) {
                QFileInfo info(editor->file()->fileName());
                directory.prepend(info.absolutePath() + QLatin1Char('/'));
            }
        }
    }
    QDir dirInfo(directory);
    QDir::Filters dirFilter = QDir::Dirs|QDir::Drives;
    QDir::Filters fileFilter = QDir::Files;
    if (m_includeHidden) {
        dirFilter |= QDir::Hidden;
        fileFilter |= QDir::Hidden;
    }
    QStringList dirs = dirInfo.entryList(dirFilter,
                                      QDir::Name|QDir::IgnoreCase|QDir::LocaleAware);
    QStringList files = dirInfo.entryList(fileFilter,
                                      QDir::Name|QDir::IgnoreCase|QDir::LocaleAware);
    foreach (const QString &dir, dirs) {
        if (future.isCanceled())
            break;
        if (dir != QLatin1String(".") && (name.isEmpty() || dir.startsWith(name, Qt::CaseInsensitive))) {
            FilterEntry filterEntry(this, dir, dirInfo.filePath(dir));
            filterEntry.resolveFileIcon = true;
            value.append(filterEntry);
        }
    }
    foreach (const QString &file, files) {
        if (future.isCanceled())
            break;
        if (name.isEmpty() || file.startsWith(name, Qt::CaseInsensitive)) {
            const QString fullPath = dirInfo.filePath(file);
            FilterEntry filterEntry(this, file, fullPath);
            filterEntry.resolveFileIcon = true;
            value.append(filterEntry);
        }
    }
    return value;
}

void FileSystemFilter::accept(FilterEntry selection) const
{
    QFileInfo info(selection.internalData.toString());
    if (info.isDir()) {
        QString value = shortcutString();
        value += QLatin1Char(' ');
        value += QDir::toNativeSeparators(info.absoluteFilePath() + QLatin1Char('/'));
        m_locatorWidget->show(value, value.length());
        return;
    }
    m_editorManager->openEditor(selection.internalData.toString(), QString(),
                                Core::EditorManager::ModeSwitch);
}

bool FileSystemFilter::openConfigDialog(QWidget *parent, bool &needsRefresh)
{
    Q_UNUSED(needsRefresh)
    Ui::FileSystemFilterOptions ui;
    QDialog dialog(parent);
    ui.setupUi(&dialog);

    ui.hiddenFilesFlag->setChecked(m_includeHidden);
    ui.limitCheck->setChecked(!isIncludedByDefault());
    ui.shortcutEdit->setText(shortcutString());

    if (dialog.exec() == QDialog::Accepted) {
        m_includeHidden = ui.hiddenFilesFlag->isChecked();
        setShortcutString(ui.shortcutEdit->text().trimmed());
        setIncludedByDefault(!ui.limitCheck->isChecked());
        return true;
    }
    return false;
}

QByteArray FileSystemFilter::saveState() const
{
    QByteArray value;
    QDataStream out(&value, QIODevice::WriteOnly);
    out << m_includeHidden;
    out << shortcutString();
    out << isIncludedByDefault();
    return value;
}

bool FileSystemFilter::restoreState(const QByteArray &state)
{
    QDataStream in(state);
    in >> m_includeHidden;

    // An attempt to prevent setting this on old configuration
    if (!in.atEnd()) {
        QString shortcut;
        bool defaultFilter;
        in >> shortcut;
        in >> defaultFilter;
        setShortcutString(shortcut);
        setIncludedByDefault(defaultFilter);
    }

    return true;
}
