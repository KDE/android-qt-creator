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

#include "qmljscompletionassist.h"
#include "qmljseditorconstants.h"
#include "qmljsreuse.h"
#include "qmlexpressionundercursor.h"

#include <coreplugin/ifile.h>

#include <texteditor/codeassist/iassistinterface.h>
#include <texteditor/codeassist/genericproposal.h>
#include <texteditor/codeassist/functionhintproposal.h>
#include <texteditor/codeassist/ifunctionhintproposalmodel.h>

#include <utils/qtcassert.h>

#include <qmljs/qmljsmodelmanagerinterface.h>
#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/qmljsinterpreter.h>
#include <qmljs/qmljscontext.h>
#include <qmljs/qmljsscopechain.h>
#include <qmljs/qmljsscanner.h>
#include <qmljs/qmljsbind.h>
#include <qmljs/qmljscompletioncontextfinder.h>
#include <qmljs/qmljsscopebuilder.h>

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QDebug>
#include <QtCore/QtAlgorithms>
#include <QtCore/QDirIterator>
#include <QtCore/QStringList>
#include <QtGui/QIcon>

using namespace QmlJS;
using namespace QmlJSEditor;
using namespace Internal;
using namespace TextEditor;

namespace {

enum CompletionOrder {
    EnumValueOrder = -5,
    SnippetOrder = -15,
    PropertyOrder = -10,
    SymbolOrder = -20,
    KeywordOrder = -25,
    TypeOrder = -30
};

static void addCompletion(QList<TextEditor::BasicProposalItem *> *completions,
                          const QString &text,
                          const QIcon &icon,
                          int order,
                          const QVariant &data = QVariant())
{
    if (text.isEmpty())
        return;

    BasicProposalItem *item = new QmlJSAssistProposalItem;
    item->setText(text);
    item->setIcon(icon);
    item->setOrder(order);
    item->setData(data);
    completions->append(item);
}

static void addCompletions(QList<TextEditor::BasicProposalItem *> *completions,
                           const QStringList &newCompletions,
                           const QIcon &icon,
                           int order)
{
    foreach (const QString &text, newCompletions)
        addCompletion(completions, text, icon, order);
}

class PropertyProcessor
{
public:
    virtual void operator()(const Value *base, const QString &name, const Value *value) = 0;
};

class CompletionAdder : public PropertyProcessor
{
protected:
    QList<TextEditor::BasicProposalItem *> *completions;

public:
    CompletionAdder(QList<TextEditor::BasicProposalItem *> *completions,
                    const QIcon &icon, int order)
        : completions(completions)
        , icon(icon)
        , order(order)
    {}

    virtual void operator()(const Value *base, const QString &name, const Value *value)
    {
        Q_UNUSED(base)
        Q_UNUSED(value)
        addCompletion(completions, name, icon, order);
    }

    QIcon icon;
    int order;
};

class LhsCompletionAdder : public CompletionAdder
{
public:
    LhsCompletionAdder(QList<TextEditor::BasicProposalItem *> *completions,
                       const QIcon &icon,
                       int order,
                       bool afterOn)
        : CompletionAdder(completions, icon, order)
        , afterOn(afterOn)
    {}

    virtual void operator ()(const Value *base, const QString &name, const Value *)
    {
        const QmlObjectValue *qmlBase = dynamic_cast<const QmlObjectValue *>(base);

        QString itemText = name;
        QString postfix;
        if (!itemText.isEmpty() && itemText.at(0).isLower())
            postfix = QLatin1String(": ");
        if (afterOn)
            postfix = QLatin1String(" {");

        // readonly pointer properties (anchors, ...) always get a .
        if (qmlBase && !qmlBase->isWritable(name) && qmlBase->isPointer(name))
            postfix = QLatin1Char('.');

        itemText.append(postfix);

        addCompletion(completions, itemText, icon, order);
    }

