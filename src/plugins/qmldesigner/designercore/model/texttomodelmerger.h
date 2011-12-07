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

#ifndef TEXTTOMODELMERGER_H
#define TEXTTOMODELMERGER_H

#include "corelib_global.h"
#include "import.h"
#include "nodelistproperty.h"
#include "modelnode.h"

#include <qmljs/qmljsdocument.h>
#include <qmljs/qmljsscopechain.h>

#include <QtCore/QStringList>
#include <QtCore/QTimer>

namespace QmlDesigner {

class CORESHARED_EXPORT RewriterView;

namespace Internal {

class DifferenceHandler;
class ReadingContext;

class TextToModelMerger
{
    TextToModelMerger(const TextToModelMerger&);
    TextToModelMerger &operator=(const TextToModelMerger&);

public:
    TextToModelMerger(RewriterView *reWriterView);
    bool isActive() const;

    void setupImports(const QmlJS::Document::Ptr &doc, DifferenceHandler &differenceHandler);
    bool load(const QString &data, DifferenceHandler &differenceHandler);

    RewriterView *view() const
    { return m_rewriterView; }

    const QmlJS::ScopeChain *scopeChain() const
    { return m_scopeChain.data(); }

    const QmlJS::Document *document() const
    { return m_document.data(); }

protected:
    void setActive(bool active);

public:
    void syncNode(ModelNode &modelNode,
                  QmlJS::AST::UiObjectMember *astNode,
                  ReadingContext *context,
                  DifferenceHandler &differenceHandler);
    QString syncScriptBinding(ModelNode &modelNode,
                              const QString &prefix,
                              QmlJS::AST::UiScriptBinding *script,
                              ReadingContext *context,
                              DifferenceHandler &differenceHandler);
    void syncNodeId(ModelNode &modelNode, const QString &astObjectId,
                    DifferenceHandler &differenceHandler);
    void syncNodeProperty(AbstractProperty &modelProperty,
                          QmlJS::AST::UiObjectBinding *binding,
                          ReadingContext *context,
                          DifferenceHandler &differenceHandler);
    void syncExpressionProperty(AbstractProperty &modelProperty,
                                const QString &javascript,
                                const QString &astType,
                                DifferenceHandler &differenceHandler);
    void syncArrayProperty(AbstractProperty &modelProperty,
                           const QList<QmlJS::AST::UiObjectMember *> &arrayMembers,
                           ReadingContext *context,
                           DifferenceHandler &differenceHandler);
    void syncVariantProperty(AbstractProperty &modelProperty,
                             const QVariant &astValue,
                             const QString &astType,
                             DifferenceHandler &differenceHandler);
    void syncNodeListProperty(NodeListProperty &modelListProperty,
                              const QList<QmlJS::AST::UiObjectMember *> arrayMembers,
                              ReadingContext *context,
                              DifferenceHandler &differenceHandler);
    ModelNode createModelNode(const QString &typeName,
                              int majorVersion,
                              int minorVersion,
                              bool isImplicitComponent,
                              QmlJS::AST::UiObjectMember *astNode,
                              ReadingContext *context,
                              DifferenceHandler &differenceHandler);
    QStringList syncGroupedProperties(ModelNode &modelNode,
                                      const QString &name,
                                      QmlJS::AST::UiObjectMemberList *members,
                                      ReadingContext *context,
                                      DifferenceHandler &differenceHandler);

    void setupComponentDelayed(const ModelNode &node, bool synchron);
    void setupCustomParserNodeDelayed(const ModelNode &node, bool synchron);

    void delayedSetup();

private:
    void setupCustomParserNode(const ModelNode &node);
    void setupComponent(const ModelNode &node);

    static QString textAt(const QmlJS::Document::Ptr &doc,
                          const QmlJS::AST::SourceLocation &location);
    static QString textAt(const QmlJS::Document::Ptr &doc,
                          const QmlJS::AST::SourceLocation &from,
                          const QmlJS::AST::SourceLocation &to);

private:
    RewriterView *m_rewriterView;
    bool m_isActive;
    QSharedPointer<const QmlJS::ScopeChain> m_scopeChain;
    QmlJS::Document::Ptr m_document;
    QTimer m_setupTimer;
    QSet<ModelNode> m_setupComponentList;
    QSet<ModelNode> m_setupCustomParserList;
};

class DifferenceHandler
{
public:
    DifferenceHandler(TextToModelMerger *textToModelMerger):
            m_merger(textToModelMerger)
    {}
    virtual ~DifferenceHandler()
    {}

