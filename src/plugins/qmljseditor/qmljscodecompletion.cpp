/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#include "qmljscodecompletion.h"
#include "qmlexpressionundercursor.h"
#include "qmljseditor.h"
#include "qmljseditorconstants.h"

#include <qmljs/qmljsmodelmanagerinterface.h>
#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/qmljsinterpreter.h>
#include <qmljs/qmljslookupcontext.h>
#include <qmljs/qmljsscanner.h>
#include <qmljs/qmljsbind.h>
#include <qmljs/qmljscompletioncontextfinder.h>
#include <qmljs/qmljsscopebuilder.h>

#include <texteditor/basetexteditor.h>
#include <texteditor/completionsettings.h>

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>

#include <utils/faketooltip.h>
#include <utils/qtcassert.h>

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QDebug>
#include <QtCore/QDirIterator>

#include <QtGui/QApplication>
#include <QtGui/QDesktopWidget>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QPainter>
#include <QtGui/QStyle>
#include <QtGui/QTextBlock>
#include <QtGui/QToolButton>

using namespace QmlJSEditor;
using namespace QmlJSEditor::Internal;
using namespace QmlJS;

namespace {

enum CompletionOrder {
    EnumValueOrder = -5,
    SnippetOrder = -15,
    PropertyOrder = -10,
    SymbolOrder = -20,
    KeywordOrder = -25,
    TypeOrder = -30
};

// Temporary workaround until we have proper icons for QML completion items
static QIcon iconForColor(const QColor &color)
{
    QPixmap pix(6, 6);

    int pixSize = 20;
    QBrush br(color);

    QPixmap pm(2 * pixSize, 2 * pixSize);
    QPainter pmp(&pm);
    pmp.fillRect(0, 0, pixSize, pixSize, Qt::lightGray);
    pmp.fillRect(pixSize, pixSize, pixSize, pixSize, Qt::lightGray);
    pmp.fillRect(0, pixSize, pixSize, pixSize, Qt::darkGray);
    pmp.fillRect(pixSize, 0, pixSize, pixSize, Qt::darkGray);
    pmp.fillRect(0, 0, 2 * pixSize, 2 * pixSize, color);
    br = QBrush(pm);

    QPainter p(&pix);
    int corr = 1;
    QRect r = pix.rect().adjusted(corr, corr, -corr, -corr);
    p.setBrushOrigin((r.width() % pixSize + pixSize) / 2 + corr, (r.height() % pixSize + pixSize) / 2 + corr);
    p.fillRect(r, br);

    p.fillRect(r.width() / 4 + corr, r.height() / 4 + corr,
               r.width() / 2, r.height() / 2,
               QColor(color.rgb()));
    p.drawRect(pix.rect().adjusted(0, 0, -1, -1));

    return pix;
}

static bool checkStartOfIdentifier(const QString &word)
{
    if (word.isEmpty())
        return false;

    const QChar ch = word.at(0);

    switch (ch.unicode()) {
    case '_': case '$':
        return true;

    default:
        return ch.isLetter();
    }
}

static bool isIdentifierChar(QChar ch)
{
    switch (ch.unicode()) {
    case '_': case '$':
        return true;

    default:
        return ch.isLetterOrNumber();
    }
}

class SearchPropertyDefinitions: protected AST::Visitor
{
    QList<AST::UiPublicMember *> _properties;

public:
    QList<AST::UiPublicMember *> operator()(Document::Ptr doc)
    {
        _properties.clear();
        if (doc && doc->qmlProgram())
            doc->qmlProgram()->accept(this);
        return _properties;
    }


protected:
    using AST::Visitor::visit;

    virtual bool visit(AST::UiPublicMember *member)
    {
        if (member->propertyToken.isValid()) {
            _properties.append(member);
        }

        return true;
    }
};

class EnumerateProperties: private Interpreter::MemberProcessor
{
    QSet<const Interpreter::ObjectValue *> _processed;
    QHash<QString, const Interpreter::Value *> _properties;
    bool _globalCompletion;
    bool _enumerateGeneratedSlots;
    const Interpreter::Context *_context;
    const Interpreter::ObjectValue *_currentObject;

public:
    EnumerateProperties(const Interpreter::Context *context)
        : _globalCompletion(false),
          _enumerateGeneratedSlots(false),
          _context(context),
          _currentObject(0)
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