    bool afterOn;
};

class ProcessProperties: private MemberProcessor
{
    QSet<const ObjectValue *> _processed;
    bool _globalCompletion;
    bool _enumerateGeneratedSlots;
    bool _enumerateSlots;
    const ScopeChain *_scopeChain;
    const ObjectValue *_currentObject;
    PropertyProcessor *_propertyProcessor;

public:
    ProcessProperties(const ScopeChain *scopeChain)
        : _globalCompletion(false),
          _enumerateGeneratedSlots(false),
          _enumerateSlots(true),
          _scopeChain(scopeChain),
          _currentObject(0),
          _propertyProcessor(0)
    {
    }

    void setGlobalCompletion(bool globalCompletion)
    {
        _globalCompletion = globalCompletion;
    }

    void setEnumerateGeneratedSlots(bool enumerate)
    {
        _enumerateGeneratedSlots = enumerate;
    }

    void setEnumerateSlots(bool enumerate)
    {
        _enumerateSlots = enumerate;
    }

    void operator ()(const Value *value, PropertyProcessor *processor)
    {
        _processed.clear();
        _propertyProcessor = processor;

        processProperties(value);
    }

    void operator ()(PropertyProcessor *processor)
    {
        _processed.clear();
        _propertyProcessor = processor;

        foreach (const ObjectValue *scope, _scopeChain->all())
            processProperties(scope);
    }

private:
    void process(const QString &name, const Value *value)
    {
        (*_propertyProcessor)(_currentObject, name, value);
    }

    virtual bool processProperty(const QString &name, const Value *value)
    {
        process(name, value);
        return true;
    }

    virtual bool processEnumerator(const QString &name, const Value *value)
    {
        if (! _globalCompletion)
            process(name, value);
        return true;
    }

    virtual bool processSignal(const QString &name, const Value *value)
    {
        if (_globalCompletion)
            process(name, value);
        return true;
    }

    virtual bool processSlot(const QString &name, const Value *value)
    {
        if (_enumerateSlots)
            process(name, value);
        return true;
    }

    virtual bool processGeneratedSlot(const QString &name, const Value *value)
    {
        if (_enumerateGeneratedSlots || (_currentObject && _currentObject->className().endsWith(QLatin1String("Keys")))) {
            // ### FIXME: add support for attached properties.
            process(name, value);
        }
        return true;
    }

    void processProperties(const Value *value)
    {
        if (! value)
            return;
        else if (const ObjectValue *object = value->asObjectValue()) {
            processProperties(object);
        }
    }

    void processProperties(const ObjectValue *object)
    {
        if (! object || _processed.contains(object))
            return;

        _processed.insert(object);
        processProperties(object->prototype(_scopeChain->context()));

        _currentObject = object;
        object->processMembers(this);
        _currentObject = 0;
    }
};

const Value *getPropertyValue(const ObjectValue *object,
                                           const QStringList &propertyNames,
                                           const ContextPtr &context)
{
    if (propertyNames.isEmpty() || !object)
        return 0;

    const Value *value = object;
    foreach (const QString &name, propertyNames) {
        if (const ObjectValue *objectValue = value->asObjectValue()) {
            value = objectValue->lookupMember(name, context);
            if (!value)
                return 0;
        } else {
            return 0;
        }
    }
    return value;
}

bool isLiteral(AST::Node *ast)
{
    if (AST::cast<AST::StringLiteral *>(ast))
        return true;
    else if (AST::cast<AST::NumericLiteral *>(ast))
        return true;
    else
        return false;
}

} // Anonymous

// -----------------------
// QmlJSAssistProposalItem
// -----------------------
bool QmlJSAssistProposalItem::prematurelyApplies(const QChar &c) const
{
    if (data().canConvert<QString>()) // snippet
        return false;

    return (text().endsWith(QLatin1String(": ")) && c == QLatin1Char(':'))
            || (text().endsWith(QLatin1Char('.')) && c == QLatin1Char('.'));
}

void QmlJSAssistProposalItem::applyContextualContent(TextEditor::BaseTextEditor *editor,
                                                      int basePosition) const
{
    const int currentPosition = editor->position();
    editor->setCursorPosition(basePosition);
    editor->remove(currentPosition - basePosition);

    QString replaceable;
    const QString &content = text();
    if (content.endsWith(QLatin1String(": ")))
        replaceable = QLatin1String(": ");
    else if (content.endsWith(QLatin1Char('.')))
        replaceable = QLatin1String(".");
    int replacedLength = 0;
    for (int i = 0; i < replaceable.length(); ++i) {
        const QChar a = replaceable.at(i);
        const QChar b = editor->characterAt(editor->position() + i);
        if (a == b)
            ++replacedLength;
        else
            break;
    }
    const int length = editor->position() - basePosition + replacedLength;
    editor->replace(length, content);
}

