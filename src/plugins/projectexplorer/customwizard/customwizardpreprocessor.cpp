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

#include "customwizardpreprocessor.h"

#include <utils/qtcassert.h>

#include <QtCore/QStringList>
#include <QtCore/QStack>
#include <QtCore/QRegExp>
#include <QtCore/QDebug>

#include <QtScript/QScriptEngine>
#include <QtScript/QScriptValue>

namespace ProjectExplorer {
namespace Internal {

enum  { debug = 0 };

// Preprocessor: Conditional section type.
enum PreprocessorSection {
    IfSection,
    ElsifSection,
    ElseSection,
    EndifSection,
    OtherSection
};

// Preprocessor: Section stack entry containing nested '@if' section
// state.
struct PreprocessStackEntry {
    PreprocessStackEntry(PreprocessorSection section = OtherSection,
                         bool parentEnabled = true,
                         bool condition = false,
                         bool anyIfClauseMatched = false);

    PreprocessorSection section;
    bool parentEnabled;
    bool condition;          // Current 'if/elsif' section is enabled.
    bool anyIfClauseMatched; // Determines if 'else' triggers
};

PreprocessStackEntry::PreprocessStackEntry(PreprocessorSection s,
                                           bool p, bool c, bool a) :
    section(s), parentEnabled(p), condition(c), anyIfClauseMatched(a)
{
}

// Context for preprocessing.
class PreprocessContext {
public:
    PreprocessContext();
    bool process(const QString &in, QString *out, QString *errorMessage);

private:
    void reset();
    PreprocessorSection preprocessorLine(const QString & in, QString *ifExpression) const;

    mutable QRegExp m_ifPattern;
    mutable QRegExp m_elsifPattern;
    const QRegExp m_elsePattern;
    const QRegExp m_endifPattern;