    QHash<QString, const Interpreter::Value *> operator ()(const Interpreter::Value *value)
    {
        _processed.clear();
        _properties.clear();
        _currentObject = Interpreter::value_cast<const Interpreter::ObjectValue *>(value);

        enumerateProperties(value);

        return _properties;
    }

    QHash<QString, const Interpreter::Value *> operator ()()
    {
        _processed.clear();
        _properties.clear();
        _currentObject = 0;

        foreach (const Interpreter::ObjectValue *scope, _context->scopeChain().all())
            enumerateProperties(scope);

        return _properties;
    }

private:
    void insertProperty(const QString &name, const Interpreter::Value *value)
    {
        _properties.insert(name, value);
    }

    virtual bool processProperty(const QString &name, const Interpreter::Value *value)
    {
        insertProperty(name, value);
        return true;
    }

    virtual bool processEnumerator(const QString &name, const Interpreter::Value *value)
    {
        if (! _globalCompletion)
            insertProperty(name, value);
        return true;
    }

    virtual bool processSignal(const QString &, const Interpreter::Value *)
    {
        return true;
    }

    virtual bool processSlot(const QString &name, const Interpreter::Value *value)
    {
        insertProperty(name, value);
        return true;
    }

    virtual bool processGeneratedSlot(const QString &name, const Interpreter::Value *value)
    {
        if (_enumerateGeneratedSlots || (_currentObject && _currentObject->className().endsWith(QLatin1String("Keys")))) {
            // ### FIXME: add support for attached properties.
            insertProperty(name, value);
        }
        return true;
    }

    void enumerateProperties(const Interpreter::Value *value)
    {
        if (! value)
            return;
        else if (const Interpreter::ObjectValue *object = value->asObjectValue()) {
            enumerateProperties(object);
        }
    }

    void enumerateProperties(const Interpreter::ObjectValue *object)
    {
        if (! object || _processed.contains(object))
            return;

        _processed.insert(object);
        enumerateProperties(object->prototype(_context));

        object->processMembers(this);
    }
};

} // end of anonymous namespace

namespace QmlJSEditor {
namespace Internal {

class FunctionArgumentWidget : public QLabel
{
public:
    FunctionArgumentWidget();
    void showFunctionHint(const QString &functionName,
                          const QStringList &signature,
                          int startPosition);

protected:
    bool eventFilter(QObject *obj, QEvent *e);

private:
    void updateArgumentHighlight();
    void updateHintText();

    QString m_functionName;
    QStringList m_signature;
    int m_minimumArgumentCount;
    int m_startpos;
    int m_currentarg;
    int m_current;
    bool m_escapePressed;

    TextEditor::ITextEditor *m_editor;