// -------------------------
// FunctionHintProposalModel
// -------------------------
class FunctionHintProposalModel : public TextEditor::IFunctionHintProposalModel
{
public:
    FunctionHintProposalModel(const QString &functionName, const QStringList &signature)
        : m_functionName(functionName)
        , m_signature(signature)
        , m_minimumArgumentCount(signature.size())
    {}

    virtual void reset() {}
    virtual int size() const { return 1; }
    virtual QString text(int index) const;
    virtual int activeArgument(const QString &prefix) const;

private:
    QString m_functionName;
    QStringList m_signature;
    int m_minimumArgumentCount;
};

QString FunctionHintProposalModel::text(int index) const
{
    Q_UNUSED(index)

    QString prettyMethod;
    prettyMethod += QString::fromLatin1("function ");
    prettyMethod += m_functionName;
    prettyMethod += QLatin1Char('(');
    for (int i = 0; i < m_minimumArgumentCount; ++i) {
        if (i != 0)
            prettyMethod += QLatin1String(", ");

        QString arg = m_signature.at(i);
        if (arg.isEmpty()) {
            arg = QLatin1String("arg");
            arg += QString::number(i + 1);
        }

        prettyMethod += arg;
    }
    prettyMethod += QLatin1Char(')');
    return prettyMethod;
}

int FunctionHintProposalModel::activeArgument(const QString &prefix) const
{
    int argnr = 0;
    int parcount = 0;
    Scanner tokenize;
    const QList<Token> tokens = tokenize(prefix);
    for (int i = 0; i < tokens.count(); ++i) {
        const Token &tk = tokens.at(i);
        if (tk.is(Token::LeftParenthesis))
            ++parcount;
        else if (tk.is(Token::RightParenthesis))
            --parcount;
        else if (! parcount && tk.is(Token::Colon))
            ++argnr;
    }

    if (parcount < 0)
        return -1;

    return argnr;
}

// -----------------------------
// QmlJSCompletionAssistProvider
// -----------------------------
bool QmlJSCompletionAssistProvider::supportsEditor(const QString &editorId) const
{
    return editorId == QLatin1String(Constants::C_QMLJSEDITOR_ID);
}

int QmlJSCompletionAssistProvider::activationCharSequenceLength() const
{
    return 1;
}

bool QmlJSCompletionAssistProvider::isActivationCharSequence(const QString &sequence) const
{
    return isActivationChar(sequence.at(0));
}

bool QmlJSCompletionAssistProvider::isContinuationChar(const QChar &c) const
{
    return isIdentifierChar(c, false);
}

IAssistProcessor *QmlJSCompletionAssistProvider::createProcessor() const
{
    return new QmlJSCompletionAssistProcessor;
}

// ------------------------------
// QmlJSCompletionAssistProcessor
// ------------------------------
QmlJSCompletionAssistProcessor::QmlJSCompletionAssistProcessor()
    : m_startPosition(0)
    , m_snippetCollector(Constants::QML_SNIPPETS_GROUP_ID, iconForColor(Qt::red), SnippetOrder)
{}

QmlJSCompletionAssistProcessor::~QmlJSCompletionAssistProcessor()
{}

IAssistProposal *QmlJSCompletionAssistProcessor::createContentProposal() const
{
    IGenericProposalModel *model = new QmlJSAssistProposalModel(m_completions);
    IAssistProposal *proposal = new GenericProposal(m_startPosition, model);
    return proposal;
}

IAssistProposal *QmlJSCompletionAssistProcessor::createHintProposal(const QString &functionName,
                                                                    const QStringList &signature) const
{
    IFunctionHintProposalModel *model = new FunctionHintProposalModel(functionName, signature);
    IAssistProposal *proposal = new FunctionHintProposal(m_startPosition, model);
    return proposal;
}