    virtual void modelMissesImport(const QmlDesigner::Import &import) = 0;
    virtual void importAbsentInQMl(const QmlDesigner::Import &import) = 0;
    virtual void bindingExpressionsDiffer(BindingProperty &modelProperty,
                                          const QString &javascript,
                                          const QString &astType) = 0;
    virtual void shouldBeBindingProperty(AbstractProperty &modelProperty,
                                         const QString &javascript,
                                         const QString &astType) = 0;
    virtual void shouldBeNodeListProperty(AbstractProperty &modelProperty,
                                          const QList<QmlJS::AST::UiObjectMember *> arrayMembers,
                                          ReadingContext *context) = 0;
    virtual void variantValuesDiffer(VariantProperty &modelProperty, const QVariant &qmlVariantValue, const QString &dynamicTypeName) = 0;
    virtual void shouldBeVariantProperty(AbstractProperty &modelProperty, const QVariant &qmlVariantValue, const QString &dynamicTypeName) = 0;
    virtual void shouldBeNodeProperty(AbstractProperty &modelProperty,
                                      const QString &typeName,
                                      int majorVersion,
                                      int minorVersion,
                                      QmlJS::AST::UiObjectMember *astNode,
                                      ReadingContext *context) = 0;
    virtual void modelNodeAbsentFromQml(ModelNode &modelNode) = 0;
    virtual ModelNode listPropertyMissingModelNode(NodeListProperty &modelProperty,
                                                   ReadingContext *context,
                                                   QmlJS::AST::UiObjectMember *arrayMember) = 0;
    virtual void typeDiffers(bool isRootNode,
                             ModelNode &modelNode,
                             const QString &typeName,
                             int majorVersion,
                             int minorVersion,
                             QmlJS::AST::UiObjectMember *astNode,
                             ReadingContext *context) = 0;
    virtual void propertyAbsentFromQml(AbstractProperty &modelProperty) = 0;
    virtual void idsDiffer(ModelNode &modelNode, const QString &qmlId) = 0;
    virtual bool isValidator() const = 0;

protected:
    TextToModelMerger *m_merger;
};

class ModelValidator: public DifferenceHandler
{
public:
    ModelValidator(TextToModelMerger *textToModelMerger):
            DifferenceHandler(textToModelMerger)
    {}
    ~ModelValidator()
    {}

    virtual void modelMissesImport(const QmlDesigner::Import &import);
    virtual void importAbsentInQMl(const QmlDesigner::Import &import);
    virtual void bindingExpressionsDiffer(BindingProperty &modelProperty,
                                          const QString &javascript,
                                          const QString &astType);
    virtual void shouldBeBindingProperty(AbstractProperty &modelProperty,
                                         const QString &javascript,
                                         const QString &astType);
    virtual void shouldBeNodeListProperty(AbstractProperty &modelProperty,
                                          const QList<QmlJS::AST::UiObjectMember *> arrayMembers,
                                          ReadingContext *context);
    virtual void variantValuesDiffer(VariantProperty &modelProperty, const QVariant &qmlVariantValue, const QString &dynamicTypeName);
    virtual void shouldBeVariantProperty(AbstractProperty &modelProperty, const QVariant &qmlVariantValue, const QString &dynamicTypeName);
    virtual void shouldBeNodeProperty(AbstractProperty &modelProperty,
                                      const QString &typeName,
                                      int majorVersion,
                                      int minorVersion,
                                      QmlJS::AST::UiObjectMember *astNode,
                                      ReadingContext *context);
    virtual void modelNodeAbsentFromQml(ModelNode &modelNode);
    virtual ModelNode listPropertyMissingModelNode(NodeListProperty &modelProperty,
                                                   ReadingContext *context,
                                                   QmlJS::AST::UiObjectMember *arrayMember);
    virtual void typeDiffers(bool isRootNode,
                             ModelNode &modelNode,
                             const QString &typeName,
                             int majorVersion,
                             int minorVersion,
                             QmlJS::AST::UiObjectMember *astNode,
                             ReadingContext *context);
    virtual void propertyAbsentFromQml(AbstractProperty &modelProperty);
    virtual void idsDiffer(ModelNode &modelNode, const QString &qmlId);
    virtual bool isValidator() const {return false; }
};

class ModelAmender: public DifferenceHandler
{
public:
    ModelAmender(TextToModelMerger *textToModelMerger):
            DifferenceHandler(textToModelMerger)
    {}
    ~ModelAmender()
    {}

    virtual void modelMissesImport(const QmlDesigner::Import &import);
    virtual void importAbsentInQMl(const QmlDesigner::Import &import);
    virtual void bindingExpressionsDiffer(BindingProperty &modelProperty,
                                          const QString &javascript,
                                          const QString &astType);
    virtual void shouldBeBindingProperty(AbstractProperty &modelProperty,
                                         const QString &javascript,
                                         const QString &astType);
    virtual void shouldBeNodeListProperty(AbstractProperty &modelProperty,
                                          const QList<QmlJS::AST::UiObjectMember *> arrayMembers,
                                          ReadingContext *context);
    virtual void variantValuesDiffer(VariantProperty &modelProperty, const QVariant &qmlVariantValue, const QString &dynamicType);
    virtual void shouldBeVariantProperty(AbstractProperty &modelProperty, const QVariant &qmlVariantValue, const QString &dynamicTypeName);
    virtual void shouldBeNodeProperty(AbstractProperty &modelProperty,
                                      const QString &typeName,
                                      int majorVersion,
                                      int minorVersion,
                                      QmlJS::AST::UiObjectMember *astNode,
                                      ReadingContext *context);
    virtual void modelNodeAbsentFromQml(ModelNode &modelNode);
    virtual ModelNode listPropertyMissingModelNode(NodeListProperty &modelProperty,
                                                   ReadingContext *context,
                                                   QmlJS::AST::UiObjectMember *arrayMember);
    virtual void typeDiffers(bool isRootNode,
                             ModelNode &modelNode,
                             const QString &typeName,
                             int majorVersion,
                             int minorVersion,
                             QmlJS::AST::UiObjectMember *astNode,
                             ReadingContext *context);
    virtual void propertyAbsentFromQml(AbstractProperty &modelProperty);
    virtual void idsDiffer(ModelNode &modelNode, const QString &qmlId);
    virtual bool isValidator() const {return true; }
};

} //Internal
} //QmlDesigner

#endif // TEXTTOMODELMERGER_H