    QWidget *m_pager;
    QLabel *m_numberLabel;
    Utils::FakeToolTip *m_popupFrame;
};


FunctionArgumentWidget::FunctionArgumentWidget():
    m_minimumArgumentCount(0),
    m_startpos(-1),
    m_current(0),
    m_escapePressed(false)
{
    QObject *editorObject = Core::EditorManager::instance()->currentEditor();
    m_editor = qobject_cast<TextEditor::ITextEditor *>(editorObject);

    m_popupFrame = new Utils::FakeToolTip(m_editor->widget());

    setParent(m_popupFrame);
    setFocusPolicy(Qt::NoFocus);

    m_pager = new QWidget;
    QHBoxLayout *hbox = new QHBoxLayout(m_pager);
    hbox->setMargin(0);
    hbox->setSpacing(0);
    m_numberLabel = new QLabel;
    hbox->addWidget(m_numberLabel);

    QHBoxLayout *layout = new QHBoxLayout;
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addWidget(m_pager);
    layout->addWidget(this);
    m_popupFrame->setLayout(layout);

    setTextFormat(Qt::RichText);

    qApp->installEventFilter(this);
}

void FunctionArgumentWidget::showFunctionHint(const QString &functionName, const QStringList &signature, int startPosition)
{
    if (m_startpos == startPosition)
        return;

    m_functionName = functionName;
    m_signature = signature;
    m_minimumArgumentCount = signature.size();
    m_startpos = startPosition;
    m_current = 0;
    m_escapePressed = false;

    // update the text
    m_currentarg = -1;
    updateArgumentHighlight();

    m_popupFrame->show();
}

void FunctionArgumentWidget::updateArgumentHighlight()
{
    int curpos = m_editor->position();
    if (curpos < m_startpos) {
        m_popupFrame->close();
        return;
    }

    updateHintText();

    QString str = m_editor->textAt(m_startpos, curpos - m_startpos);
    int argnr = 0;
    int parcount = 0;
    Scanner tokenize;
    const QList<Token> tokens = tokenize(str);
    for (int i = 0; i < tokens.count(); ++i) {
        const Token &tk = tokens.at(i);
        if (tk.is(Token::LeftParenthesis))
            ++parcount;
        else if (tk.is(Token::RightParenthesis))
            --parcount;
        else if (! parcount && tk.is(Token::Colon))
            ++argnr;
    }

    if (m_currentarg != argnr) {
        // m_currentarg = argnr;
        updateHintText();
    }

    if (parcount < 0)
        m_popupFrame->close();
}

bool FunctionArgumentWidget::eventFilter(QObject *obj, QEvent *e)
{
    switch (e->type()) {
    case QEvent::ShortcutOverride:
        if (static_cast<QKeyEvent*>(e)->key() == Qt::Key_Escape) {
            m_escapePressed = true;
        }
        break;
    case QEvent::KeyPress:
        if (static_cast<QKeyEvent*>(e)->key() == Qt::Key_Escape) {
            m_escapePressed = true;
        }
        break;
    case QEvent::KeyRelease:
        if (static_cast<QKeyEvent*>(e)->key() == Qt::Key_Escape && m_escapePressed) {
            m_popupFrame->close();
            return false;
        }
        updateArgumentHighlight();
        break;
    case QEvent::WindowDeactivate:
    case QEvent::FocusOut:
        if (obj != m_editor->widget())
            break;
        m_popupFrame->close();
        break;
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
    case QEvent::Wheel: {
            QWidget *widget = qobject_cast<QWidget *>(obj);
            if (! (widget == this || m_popupFrame->isAncestorOf(widget))) {
                m_popupFrame->close();
            }
        }
        break;
    default:
        break;
    }
    return false;
}

void FunctionArgumentWidget::updateHintText()
{
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

    m_numberLabel->setText(prettyMethod);

    m_popupFrame->setFixedWidth(m_popupFrame->minimumSizeHint().width());

    const QDesktopWidget *desktop = QApplication::desktop();
#ifdef Q_WS_MAC
    const QRect screen = desktop->availableGeometry(desktop->screenNumber(m_editor->widget()));
#else
    const QRect screen = desktop->screenGeometry(desktop->screenNumber(m_editor->widget()));
#endif

    const QSize sz = m_popupFrame->sizeHint();
    QPoint pos = m_editor->cursorRect(m_startpos).topLeft();
    pos.setY(pos.y() - sz.height() - 1);

    if (pos.x() + sz.width() > screen.right())
        pos.setX(screen.right() - sz.width());

    m_popupFrame->move(pos);
}

} } // namespace QmlJSEditor::Internal

CodeCompletion::CodeCompletion(ModelManagerInterface *modelManager, QObject *parent)
    : TextEditor::ICompletionCollector(parent),
      m_modelManager(modelManager),
      m_editor(0),
      m_startPosition(0),
      m_restartCompletion(false),
      m_snippetProvider(Constants::QML_SNIPPETS_GROUP_ID, iconForColor(Qt::red), SnippetOrder)
{
    Q_ASSERT(modelManager);
}