IAssistProposal *QmlJSCompletionAssistProcessor::perform(const IAssistInterface *assistInterface)
{
    m_interface.reset(static_cast<const QmlJSCompletionAssistInterface *>(assistInterface));

    if (assistInterface->reason() == IdleEditor && !acceptsIdleEditor())
        return 0;

    const QString &fileName = m_interface->file()->fileName();

    m_startPosition = assistInterface->position();
    while (isIdentifierChar(m_interface->document()->characterAt(m_startPosition - 1), false, false))
        --m_startPosition;

    m_completions.clear();

    const QmlJSCompletionAssistInterface *qmlInterface =
            static_cast<const QmlJSCompletionAssistInterface *>(assistInterface);
    const SemanticInfo &semanticInfo = qmlInterface->semanticInfo();
    if (!semanticInfo.isValid())
        return 0;

    const Document::Ptr document = semanticInfo.document;
    const QFileInfo currentFileInfo(fileName);

    bool isQmlFile = false;
    if (currentFileInfo.suffix() == QLatin1String("qml"))
        isQmlFile = true;

    const QList<AST::Node *> path = semanticInfo.rangePath(m_interface->position());
    const ContextPtr &context = semanticInfo.context;
    const ScopeChain &scopeChain = semanticInfo.scopeChain(path);

    // Search for the operator that triggered the completion.
    QChar completionOperator;
    if (m_startPosition > 0)
        completionOperator = m_interface->document()->characterAt(m_startPosition - 1);

    QTextCursor startPositionCursor(qmlInterface->document());
    startPositionCursor.setPosition(m_startPosition);
    CompletionContextFinder contextFinder(startPositionCursor);

    const ObjectValue *qmlScopeType = 0;
    if (contextFinder.isInQmlContext()) {
        // find the enclosing qml object
        // ### this should use semanticInfo.declaringMember instead, but that may also return functions
        int i;
        for (i = path.size() - 1; i >= 0; --i) {
            AST::Node *node = path[i];
            if (AST::cast<AST::UiObjectDefinition *>(node) || AST::cast<AST::UiObjectBinding *>(node)) {
                qmlScopeType = document->bind()->findQmlObject(node);
                if (qmlScopeType)
                    break;
            }
        }
        // grouped property bindings change the scope type
        for (i++; i < path.size(); ++i) {
            AST::UiObjectDefinition *objDef = AST::cast<AST::UiObjectDefinition *>(path[i]);
            if (!objDef || !document->bind()->isGroupedPropertyBinding(objDef))
                break;
            const ObjectValue *newScopeType = qmlScopeType;
            for (AST::UiQualifiedId *it = objDef->qualifiedTypeNameId; it; it = it->next) {
                if (!newScopeType || it->name.isEmpty()) {
                    newScopeType = 0;
                    break;
                }
                const Value *v = newScopeType->lookupMember(it->name.toString(), context);
                v = context->lookupReference(v);
                newScopeType = value_cast<const ObjectValue *>(v);
            }
            if (!newScopeType)
                break;
            qmlScopeType = newScopeType;
        }
        // fallback to getting the base type object
        if (!qmlScopeType)
            qmlScopeType = context->lookupType(document.data(), contextFinder.qmlObjectTypeName());
    }

    if (contextFinder.isInStringLiteral()) {
        // get the text of the literal up to the cursor position
        //QTextCursor tc = textWidget->textCursor();
        QTextCursor tc(qmlInterface->document());
        tc.setPosition(qmlInterface->position());
        QmlExpressionUnderCursor expressionUnderCursor;
        expressionUnderCursor(tc);
        QString literalText = expressionUnderCursor.text();
        QTC_ASSERT(!literalText.isEmpty() && (
                       literalText.at(0) == QLatin1Char('"')
                       || literalText.at(0) == QLatin1Char('\'')), return 0);
        literalText = literalText.mid(1);

        if (contextFinder.isInImport()) {
            QStringList patterns;
            patterns << QLatin1String("*.qml") << QLatin1String("*.js");
            if (completeFileName(document->path(), literalText, patterns))
                return createContentProposal();
            return 0;
        }

        const Value *value =
                getPropertyValue(qmlScopeType, contextFinder.bindingPropertyName(), context);
        if (!value) {
            // do nothing
        } else if (value->asUrlValue()) {
            if (completeUrl(document->path(), literalText))
                return createContentProposal();
        }

        // ### enum completion?

        // completion gets triggered for / in string literals, if we don't
        // return here, this will mean the snippet completion pops up for
        // each / in a string literal that is not triggering file completion
        return 0;
    } else if (completionOperator.isSpace()
               || completionOperator.isNull()
               || isDelimiterChar(completionOperator)
               || (completionOperator == QLatin1Char('(')
                   && m_startPosition != m_interface->position())) {

        bool doGlobalCompletion = true;
        bool doQmlKeywordCompletion = true;
        bool doJsKeywordCompletion = true;
        bool doQmlTypeCompletion = false;

        if (contextFinder.isInLhsOfBinding() && qmlScopeType) {
            doGlobalCompletion = false;
            doJsKeywordCompletion = false;
            doQmlTypeCompletion = true;

            ProcessProperties processProperties(&scopeChain);
            processProperties.setGlobalCompletion(true);
            processProperties.setEnumerateGeneratedSlots(true);
            processProperties.setEnumerateSlots(false);

            // id: is special
            BasicProposalItem *idProposalItem = new QmlJSAssistProposalItem;
            idProposalItem->setText(QLatin1String("id: "));
            idProposalItem->setIcon(m_interface->symbolIcon());
            idProposalItem->setOrder(PropertyOrder);
            m_completions.append(idProposalItem);

            {
                LhsCompletionAdder completionAdder(&m_completions, m_interface->symbolIcon(),
                                                   PropertyOrder, contextFinder.isAfterOnInLhsOfBinding());
                processProperties(qmlScopeType, &completionAdder);
            }

            if (ScopeBuilder::isPropertyChangesObject(context, qmlScopeType)
                    && scopeChain.qmlScopeObjects().size() == 2) {
                CompletionAdder completionAdder(&m_completions, m_interface->symbolIcon(), SymbolOrder);
                processProperties(scopeChain.qmlScopeObjects().first(), &completionAdder);
            }
        }

        if (contextFinder.isInRhsOfBinding() && qmlScopeType) {
            doQmlKeywordCompletion = false;

            // complete enum values for enum properties
            const Value *value =
                    getPropertyValue(qmlScopeType, contextFinder.bindingPropertyName(), context);
            if (const QmlEnumValue *enumValue =
                    dynamic_cast<const QmlEnumValue *>(value)) {
                const QString &name = context->imports(document.data())->nameForImportedObject(enumValue->owner(), context.data());
                foreach (const QString &key, enumValue->keys()) {
                    QString completion;
                    if (name.isEmpty())
                        completion = QString("\"%1\"").arg(key);
                    else
                        completion = QString("%1.%2").arg(name, key);
                    addCompletion(&m_completions, key, m_interface->symbolIcon(),
                                  EnumValueOrder, completion);
                }
            }
        }

        if (!contextFinder.isInImport() && !contextFinder.isInQmlContext())
            doQmlTypeCompletion = true;

        if (doQmlTypeCompletion) {
            if (const ObjectValue *qmlTypes = scopeChain.qmlTypes()) {
                ProcessProperties processProperties(&scopeChain);
                CompletionAdder completionAdder(&m_completions, m_interface->symbolIcon(), TypeOrder);
                processProperties(qmlTypes, &completionAdder);
            }
        }

        if (doGlobalCompletion) {
            // It's a global completion.
            ProcessProperties processProperties(&scopeChain);
            processProperties.setGlobalCompletion(true);
            CompletionAdder completionAdder(&m_completions, m_interface->symbolIcon(), SymbolOrder);
            processProperties(&completionAdder);
        }

        if (doJsKeywordCompletion) {
            // add js keywords
            addCompletions(&m_completions, Scanner::keywords(), m_interface->keywordIcon(), KeywordOrder);
        }

        // add qml extra words
        if (doQmlKeywordCompletion && isQmlFile) {
            static QStringList qmlWords;
            static QStringList qmlWordsAlsoInJs;

            if (qmlWords.isEmpty()) {
                qmlWords << QLatin1String("property")
                            //<< QLatin1String("readonly")
                         << QLatin1String("signal")
                         << QLatin1String("import");
            }
            if (qmlWordsAlsoInJs.isEmpty())
                qmlWordsAlsoInJs << QLatin1String("default") << QLatin1String("function");

            addCompletions(&m_completions, qmlWords, m_interface->keywordIcon(), KeywordOrder);
            if (!doJsKeywordCompletion)
                addCompletions(&m_completions, qmlWordsAlsoInJs, m_interface->keywordIcon(), KeywordOrder);
        }
    }

    else if (completionOperator == QLatin1Char('.') || completionOperator == QLatin1Char('(')) {
        // Look at the expression under cursor.
        //QTextCursor tc = textWidget->textCursor();
        QTextCursor tc(qmlInterface->document());
        tc.setPosition(m_startPosition - 1);

        QmlExpressionUnderCursor expressionUnderCursor;
        QmlJS::AST::ExpressionNode *expression = expressionUnderCursor(tc);

        if (expression != 0 && ! isLiteral(expression)) {
            // Evaluate the expression under cursor.
            ValueOwner *interp = context->valueOwner();
            const Value *value =
                    interp->convertToObject(scopeChain.evaluate(expression));
            //qDebug() << "type:" << interp->typeId(value);

            if (value && completionOperator == QLatin1Char('.')) { // member completion
                ProcessProperties processProperties(&scopeChain);
                if (contextFinder.isInLhsOfBinding() && qmlScopeType) {
                    LhsCompletionAdder completionAdder(&m_completions, m_interface->symbolIcon(),
                                                       PropertyOrder, contextFinder.isAfterOnInLhsOfBinding());
                    processProperties.setEnumerateGeneratedSlots(true);
                    processProperties(value, &completionAdder);
                } else {
                    CompletionAdder completionAdder(&m_completions, m_interface->symbolIcon(), SymbolOrder);
                    processProperties(value, &completionAdder);
                }
            } else if (value
                       && completionOperator == QLatin1Char('(')
                       && m_startPosition == m_interface->position()) {
                // function completion
                if (const FunctionValue *f = value->asFunctionValue()) {
                    QString functionName = expressionUnderCursor.text();
                    int indexOfDot = functionName.lastIndexOf(QLatin1Char('.'));
                    if (indexOfDot != -1)
                        functionName = functionName.mid(indexOfDot + 1);

                    QStringList signature;
                    for (int i = 0; i < f->argumentCount(); ++i)
                        signature.append(f->argumentName(i));

                    return createHintProposal(functionName.trimmed(), signature);
                }
            }
        }

        if (! m_completions.isEmpty())
            return createContentProposal();
        return 0;
    }

    if (isQmlFile
            && (completionOperator.isNull()
                || completionOperator.isSpace()
                || isDelimiterChar(completionOperator))) {
        m_completions.append(m_snippetCollector.collect());
    }

    if (! m_completions.isEmpty())
        return createContentProposal();
    return 0;
}

