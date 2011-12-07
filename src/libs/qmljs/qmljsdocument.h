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
#ifndef QMLDOCUMENT_H
#define QMLDOCUMENT_H

#include <QtCore/QList>
#include <QtCore/QPair>
#include <QtCore/QSharedPointer>
#include <QtCore/QString>

#include <languageutils/fakemetaobject.h>

#include "parser/qmldirparser_p.h"
#include "parser/qmljsengine_p.h"
#include "qmljs_global.h"

namespace QmlJS {

class Bind;
class Snapshot;

class QMLJS_EXPORT Document
{
public:
    typedef QSharedPointer<const Document> Ptr;
    typedef QSharedPointer<Document> MutablePtr;

    // used in a 3-bit bitfield
    enum Language
    {
        QmlLanguage = 0,
        JavaScriptLanguage = 1,
        JsonLanguage = 2,
        UnknownLanguage = 3
    };

protected:
    Document(const QString &fileName, Language language);

public:
    ~Document();

    static MutablePtr create(const QString &fileName, Language language);
    static Language guessLanguageFromSuffix(const QString &fileName);

    Document::Ptr ptr() const;

    bool isQmlDocument() const;
    Language language() const;

    AST::UiProgram *qmlProgram() const;
    AST::Program *jsProgram() const;
    AST::ExpressionNode *expression() const;
    AST::Node *ast() const;

    const QmlJS::Engine *engine() const;

    QList<DiagnosticMessage> diagnosticMessages() const;

    QString source() const;
    void setSource(const QString &source);

    bool parse();
    bool parseQml();
    bool parseJavaScript();
    bool parseExpression();

    bool isParsedCorrectly() const
    { return _parsedCorrectly; }

    Bind *bind() const;

    int editorRevision() const;
    void setEditorRevision(int revision);

    QString fileName() const;
    QString path() const;
    QString componentName() const;

private:
    bool parse_helper(int kind);

private:
    QmlJS::Engine *_engine;
    AST::Node *_ast;
    Bind *_bind;
    QList<QmlJS::DiagnosticMessage> _diagnosticMessages;
    QString _fileName;
    QString _path;
    QString _componentName;
    QString _source;
    QWeakPointer<Document> _ptr;
    int _editorRevision;
    Language _language : 3;
    bool _parsedCorrectly : 1;

    // for documentFromSource
    friend class Snapshot;
};

class QMLJS_EXPORT ModuleApiInfo
{
public:
    QString uri;
    LanguageUtils::ComponentVersion version;
    QString cppName;
};

class QMLJS_EXPORT LibraryInfo
{
public:
    enum PluginTypeInfoStatus {
        NoTypeInfo,
        DumpDone,
        DumpError,
        TypeInfoFileDone,
        TypeInfoFileError
    };

    enum Status {
        NotScanned,
        NotFound,
        Found
    };

private:
    Status _status;
    QList<QmlDirParser::Component> _components;
    QList<QmlDirParser::Plugin> _plugins;
    QList<QmlDirParser::TypeInfo> _typeinfos;
    typedef QList<LanguageUtils::FakeMetaObject::ConstPtr> FakeMetaObjectList;
    FakeMetaObjectList _metaObjects;
    QList<ModuleApiInfo> _moduleApis;

    PluginTypeInfoStatus _dumpStatus;
    QString _dumpError;

public:
    explicit LibraryInfo(Status status = NotScanned);
    explicit LibraryInfo(const QmlDirParser &parser);
    ~LibraryInfo();

    QList<QmlDirParser::Component> components() const
    { return _components; }

    QList<QmlDirParser::Plugin> plugins() const
    { return _plugins; }

    QList<QmlDirParser::TypeInfo> typeInfos() const
    { return _typeinfos; }

    FakeMetaObjectList metaObjects() const
    { return _metaObjects; }

    void setMetaObjects(const FakeMetaObjectList &objects)
    { _metaObjects = objects; }

    QList<ModuleApiInfo> moduleApis() const
    { return _moduleApis; }

    void setModuleApis(const QList<ModuleApiInfo> &apis)
    { _moduleApis = apis; }

    bool isValid() const
    { return _status == Found; }

    bool wasScanned() const
    { return _status != NotScanned; }

    PluginTypeInfoStatus pluginTypeInfoStatus() const
    { return _dumpStatus; }

    QString pluginTypeInfoError() const
    { return _dumpError; }

    void setPluginTypeInfoStatus(PluginTypeInfoStatus dumped, const QString &error = QString())
    { _dumpStatus = dumped; _dumpError = error; }
};

class QMLJS_EXPORT Snapshot
{
    typedef QHash<QString, Document::Ptr> _Base;
    QHash<QString, Document::Ptr> _documents;
    QHash<QString, QList<Document::Ptr> > _documentsByPath;
    QHash<QString, LibraryInfo> _libraries;

public:
    Snapshot();
    ~Snapshot();

    typedef _Base::iterator iterator;
    typedef _Base::const_iterator const_iterator;

    const_iterator begin() const { return _documents.begin(); }
    const_iterator end() const { return _documents.end(); }

    void insert(const Document::Ptr &document, bool allowInvalid = false);
    void insertLibraryInfo(const QString &path, const LibraryInfo &info);
    void remove(const QString &fileName);

    Document::Ptr document(const QString &fileName) const;
    QList<Document::Ptr> documentsInDirectory(const QString &path) const;
    LibraryInfo libraryInfo(const QString &path) const;

    Document::MutablePtr documentFromSource(const QString &code,
                                     const QString &fileName,
                                     Document::Language language) const;
};

} // namespace QmlJS

#endif // QMLDOCUMENT_H
