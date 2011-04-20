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
#ifndef CPPCURRENTDOCUMENTFILTER_H
#define CPPCURRENTDOCUMENTFILTER_H

#include "searchsymbols.h"
#include <locator/ilocatorfilter.h>

namespace Core {
class EditorManager;
class IEditor;
}

namespace CppTools {
namespace Internal {

class CppModelManager;

class CppCurrentDocumentFilter : public  Locator::ILocatorFilter
{
    Q_OBJECT

public:
    CppCurrentDocumentFilter(CppModelManager *manager, Core::EditorManager *editorManager);
    ~CppCurrentDocumentFilter() {}

    QString displayName() const { return tr("Methods in Current Document"); }
    QString id() const { return QLatin1String("Methods in current Document"); }
    Priority priority() const { return Medium; }
    QList<Locator::FilterEntry> matchesFor(QFutureInterface<Locator::FilterEntry> &future, const QString &entry);
    void accept(Locator::FilterEntry selection) const;
    void refresh(QFutureInterface<void> &future);

private slots:
    void onDocumentUpdated(CPlusPlus::Document::Ptr doc);
    void onCurrentEditorChanged(Core::IEditor * currentEditor);
    void onEditorAboutToClose(Core::IEditor * currentEditor);

private:
    CppModelManager * m_modelManager;
    QString m_currentFileName;
    QList<ModelItemInfo> m_itemsOfCurrentDoc;
    SearchSymbols search;
};

} // namespace Internal
} // namespace CppTools

#endif // CPPCURRENTDOCUMENTFILTER_H