CodeCompletion::~CodeCompletion()
{ }

TextEditor::ITextEditable *CodeCompletion::editor() const
{ return m_editor; }

int CodeCompletion::startPosition() const
{ return m_startPosition; }

bool CodeCompletion::shouldRestartCompletion()
{ return m_restartCompletion; }

bool CodeCompletion::supportsEditor(TextEditor::ITextEditable *editor)
{
    if (qobject_cast<QmlJSTextEditor *>(editor->widget()))
        return true;

    return false;
}

bool CodeCompletion::triggersCompletion(TextEditor::ITextEditable *editor)
{
    if (maybeTriggersCompletion(editor)) {
        // check the token under cursor

        if (QmlJSTextEditor *ed = qobject_cast<QmlJSTextEditor *>(editor->widget())) {

            QTextCursor tc = ed->textCursor();
            QTextBlock block = tc.block();
            const int column = tc.positionInBlock();
            const QChar ch = block.text().at(column - 1);
            const int blockState = qMax(0, block.previous().userState()) & 0xff;
            const QString blockText = block.text();

            Scanner scanner;
            const QList<Token> tokens = scanner(blockText, blockState);
            foreach (const Token &tk, tokens) {
                if (column >= tk.begin() && column <= tk.end()) {
                    if (ch == QLatin1Char('/') && tk.is(Token::String))
                        return true; // path completion inside string literals
                    if (tk.is(Token::Comment) || tk.is(Token::String))
                        return false;
                    break;
                }
            }
            if (ch == QLatin1Char('/'))
                return false;
        }
        return true;
    }

    return false;
}

bool CodeCompletion::maybeTriggersCompletion(TextEditor::ITextEditable *editor)
{
    const int cursorPosition = editor->position();
    const QChar ch = editor->characterAt(cursorPosition - 1);

    if (ch == QLatin1Char('(') || ch == QLatin1Char('.') || ch == QLatin1Char('/'))
        return true;
    if (completionSettings().m_completionTrigger != TextEditor::AutomaticCompletion)
        return false;

    const QChar characterUnderCursor = editor->characterAt(cursorPosition);

    if (isIdentifierChar(ch) && (characterUnderCursor.isSpace() ||
                                      characterUnderCursor.isNull() ||
                                      isDelimiter(characterUnderCursor))) {
        int pos = editor->position() - 1;
        for (; pos != -1; --pos) {
            if (! isIdentifierChar(editor->characterAt(pos)))
                break;
        }
        ++pos;

        const QString word = editor->textAt(pos, cursorPosition - pos);
        if (word.length() > 2 && checkStartOfIdentifier(word)) {
            for (int i = 0; i < word.length(); ++i) {
                if (! isIdentifierChar(word.at(i)))
                    return false;
            }
            return true;
        }
    }

    return false;
}

bool CodeCompletion::isDelimiter(QChar ch) const
{
    switch (ch.unicode()) {
    case '{':
    case '}':
    case '[':
    case ']':
    case ')':
    case '?':
    case '!':
    case ':':
    case ';':
    case ',':
    case '+':
    case '-':
    case '*':
    case '/':
        return true;

    default:
        return false;
    }
}

static bool isLiteral(AST::Node *ast)
{
    if (AST::cast<AST::StringLiteral *>(ast))
        return true;
    else if (AST::cast<AST::NumericLiteral *>(ast))
        return true;
    else
        return false;
}

bool CodeCompletion::completeUrl(const QString &relativeBasePath, const QString &urlString)
{
    const QUrl url(urlString);
    QString fileName = url.toLocalFile();
    if (fileName.isEmpty())
        return false;

    return completeFileName(relativeBasePath, fileName);
}

bool CodeCompletion::completeFileName(const QString &relativeBasePath, const QString &fileName,
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

    QDirIterator dirIterator(directoryPrefix, patterns, QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);
    while (dirIterator.hasNext()) {
        dirIterator.next();
        const QString fileName = dirIterator.fileName();

        TextEditor::CompletionItem item(this);
        item.text += fileName;
        // ### Icon for file completions
        item.icon = iconForColor(Qt::darkBlue);
        m_completions.append(item);
    }

    return !m_completions.isEmpty();
}