    QStack<PreprocessStackEntry> m_sectionStack;
    QScriptEngine m_scriptEngine;
};

PreprocessContext::PreprocessContext() :
    // Cut out expression for 'if/elsif '
    m_ifPattern(QLatin1String("^([\\s]*@[\\s]*if[\\s]*)(.*)$")),
    m_elsifPattern(QLatin1String("^([\\s]*@[\\s]*elsif[\\s]*)(.*)$")),
    m_elsePattern(QLatin1String("^[\\s]*@[\\s]*else.*$")),
    m_endifPattern(QLatin1String("^[\\s]*@[\\s]*endif.*$"))
{
    QTC_ASSERT(m_ifPattern.isValid() && m_elsifPattern.isValid()
               && m_elsePattern.isValid() &&m_endifPattern.isValid(), return);
}

void PreprocessContext::reset()
{
    m_sectionStack.clear(); // Add a default, enabled section.
    m_sectionStack.push(PreprocessStackEntry(OtherSection, true, true));
}

// Determine type of line and return enumeration, cut out
// expression for '@if/@elsif'.
PreprocessorSection PreprocessContext::preprocessorLine(const QString &in,
                                                        QString *ifExpression) const
{
    if (m_ifPattern.exactMatch(in)) {
        *ifExpression = m_ifPattern.cap(2).trimmed();
        return IfSection;
    }
    if (m_elsifPattern.exactMatch(in)) {
        *ifExpression = m_elsifPattern.cap(2).trimmed();
        return ElsifSection;
    }

    ifExpression->clear();

    if (m_elsePattern.exactMatch(in))
        return ElseSection;
    if (m_endifPattern.exactMatch(in))
        return EndifSection;
    return OtherSection;
}

// Evaluate an expression within an 'if'/'elsif' to a bool via QScript
bool evaluateBooleanJavaScriptExpression(QScriptEngine &engine, const QString &expression, bool *result, QString *errorMessage)
{
    errorMessage->clear();
    *result = false;
    engine.clearExceptions();
    const QScriptValue value = engine.evaluate(expression);
    if (engine.hasUncaughtException()) {
        *errorMessage = QString::fromLatin1("Error in '%1': %2").
                        arg(expression, engine.uncaughtException().toString());
        return false;
    }
    // Try to convert to bool, be that an int or whatever.
    if (value.isBool()) {
        *result = value.toBool();
        return true;
    }
    if (value.isNumber()) {
        *result = !qFuzzyCompare(value.toNumber(), 0);
        return true;
    }
    if (value.isString()) {
        *result = !value.toString().isEmpty();
        return true;
    }
    *errorMessage = QString::fromLatin1("Cannot convert result of '%1' ('%2'to bool.").
                        arg(expression, value.toString());
    return false;
}

static inline QString msgEmptyStack(int line)
{
    return QString::fromLatin1("Unmatched '@endif' at line %1.").arg(line);
}

bool PreprocessContext::process(const QString &in, QString *out, QString *errorMessage)
{
    out->clear();
    if (in.isEmpty())
        return true;

    out->reserve(in.size());
    reset();

    const QChar newLine = QLatin1Char('\n');
    const QStringList lines = in.split(newLine, QString::KeepEmptyParts);
    const int lineCount = lines.size();
    for (int l = 0; l < lineCount; l++) {
        // Check for element of the stack (be it dummy, else something is wrong).
        if (m_sectionStack.isEmpty()) {
            *errorMessage = msgEmptyStack(l);
            return false;
        }
        QString expression;
        bool expressionValue = false;
        PreprocessStackEntry &top = m_sectionStack.back();

        switch (preprocessorLine(lines.at(l), &expression)) {
        case IfSection:
            // '@If': Push new section
            if (top.parentEnabled) {
                if (!evaluateBooleanJavaScriptExpression(m_scriptEngine, expression, &expressionValue, errorMessage)) {
                    *errorMessage = QString::fromLatin1("Error in @if at %1: %2").
                            arg(l + 1).arg(*errorMessage);
                    return false;
                }
            }
            if (debug)
                qDebug("'%s' : expr='%s' -> %d", qPrintable(lines.at(l)), qPrintable(expression), expressionValue);
            m_sectionStack.push(PreprocessStackEntry(IfSection,
                                                     top.condition, expressionValue, expressionValue));
            break;
        case ElsifSection: // '@elsif': Check condition.
            if (top.section != IfSection && top.section != ElsifSection) {
                *errorMessage = QString::fromLatin1("No preceding @if found for @elsif at %1").
                                arg(l + 1);
                return false;
            }
            if (top.parentEnabled) {
                if (!evaluateBooleanJavaScriptExpression(m_scriptEngine, expression, &expressionValue, errorMessage)) {
                    *errorMessage = QString::fromLatin1("Error in @elsif at %1: %2").
                            arg(l + 1).arg(*errorMessage);
                    return false;
                }
            }
            if (debug)
                qDebug("'%s' : expr='%s' -> %d", qPrintable(lines.at(l)), qPrintable(expression), expressionValue);
            top.section = ElsifSection;
            // ignore consecutive '@elsifs' once something matched
            if (top.anyIfClauseMatched) {
                top.condition = false;
            } else {
                if ( (top.condition = expressionValue) )
                    top.anyIfClauseMatched = true;
            }
            break;
        case ElseSection: // '@else': Check condition.
            if (top.section != IfSection && top.section != ElsifSection) {
                *errorMessage = QString::fromLatin1("No preceding @if/@elsif found for @else at %1").
                                                    arg(l + 1);
                return false;
            }
            expressionValue = top.parentEnabled && !top.anyIfClauseMatched;
            if (debug)
                qDebug("%s -> %d", qPrintable(lines.at(l)), expressionValue);
            top.section = ElseSection;
            top.condition = expressionValue;
            break;
        case EndifSection: // '@endif': Discard section.
            m_sectionStack.pop();
            break;
        case OtherSection: // Rest: Append according to current condition.
            if (top.condition) {
                out->append(lines.at(l));
                out->append(newLine);
            }
            break;
        } // switch section

    } // for lines
    return true;
}

bool customWizardPreprocess(const QString &in, QString *out, QString *errorMessage)
{
    PreprocessContext context;
    return context.process(in, out, errorMessage);
}

} // namespace Internal
} // namespace ProjectExplorer
