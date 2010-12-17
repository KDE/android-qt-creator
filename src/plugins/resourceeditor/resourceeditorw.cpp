/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "resourceeditorw.h"
#include "resourceeditorplugin.h"
#include "resourceeditorconstants.h"

#include <qrceditor.h>

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>
#include <utils/reloadpromptutils.h>

#include <QtCore/QTemporaryFile>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/qdebug.h>
#include <QtGui/QMainWindow>
#include <QtGui/QHBoxLayout>

namespace ResourceEditor {
namespace Internal {

enum { debugResourceEditorW = 0 };



ResourceEditorFile::ResourceEditorFile(ResourceEditorW *parent) :
    IFile(parent),
    m_mimeType(QLatin1String(ResourceEditor::Constants::C_RESOURCE_MIMETYPE)),
    m_parent(parent)
{
    if (debugResourceEditorW)
        qDebug() <<  "ResourceEditorFile::ResourceEditorFile()";
}

QString ResourceEditorFile::mimeType() const
{
    return m_mimeType;
}


ResourceEditorW::ResourceEditorW(const Core::Context &context,
                               ResourceEditorPlugin *plugin,
                               QWidget *parent)
        : m_context(context),
        m_resourceEditor(new SharedTools::QrcEditor(parent)),
        m_resourceFile(new ResourceEditorFile(this)),
        m_plugin(plugin)
{
    m_resourceEditor->setResourceDragEnabled(true);

    connect(m_resourceEditor, SIGNAL(dirtyChanged(bool)), this, SLOT(dirtyChanged(bool)));
    connect(m_resourceEditor, SIGNAL(undoStackChanged(bool, bool)),
            this, SLOT(onUndoStackChanged(bool, bool)));
    connect(m_resourceFile, SIGNAL(changed()), this, SIGNAL(changed()));
    if (debugResourceEditorW)
        qDebug() <<  "ResourceEditorW::ResourceEditorW()";
}

ResourceEditorW::~ResourceEditorW()
{
    if (m_resourceEditor)
        m_resourceEditor->deleteLater();
}

bool ResourceEditorW::createNew(const QString &contents)
{
    QTemporaryFile tempFile(0);
    tempFile.setAutoRemove(true);
    if (!tempFile.open())
        return false;
    const QString tempFileName =  tempFile.fileName();
    tempFile.write(contents.toUtf8());
    tempFile.close();

    const bool rc = m_resourceEditor->load(tempFileName);
    m_resourceEditor->setFileName(QString());
    if (debugResourceEditorW)
        qDebug() <<  "ResourceEditorW::createNew: " << contents << " (" << tempFileName << ") returns " << rc;
    return rc;
}

bool ResourceEditorW::open(const QString &fileName /* = QString() */)
{
    if (debugResourceEditorW)
        qDebug() <<  "ResourceEditorW::open: " << fileName;

    if (fileName.isEmpty()) {
        setDisplayName(tr("untitled"));
        return true;
    }

    const QFileInfo fi(fileName);

    const QString absFileName = fi.absoluteFilePath();

    if (!fi.isReadable())
        return false;

    if (!m_resourceEditor->load(absFileName))
        return false;

    setDisplayName(fi.fileName());

    emit changed();
    return true;
}

bool ResourceEditorFile::save(const QString &name /* = QString() */)
{
    if (debugResourceEditorW)
        qDebug(">ResourceEditorW::save: %s", qPrintable(name));

    const QString oldFileName = fileName();
    const QString actualName = name.isEmpty() ? oldFileName : name;
    if (actualName.isEmpty())
        return false;

    m_parent->m_resourceEditor->setFileName(actualName);
    if (!m_parent->m_resourceEditor->save()) {
        m_parent->m_resourceEditor->setFileName(oldFileName);
        return false;
    }

    m_parent->m_resourceEditor->setDirty(false);
    m_parent->setDisplayName(QFileInfo(actualName).fileName());

    emit changed();
    return true;
}

void ResourceEditorFile::rename(const QString &newName)
{
    m_parent->m_resourceEditor->setFileName(newName);
    emit changed();
}

QString ResourceEditorW::id() const {
    return QLatin1String(ResourceEditor::Constants::RESOURCEEDITOR_ID);
}

QString ResourceEditorFile::fileName() const
{
    return m_parent->m_resourceEditor->fileName();
}

bool ResourceEditorFile::isModified() const
{
    return m_parent->m_resourceEditor->isDirty();
}

bool ResourceEditorFile::isReadOnly() const
{
    const QString fileName = m_parent->m_resourceEditor->fileName();
    if (fileName.isEmpty())
        return false;
    const QFileInfo fi(fileName);
    return !fi.isWritable();
}

bool ResourceEditorFile::isSaveAsAllowed() const
{
    return true;
}

Core::IFile::ReloadBehavior ResourceEditorFile::reloadBehavior(ChangeTrigger state, ChangeType type) const
{
    if (type == TypePermissions)
        return BehaviorSilent;
    if (type == TypeContents) {
        if (state == TriggerInternal && !isModified())
            return BehaviorSilent;
        return BehaviorAsk;
    }
    return BehaviorAsk;
}

void ResourceEditorFile::reload(ReloadFlag flag, ChangeType type)
{
    if (flag == FlagIgnore)
        return;
    if (type == TypePermissions) {
        emit changed();
    } else {
        emit aboutToReload();
        if (m_parent->open(m_parent->m_resourceEditor->fileName()))
            emit reloaded();
    }
}

QString ResourceEditorFile::defaultPath() const
{
    return QString();
}

void ResourceEditorW::setSuggestedFileName(const QString &fileName)
{
    m_suggestedName = fileName;
}

QString ResourceEditorFile::suggestedFileName() const
{
    return m_parent->m_suggestedName;
}

void ResourceEditorW::dirtyChanged(bool dirty)
{
    if (debugResourceEditorW)
        qDebug() << " ResourceEditorW::dirtyChanged" <<  dirty;
    if (dirty)
        emit changed();
}

QWidget *ResourceEditorW::widget()
{
    return m_resourceEditor; /* we know it's a subclass of QWidget...*/
}

void ResourceEditorW::onUndoStackChanged(bool canUndo, bool canRedo)
{
    m_plugin->onUndoStackChanged(this, canUndo, canRedo);
}

void ResourceEditorW::onUndo()
{
    if (!m_resourceEditor.isNull())
        m_resourceEditor.data()->onUndo();
}

void ResourceEditorW::onRedo()
{
    if (!m_resourceEditor.isNull())
        m_resourceEditor.data()->onRedo();
}

} // namespace Internal
} // namespace ResourceEditor