void CodeCompletion::addCompletions(const QHash<QString, const Interpreter::Value *> &newCompletions,
                                    const QIcon &icon, int order)
{
    QHashIterator<QString, const Interpreter::Value *> it(newCompletions);
    while (it.hasNext()) {
        it.next();

        TextEditor::CompletionItem item(this);
        item.text = it.key();
        item.icon = icon;
        item.order = order;
        m_completions.append(item);
    }
}

void CodeCompletion::addCompletions(const QStringList &newCompletions,
                                    const QIcon &icon, int order)
{
    foreach (const QString &text, newCompletions) {
        TextEditor::CompletionItem item(this);
        item.text = text;
        item.icon = icon;
        item.order = order;
        m_completions.append(item);
    }
}

void CodeCompletion::addCompletionsPropertyLhs(
        const QHash<QString, const Interpreter::Value *> &newCompletions,
        const QIcon &icon, int order, bool afterOn)
{
    QHashIterator<QString, const Interpreter::Value *> it(newCompletions);
    while (it.hasNext()) {
        it.next();

        TextEditor::CompletionItem item(this);
        item.text = it.key();

        QLatin1String postfix(": ");
        if (afterOn)
            postfix = QLatin1String(" {");
        if (const Interpreter::QmlObjectValue *qmlValue = dynamic_cast<const Interpreter::QmlObjectValue *>(it.value())) {
            // to distinguish "anchors." from "gradient:" we check if the right hand side
            // type is instantiatable or is the prototype of an instantiatable object
            if (qmlValue->hasChildInPackage())
                item.text.append(postfix);
            else
                item.text.append(QLatin1Char('.'));
        } else {
            item.text.append(postfix);
        }
        item.icon = icon;
        item.order = order;
        m_completions.append(item);
    }
}

static const Interpreter::Value *getPropertyValue(
    const Interpreter::ObjectValue *object,
    const QStringList &propertyNames,
    const Interpreter::Context *context)
{
    if (propertyNames.isEmpty() || !object)
        return 0;

    const Interpreter::Value *value = object;
    foreach (const QString &name, propertyNames) {
        if (const Interpreter::ObjectValue *objectValue = value->asObjectValue()) {
            value = objectValue->property(name, context);
            if (!value)
                return 0;
        } else {
            return 0;
        }
    }
    return value;
}