bool QmlJSCompletionAssistProcessor::acceptsIdleEditor() const
{
    const int cursorPos = m_interface->position();

    bool maybeAccept = false;
    const QChar &charBeforeCursor = m_interface->document()->characterAt(cursorPos - 1);
    if (isActivationChar(charBeforeCursor)) {
        maybeAccept = true;
    } else {
        const QChar &charUnderCursor = m_interface->document()->characterAt(cursorPos);
        if (isIdentifierChar(charBeforeCursor)
                && ((charUnderCursor.isSpace()
                    || charUnderCursor.isNull()
                    || isDelimiterChar(charUnderCursor))
                || isIdentifierChar(charUnderCursor))) {

            int startPos = cursorPos - 1;
            for (; startPos != -1; --startPos) {
                if (!isIdentifierChar(m_interface->document()->characterAt(startPos)))
                    break;
            }
            ++startPos;

            const QString &word = m_interface->textAt(startPos, cursorPos - startPos);
            if (word.length() > 2 && isIdentifierChar(word.at(0), true)) {
                for (int i = 1; i < word.length(); ++i) {
                    if (!isIdentifierChar(word.at(i)))
                        return false;
                }
                maybeAccept = true;
            }
        }
    }

    if (maybeAccept) {
        QTextCursor tc(m_interface->document());
        tc.setPosition(m_interface->position());
        const QTextBlock &block = tc.block();
        const QString &blockText = block.text();
        const int blockState = qMax(0, block.previous().userState()) & 0xff;

        Scanner scanner;
        const QList<Token> tokens = scanner(blockText, blockState);
        const int column = block.position() - m_interface->position();
        foreach (const Token &tk, tokens) {
            if (column >= tk.begin() && column <= tk.end()) {
                if (charBeforeCursor == QLatin1Char('/') && tk.is(Token::String))
                    return true; // path completion inside string literals
                if (tk.is(Token::Comment) || tk.is(Token::String) || tk.is(Token::RegExp))
                    return false;
                break;
            }
        }
        if (charBeforeCursor != QLatin1Char('/'))
            return true;
    }

    return false;
}

