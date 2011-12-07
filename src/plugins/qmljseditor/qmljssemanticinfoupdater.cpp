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

#include "qmljssemanticinfoupdater.h"

#include <qmljs/qmljsmodelmanagerinterface.h>
#include <qmljs/qmljsdocument.h>
#include <qmljs/qmljscheck.h>
#include <qmljs/qmljscontext.h>
#include <qmljs/qmljslink.h>
#include <qmljstools/qmljsmodelmanager.h>

namespace QmlJSEditor {
namespace Internal {

SemanticInfoUpdater::SemanticInfoUpdater(QObject *parent)
        : QThread(parent)
        , m_wasCancelled(false)
{
}

SemanticInfoUpdater::~SemanticInfoUpdater()
{
}

void SemanticInfoUpdater::abort()
{
    QMutexLocker locker(&m_mutex);
    m_wasCancelled = true;
    m_condition.wakeOne();
}

void SemanticInfoUpdater::update(const QmlJS::Document::Ptr &doc, const QmlJS::Snapshot &snapshot)
{
    QMutexLocker locker(&m_mutex);
    m_sourceDocument = doc;
    m_sourceSnapshot = snapshot;
    m_condition.wakeOne();
}

void SemanticInfoUpdater::reupdate(const QmlJS::Snapshot &snapshot)
{
    QMutexLocker locker(&m_mutex);
    m_sourceDocument = m_lastSemanticInfo.document;
    m_sourceSnapshot = snapshot;
    m_condition.wakeOne();
}

void SemanticInfoUpdater::run()
{
    setPriority(QThread::LowestPriority);

    forever {
        m_mutex.lock();

        while (! (m_wasCancelled || m_sourceDocument))
            m_condition.wait(&m_mutex);

        const bool done = m_wasCancelled;
        QmlJS::Document::Ptr doc = m_sourceDocument;
        QmlJS::Snapshot snapshot = m_sourceSnapshot;
        m_sourceDocument.clear();
        m_sourceSnapshot = QmlJS::Snapshot();

        m_mutex.unlock();

        if (done)
            break;

        const SemanticInfo info = makeNewSemanticInfo(doc, snapshot);

        m_mutex.lock();
        const bool cancelledOrNewData = m_wasCancelled || m_sourceDocument;
        m_mutex.unlock();

        if (! cancelledOrNewData) {
            m_lastSemanticInfo = info;
            emit updated(info);
        }
    }
}

SemanticInfo SemanticInfoUpdater::makeNewSemanticInfo(const QmlJS::Document::Ptr &doc, const QmlJS::Snapshot &snapshot)
{
    using namespace QmlJS;

    SemanticInfo semanticInfo;
    semanticInfo.document = doc;
    semanticInfo.snapshot = snapshot;

    ModelManagerInterface *modelManager = ModelManagerInterface::instance();

    Link link(semanticInfo.snapshot, modelManager->importPaths(), modelManager->builtins(doc));
    semanticInfo.context = link(doc, &semanticInfo.semanticMessages);

    ScopeChain *scopeChain = new ScopeChain(doc, semanticInfo.context);
    semanticInfo.m_rootScopeChain = QSharedPointer<const ScopeChain>(scopeChain);

    if (doc->language() != Document::JsonLanguage) {
        Check checker(doc, semanticInfo.context);
        semanticInfo.staticAnalysisMessages = checker();
    }

    return semanticInfo;
}

} // namespace Internal
} // namespace QmlJSEditor