int CodeCompletion::startCompletion(TextEditor::ITextEditable *editor)
{
    m_restartCompletion = false;

    m_editor = editor;

    QmlJSTextEditor *edit = qobject_cast<QmlJSTextEditor *>(m_editor->widget());
    if (! edit)
        return -1;

    m_startPosition = editor->position();
    const QString fileName = editor->file()->fileName();

    while (editor->characterAt(m_startPosition - 1).isLetterOrNumber() ||
           editor->characterAt(m_startPosition - 1) == QLatin1Char('_'))
        --m_startPosition;

    m_completions.clear();

    const SemanticInfo semanticInfo = edit->semanticInfo();

    if (! semanticInfo.isValid())
        return -1;

    const Document::Ptr document = semanticInfo.document;
    const QFileInfo currentFileInfo(fileName);

    bool isQmlFile = false;
    if (currentFileInfo.suffix() == QLatin1String("qml"))
        isQmlFile = true;

    const QIcon symbolIcon = iconForColor(Qt::darkCyan);
    const QIcon keywordIcon = iconForColor(Qt::darkYellow);

    const QList<AST::Node *> path = semanticInfo.astPath(editor->position());
    LookupContext::Ptr lookupContext = semanticInfo.lookupContext(path);
    const Interpreter::Context *context = lookupContext->context();

    // Search for the operator that triggered the completion.
    QChar completionOperator;
    if (m_startPosition > 0)
        completionOperator = editor->characterAt(m_startPosition - 1);

    QTextCursor startPositionCursor(edit->document());
    startPositionCursor.setPosition(m_startPosition);
    CompletionContextFinder contextFinder(startPositionCursor);

    const Interpreter::ObjectValue *qmlScopeType = 0;
    if (contextFinder.isInQmlContext()) {
        // ### this should use semanticInfo.declaringMember instead, but that may also return functions
        for (int i = path.size() - 1; i >= 0; --i) {
            AST::Node *node = path[i];
            if (AST::cast<AST::UiObjectDefinition *>(node) || AST::cast<AST::UiObjectBinding *>(node)) {
                qmlScopeType = document->bind()->findQmlObject(node);
                if (qmlScopeType)
                    break;
            }
        }
        // fallback to getting the base type object
        if (!qmlScopeType)
            qmlScopeType = context->lookupType(document.data(), contextFinder.qmlObjectTypeName());
    }

    if (contextFinder.isInStringLiteral()) {
        // get the text of the literal up to the cursor position
        QTextCursor tc = edit->textCursor();
        QmlExpressionUnderCursor expressionUnderCursor;
        expressionUnderCursor(tc);
        QString literalText = expressionUnderCursor.text();
        QTC_ASSERT(!literalText.isEmpty() && (
                       literalText.at(0) == QLatin1Char('"')
                       || literalText.at(0) == QLatin1Char('\'')), return -1);
        literalText = literalText.mid(1);

        if (contextFinder.isInImport()) {
            QStringList patterns;
            patterns << QLatin1String("*.qml") << QLatin1String("*.js");
            if (completeFileName(document->path(), literalText, patterns))
                return m_startPosition;
            return -1;
        }

        const Interpreter::Value *value = getPropertyValue(qmlScopeType, contextFinder.bindingPropertyName(), context);
        if (!value) {
            // do nothing
        } else if (value->asUrlValue()) {
            if (completeUrl(document->path(), literalText))
                return m_startPosition;
        }

        // ### enum completion?

        // completion gets triggered for / in string literals, if we don't
        // return here, this will mean the snippet completion pops up for
        // each / in a string literal that is not triggering file completion
        return -1;
    } else if (completionOperator.isSpace() || completionOperator.isNull() || isDelimiter(completionOperator) ||
               (completionOperator == QLatin1Char('(') && m_startPosition != editor->position())) {

        bool doGlobalCompletion = true;
        bool doQmlKeywordCompletion = true;
        bool doJsKeywordCompletion = true;

        if (contextFinder.isInLhsOfBinding() && qmlScopeType) {
            doGlobalCompletion = false;
            doJsKeywordCompletion = false;

            EnumerateProperties enumerateProperties(context);
            enumerateProperties.setGlobalCompletion(true);
            enumerateProperties.setEnumerateGeneratedSlots(true);

            // id: is special
            TextEditor::CompletionItem idPropertyCompletion(this);
            idPropertyCompletion.text = QLatin1String("id: ");
            idPropertyCompletion.icon = symbolIcon;
            idPropertyCompletion.order = PropertyOrder;
            m_completions.append(idPropertyCompletion);

            addCompletionsPropertyLhs(enumerateProperties(qmlScopeType), symbolIcon, PropertyOrder, contextFinder.isAfterOnInLhsOfBinding());
            if (const Interpreter::ObjectValue *qmlTypes = context->scopeChain().qmlTypes)
                addCompletions(enumerateProperties(qmlTypes), symbolIcon, TypeOrder);

            if (ScopeBuilder::isPropertyChangesObject(context, qmlScopeType)
                    && context->scopeChain().qmlScopeObjects.size() == 2) {
                addCompletions(enumerateProperties(context->scopeChain().qmlScopeObjects.first()), symbolIcon, SymbolOrder);
            }
        }

        if (contextFinder.isInRhsOfBinding() && qmlScopeType) {
            doQmlKeywordCompletion = false;

            // complete enum values for enum properties
            const Interpreter::Value *value = getPropertyValue(qmlScopeType, contextFinder.bindingPropertyName(), context);
            if (const Interpreter::QmlEnumValue *enumValue = dynamic_cast<const Interpreter::QmlEnumValue *>(value)) {
                foreach (const QString &key, enumValue->keys()) {
                    TextEditor::CompletionItem item(this);
                    item.text = key;
                    item.data = QString("\"%1\"").arg(key);
                    item.icon = symbolIcon;
                    item.order = EnumValueOrder;
                    m_completions.append(item);
                }
            }
        }

        if (doGlobalCompletion) {
            // It's a global completion.
            EnumerateProperties enumerateProperties(context);
            enumerateProperties.setGlobalCompletion(true);
            addCompletions(enumerateProperties(), symbolIcon, SymbolOrder);
        }

        if (doJsKeywordCompletion) {
            // add js keywords
            addCompletions(Scanner::keywords(), keywordIcon, KeywordOrder);
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
            if (qmlWordsAlsoInJs.isEmpty()) {
                qmlWordsAlsoInJs << QLatin1String("default")
                        << QLatin1String("function");
            }

            addCompletions(qmlWords, keywordIcon, KeywordOrder);
            if (!doJsKeywordCompletion)
                addCompletions(qmlWordsAlsoInJs, keywordIcon, KeywordOrder);
        }
    }

    else if (completionOperator == QLatin1Char('.') || completionOperator == QLatin1Char('(')) {
        // Look at the expression under cursor.
        QTextCursor tc = edit->textCursor();
        tc.setPosition(m_startPosition - 1);

        QmlExpressionUnderCursor expressionUnderCursor;
        QmlJS::AST::ExpressionNode *expression = expressionUnderCursor(tc);

        if (expression != 0 && ! isLiteral(expression)) {
            // Evaluate the expression under cursor.
            Interpreter::Engine *interp = lookupContext->engine();
            const Interpreter::Value *value = interp->convertToObject(lookupContext->evaluate(expression));
            //qDebug() << "type:" << interp.typeId(value);

            if (value && completionOperator == QLatin1Char('.')) { // member completion
                EnumerateProperties enumerateProperties(context);
                if (contextFinder.isInLhsOfBinding() && qmlScopeType && expressionUnderCursor.text().at(0).isLower())
                    addCompletionsPropertyLhs(enumerateProperties(value), symbolIcon, PropertyOrder, contextFinder.isAfterOnInLhsOfBinding());
                else
                    addCompletions(enumerateProperties(value), symbolIcon, SymbolOrder);
            } else if (value && completionOperator == QLatin1Char('(') && m_startPosition == editor->position()) {
                // function completion
                if (const Interpreter::FunctionValue *f = value->asFunctionValue()) {
                    QString functionName = expressionUnderCursor.text();
                    int indexOfDot = functionName.lastIndexOf(QLatin1Char('.'));
                    if (indexOfDot != -1)
                        functionName = functionName.mid(indexOfDot + 1);

                    // Recreate if necessary
                    if (!m_functionArgumentWidget)
                        m_functionArgumentWidget = new QmlJSEditor::Internal::FunctionArgumentWidget;

                    QStringList signature;
                    for (int i = 0; i < f->argumentCount(); ++i)
                        signature.append(f->argumentName(i));

                    m_functionArgumentWidget->showFunctionHint(functionName.trimmed(),
                                                               signature,
                                                               m_startPosition);
                }

                return -1; // We always return -1 when completing function prototypes.
            }
        }

        if (! m_completions.isEmpty())
            return m_startPosition;

        return -1;
    }

    if (isQmlFile && (completionOperator.isNull() || completionOperator.isSpace() || isDelimiter(completionOperator))) {
        m_completions.append(m_snippetProvider.getSnippets(this));
    }

    if (! m_completions.isEmpty())
        return m_startPosition;

    return -1;
}