bool QmlJSCompletionAssistProcessor::completeFileName(const QString &relativeBasePath,
                                                      const QString &fileName,
                                                      const QStringList &patterns)
{
    const QFileInfo fileInfo(fileName);
    QString directoryPrefix;
    if (fileInfo.isRelative()) {
        directoryPrefix = relativeBasePath;
        directoryPrefix += QDir::separator();
        directoryPrefix += fileInfo.path();
    } else {
        directoryPrefix = fileInfo.path();
    }
    if (!QFileInfo(directoryPrefix).exists())
        return false;

    QDirIterator dirIterator(directoryPrefix,
                             patterns,
                             QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);
    while (dirIterator.hasNext()) {
        dirIterator.next();
        const QString fileName = dirIterator.fileName();

        BasicProposalItem *item = new QmlJSAssistProposalItem;
        item->setText(fileName);
        item->setIcon(m_interface->fileNameIcon());
        m_completions.append(item);
    }

    return !m_completions.isEmpty();
}

bool QmlJSCompletionAssistProcessor::completeUrl(const QString &relativeBasePath, const QString &urlString)
{
    const QUrl url(urlString);
    QString fileName = url.toLocalFile();
    if (fileName.isEmpty())
        return false;

    return completeFileName(relativeBasePath, fileName);
}

