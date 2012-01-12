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

#include "designmode.h"

#include <coreplugin/icore.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/openeditorsmodel.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/icorelistener.h>
#include <coreplugin/editormanager/ieditor.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/qtcassert.h>

#include <QtCore/QPair>
#include <QtCore/QFileInfo>
#include <QtCore/QStringList>
#include <QtCore/QDebug>

#include <QtGui/QAction>
#include <QtGui/QPlainTextEdit>
#include <QtGui/QStackedWidget>

static Core::DesignMode *m_instance = 0;

namespace Core {

class EditorManager;

enum {
    debug = false
};

namespace Internal {

class DesignModeCoreListener : public Core::ICoreListener
{
public:
    DesignModeCoreListener(DesignMode* mode);
    bool coreAboutToClose();
private:
    DesignMode *m_mode;
};

DesignModeCoreListener::DesignModeCoreListener(DesignMode *mode) :
        m_mode(mode)
{
}

bool DesignModeCoreListener::coreAboutToClose()
{
    m_mode->currentEditorChanged(0);
    return true;
}

} // namespace Internal

struct DesignEditorInfo
{
    int widgetIndex;
    QStringList mimeTypes;
    Context context;
    QWidget *widget;
};

class DesignModePrivate
{
public:
    explicit DesignModePrivate(DesignMode *q);

public:
    Internal::DesignModeCoreListener *m_coreListener;
    QWeakPointer<Core::IEditor> m_currentEditor;
    bool m_isActive;
    bool m_isRequired;
    QList<DesignEditorInfo*> m_editors;
    QStackedWidget *m_stackWidget;
    Context m_activeContext;
};

DesignModePrivate::DesignModePrivate(DesignMode *q)
  : m_coreListener(new Internal::DesignModeCoreListener(q)),
    m_isActive(false),
    m_isRequired(false),
    m_stackWidget(new QStackedWidget)
{
}

DesignMode::DesignMode()
    : d(new DesignModePrivate(this))
{
    m_instance = this;
    setObjectName(QLatin1String("DesignMode"));
    setEnabled(false);
    setContext(Context(Constants::C_DESIGN_MODE));
    setWidget(d->m_stackWidget);
    setDisplayName(tr("Design"));
    setIcon(QIcon(QLatin1String(":/fancyactionbar/images/mode_Design.png")));
    setPriority(Constants::P_MODE_DESIGN);
    setId(QLatin1String(Constants::MODE_DESIGN));
    setType(QLatin1String(Constants::MODE_DESIGN_TYPE));

    ExtensionSystem::PluginManager::instance()->addObject(d->m_coreListener);

    connect(EditorManager::instance(), SIGNAL(currentEditorChanged(Core::IEditor*)),
            this, SLOT(currentEditorChanged(Core::IEditor*)));

    connect(ModeManager::instance(), SIGNAL(currentModeChanged(Core::IMode*,Core::IMode*)),
            this, SLOT(updateContext(Core::IMode*,Core::IMode*)));
}

DesignMode::~DesignMode()
{
    ExtensionSystem::PluginManager::instance()->removeObject(d->m_coreListener);
    delete d->m_coreListener;

    qDeleteAll(d->m_editors);
    delete d;
}

DesignMode *DesignMode::instance()
{
    return m_instance;
}

void DesignMode::setDesignModeIsRequired()
{
    d->m_isRequired = true;
}

bool DesignMode::designModeIsRequired() const
{
    return d->m_isRequired;
}

QStringList DesignMode::registeredMimeTypes() const
{
    QStringList rc;
    foreach(const DesignEditorInfo *i, d->m_editors)
        rc += i->mimeTypes;
    return rc;
}

/**
  * Registers a widget to be displayed when an editor with a file specified in
  * mimeTypes is opened. This also appends the additionalContext in ICore to
  * the context, specified here.
  */
void DesignMode::registerDesignWidget(QWidget *widget,
                                      const QStringList &mimeTypes,
                                      const Context &context)
{
    setDesignModeIsRequired();
    int index = d->m_stackWidget->addWidget(widget);

    DesignEditorInfo *info = new DesignEditorInfo;
    info->mimeTypes = mimeTypes;
    info->context = context;
    info->widgetIndex = index;
    info->widget = widget;
    d->m_editors.append(info);
}

void DesignMode::unregisterDesignWidget(QWidget *widget)
{
    d->m_stackWidget->removeWidget(widget);
    foreach(DesignEditorInfo *info, d->m_editors) {
        if (info->widget == widget) {
            d->m_editors.removeAll(info);
            break;
        }
    }
}

// if editor changes, check if we have valid mimetype registered.
void DesignMode::currentEditorChanged(Core::IEditor *editor)
{
    if (editor && (d->m_currentEditor.data() == editor))
        return;

    bool mimeEditorAvailable = false;

    if (editor && editor->file()) {
        const QString mimeType = editor->file()->mimeType();
        if (!mimeType.isEmpty()) {
            foreach (DesignEditorInfo *editorInfo, d->m_editors) {
                foreach (const QString &mime, editorInfo->mimeTypes) {
                    if (mime == mimeType) {
                        d->m_stackWidget->setCurrentIndex(editorInfo->widgetIndex);
                        setActiveContext(editorInfo->context);
                        mimeEditorAvailable = true;
                        setEnabled(true);
                        break;
                    }
                } // foreach mime
                if (mimeEditorAvailable)
                    break;
            } // foreach editorInfo
        }
    }
    if (d->m_currentEditor)
        disconnect(d->m_currentEditor.data(), SIGNAL(changed()), this, SLOT(updateActions()));

    if (!mimeEditorAvailable) {
        setActiveContext(Context());
        if (ModeManager::instance()->currentMode() == this)
            ModeManager::instance()->activateMode(QLatin1String(Core::Constants::MODE_EDIT));
        setEnabled(false);
        d->m_currentEditor = QWeakPointer<Core::IEditor>();
        emit actionsUpdated(d->m_currentEditor.data());
    } else {
        d->m_currentEditor = QWeakPointer<Core::IEditor>(editor);

        if (d->m_currentEditor)
            connect(d->m_currentEditor.data(), SIGNAL(changed()), this, SLOT(updateActions()));

        emit actionsUpdated(d->m_currentEditor.data());
    }
}

void DesignMode::updateActions()
{
    emit actionsUpdated(d->m_currentEditor.data());
}

void DesignMode::updateContext(Core::IMode *newMode, Core::IMode *oldMode)
{
    if (newMode == this) {
        // Apply active context
        Core::ICore::instance()->updateAdditionalContexts(Context(), d->m_activeContext);
    } else if (oldMode == this) {
        // Remove active context
        Core::ICore::instance()->updateAdditionalContexts(d->m_activeContext, Context());
    }
}

void DesignMode::setActiveContext(const Context &context)
{
    if (d->m_activeContext == context)
        return;

    if (ModeManager::instance()->currentMode() == this)
        Core::ICore::instance()->updateAdditionalContexts(d->m_activeContext, context);

    d->m_activeContext = context;
}

} // namespace Core
