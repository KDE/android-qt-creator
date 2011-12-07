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

#ifndef QMLJSSEMANTICHIGHLIGHTER_H
#define QMLJSSEMANTICHIGHLIGHTER_H

#include <texteditor/semantichighlighter.h>
#include <QtCore/QFutureWatcher>

namespace QmlJS {
class ScopeChain;
namespace AST {
class SourceLocation;
}
}

namespace TextEditor {
class FontSettings;
}

namespace QmlJSEditor {

class QmlJSTextEditorWidget;

namespace Internal {

class SemanticHighlighter : public QObject
{
    Q_OBJECT
public:
    enum UseType
    {
        UnknownType,
        LocalIdType, // ids in the same file
        ExternalIdType, // ids from instantiating files
        QmlTypeType, // qml types
        RootObjectPropertyType, // property in root object
        ScopeObjectPropertyType, // property in scope object
        ExternalObjectPropertyType, // property in root object of instantiating file
        JsScopeType, // var or function in local js scope
        JsImportType, // name of js import
        JsGlobalType, // in global scope
        LocalStateNameType, // name of a state in the current file
        BindingNameType, // name on the left hand side of a binding
        FieldType // member of an object
    };

    typedef TextEditor::SemanticHighlighter::Result Use;

    SemanticHighlighter(QmlJSTextEditorWidget *editor);

    void rerun(const QmlJS::ScopeChain &scopeChain);
    void cancel();

    int startRevision() const;

    void updateFontSettings(const TextEditor::FontSettings &fontSettings);

private slots:
    void applyResults(int from, int to);
    void finished();

private:
    QFutureWatcher<Use>  m_watcher;
    QmlJSTextEditorWidget *m_editor;
    int m_startRevision;
    QHash<int, QTextCharFormat> m_formats;
};

} // namespace Internal
} // namespace QmlJSEditor

#endif // QMLJSSEMANTICHIGHLIGHTER_H