// ------------------------------
// QmlJSCompletionAssistInterface
// ------------------------------
QmlJSCompletionAssistInterface::QmlJSCompletionAssistInterface(QTextDocument *document,
                                                               int position,
                                                               Core::IFile *file,
                                                               TextEditor::AssistReason reason,
                                                               const SemanticInfo &info)
    : DefaultAssistInterface(document, position, file, reason)
    , m_semanticInfo(info)
    , m_darkBlueIcon(iconForColor(Qt::darkBlue))
    , m_darkYellowIcon(iconForColor(Qt::darkYellow))
    , m_darkCyanIcon(iconForColor(Qt::darkCyan))
{}

const SemanticInfo &QmlJSCompletionAssistInterface::semanticInfo() const
{
    return m_semanticInfo;
}

namespace {

struct QmlJSLessThan
{
    bool operator() (const BasicProposalItem *a, const BasicProposalItem *b)
    {
        if (a->order() != b->order())
            return a->order() > b->order();
        else if (a->text().isEmpty())
            return true;
        else if (b->text().isEmpty())
            return false;
        else if (a->data().isValid() != b->data().isValid())
            return a->data().isValid();
        else if (a->text().at(0).isUpper() && b->text().at(0).isLower())
            return false;
        else if (a->text().at(0).isLower() && b->text().at(0).isUpper())
            return true;
        return a->text() < b->text();
    }
};

} // Anonymous

// -------------------------
// QmlJSAssistProposalModel
// -------------------------
void QmlJSAssistProposalModel::sort()
{
    qSort(currentItems().first, currentItems().second, QmlJSLessThan());
}

bool QmlJSAssistProposalModel::keepPerfectMatch(TextEditor::AssistReason reason) const
{
    return reason == ExplicitlyInvoked;
}