void CodeCompletion::completions(QList<TextEditor::CompletionItem> *completions)
{
    const int length = m_editor->position() - m_startPosition;

    if (length == 0)
        *completions = m_completions;
    else if (length > 0) {
        const QString key = m_editor->textAt(m_startPosition, length);

        filter(m_completions, completions, key);

        if (completions->size() == 1) {
            if (key == completions->first().text)
                completions->clear();
        }
    }
}

bool CodeCompletion::typedCharCompletes(const TextEditor::CompletionItem &item, QChar typedChar)
{
    if (item.data.canConvert<QString>()) // snippet
        return false;

    return (item.text.endsWith(QLatin1String(": ")) && typedChar == QLatin1Char(':'))
            || (item.text.endsWith(QLatin1Char('.')) && typedChar == QLatin1Char('.'));
}

void CodeCompletion::complete(const TextEditor::CompletionItem &item, QChar typedChar)
{
    Q_UNUSED(typedChar) // Currently always included in the completion item when used

    QString toInsert = item.text;

    if (QmlJSTextEditor *edit = qobject_cast<QmlJSTextEditor *>(m_editor->widget())) {
        if (item.data.isValid()) {
            QTextCursor tc = edit->textCursor();
            tc.setPosition(m_startPosition, QTextCursor::KeepAnchor);
            toInsert = item.data.toString();
            edit->insertCodeSnippet(tc, toInsert);
            return;
        }
    }

    QString replacableChars;
    if (toInsert.endsWith(QLatin1String(": ")))
        replacableChars = QLatin1String(": ");
    else if (toInsert.endsWith(QLatin1Char('.')))
        replacableChars = QLatin1String(".");

    int replacedLength = 0;

    // Avoid inserting characters that are already there
    for (int i = 0; i < replacableChars.length(); ++i) {
        const QChar a = replacableChars.at(i);
        const QChar b = m_editor->characterAt(m_editor->position() + i);
        if (a == b)
            ++replacedLength;
        else
            break;
    }

    const int length = m_editor->position() - m_startPosition + replacedLength;
    m_editor->setCurPos(m_startPosition);
    m_editor->replace(length, toInsert);

    if (toInsert.endsWith(QLatin1Char('.')))
        m_restartCompletion = true;
}

bool CodeCompletion::partiallyComplete(const QList<TextEditor::CompletionItem> &completionItems)
{
    if (completionItems.count() == 1) {
        const TextEditor::CompletionItem item = completionItems.first();

        if (!item.data.canConvert<QString>()) {
            complete(item, QChar());
            return true;
        }
    }

    return TextEditor::ICompletionCollector::partiallyComplete(completionItems);
}

void CodeCompletion::cleanup()
{
    m_editor = 0;
    m_startPosition = 0;
    m_completions.clear();
}

static bool qmlCompletionItemLessThan(const TextEditor::CompletionItem &l, const TextEditor::CompletionItem &r)
{
    if (l.order != r.order)
        return l.order > r.order;
    else if (l.text.isEmpty())
        return true;
    else if (r.text.isEmpty())
        return false;
    else if (l.data.isValid() != r.data.isValid())
        return l.data.isValid();
    else if (l.text.at(0).isUpper() && r.text.at(0).isLower())
        return false;
    else if (l.text.at(0).isLower() && r.text.at(0).isUpper())
        return true;

    return l.text < r.text;
}

void CodeCompletion::sortCompletion(QList<TextEditor::CompletionItem> &completionItems)
{
    qStableSort(completionItems.begin(), completionItems.end(), qmlCompletionItemLessThan);
}

QList<TextEditor::CompletionItem> CodeCompletion::getCompletions()
{
    QList<TextEditor::CompletionItem> completionItems;

    completions(&completionItems);

    sortCompletion(completionItems);

    // Remove duplicates
    QString lastKey;
    QVariant lastData;
    QList<TextEditor::CompletionItem> uniquelist;

    foreach (const TextEditor::CompletionItem &item, completionItems) {
        if (item.text != lastKey || item.data.type() != lastData.type()) {
            uniquelist.append(item);
            lastKey = item.text;
            lastData = item.data;
        }
    }

    return uniquelist;
}
