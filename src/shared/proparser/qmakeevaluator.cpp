/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#include "qmakeevaluator.h"

#include "qmakeglobals.h"
#include "qmakeparser.h"
#include "qmakeevaluator_p.h"
#include "ioutils.h"

#include <QByteArray>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QList>
#include <QRegExp>
#include <QSet>
#include <QStack>
#include <QString>
#include <QStringList>
#ifdef PROEVALUATOR_THREAD_SAFE
# include <QThreadPool>
#endif

#ifdef Q_OS_UNIX
#include <unistd.h>
#include <sys/utsname.h>
#else
#include <Windows.h>
#endif
#include <stdio.h>
#include <stdlib.h>

using namespace ProFileEvaluatorInternal;

QT_BEGIN_NAMESPACE

using namespace ProStringConstants;

#define fL1S(s) QString::fromLatin1(s)


QMakeBaseEnv::QMakeBaseEnv()
    : evaluator(0)
{
#ifdef PROEVALUATOR_THREAD_SAFE
    inProgress = false;
#endif
}

QMakeBaseEnv::~QMakeBaseEnv()
{
    delete evaluator;
}

namespace ProFileEvaluatorInternal {
QMakeStatics statics;
}

void QMakeEvaluator::initStatics()
{
    if (!statics.field_sep.isNull())
        return;

    statics.field_sep = QLatin1String(" ");
    statics.strtrue = QLatin1String("true");
    statics.strfalse = QLatin1String("false");
    statics.strCONFIG = ProString("CONFIG");
    statics.strARGS = ProString("ARGS");
    statics.strDot = QLatin1String(".");
    statics.strDotDot = QLatin1String("..");
    statics.strever = QLatin1String("ever");
    statics.strforever = QLatin1String("forever");
    statics.strTEMPLATE = ProString("TEMPLATE");

    statics.fakeValue = ProStringList(ProString("_FAKE_")); // It has to have a unique begin() value

    initFunctionStatics();

    static const struct {
        const char * const oldname, * const newname;
    } mapInits[] = {
        { "INTERFACES", "FORMS" },
        { "QMAKE_POST_BUILD", "QMAKE_POST_LINK" },
        { "TARGETDEPS", "POST_TARGETDEPS" },
        { "LIBPATH", "QMAKE_LIBDIR" },
        { "QMAKE_EXT_MOC", "QMAKE_EXT_CPP_MOC" },
        { "QMAKE_MOD_MOC", "QMAKE_H_MOD_MOC" },
        { "QMAKE_LFLAGS_SHAPP", "QMAKE_LFLAGS_APP" },
        { "PRECOMPH", "PRECOMPILED_HEADER" },
        { "PRECOMPCPP", "PRECOMPILED_SOURCE" },
        { "INCPATH", "INCLUDEPATH" },
        { "QMAKE_EXTRA_WIN_COMPILERS", "QMAKE_EXTRA_COMPILERS" },
        { "QMAKE_EXTRA_UNIX_COMPILERS", "QMAKE_EXTRA_COMPILERS" },
        { "QMAKE_EXTRA_WIN_TARGETS", "QMAKE_EXTRA_TARGETS" },
        { "QMAKE_EXTRA_UNIX_TARGETS", "QMAKE_EXTRA_TARGETS" },
        { "QMAKE_EXTRA_UNIX_INCLUDES", "QMAKE_EXTRA_INCLUDES" },
        { "QMAKE_EXTRA_UNIX_VARIABLES", "QMAKE_EXTRA_VARIABLES" },
        { "QMAKE_RPATH", "QMAKE_LFLAGS_RPATH" },
        { "QMAKE_FRAMEWORKDIR", "QMAKE_FRAMEWORKPATH" },
        { "QMAKE_FRAMEWORKDIR_FLAGS", "QMAKE_FRAMEWORKPATH_FLAGS" },
        { "IN_PWD", "PWD" }
    };
    for (unsigned i = 0; i < sizeof(mapInits)/sizeof(mapInits[0]); ++i)
        statics.varMap.insert(ProString(mapInits[i].oldname),
                              ProString(mapInits[i].newname));
}

const ProString &QMakeEvaluator::map(const ProString &var)
{
    QHash<ProString, ProString>::ConstIterator it = statics.varMap.constFind(var);
    return (it != statics.varMap.constEnd()) ? it.value() : var;
}


QMakeEvaluator::QMakeEvaluator(QMakeGlobals *option,
                               QMakeParser *parser, QMakeHandler *handler)
  : m_option(option), m_parser(parser), m_handler(handler)
{
    // So that single-threaded apps don't have to call initialize() for now.
    initStatics();

    // Configuration, more or less
#ifdef PROEVALUATOR_CUMULATIVE
    m_cumulative = false;
#endif

    // Evaluator state
    m_skipLevel = 0;
    m_loopLevel = 0;
    m_listCount = 0;
    m_valuemapStack.push(ProValueMap());
}

QMakeEvaluator::~QMakeEvaluator()
{
}

void QMakeEvaluator::initFrom(const QMakeEvaluator &other)
{
    Q_ASSERT_X(&other, "QMakeEvaluator::visitProFile", "Project not prepared");
    m_functionDefs = other.m_functionDefs;
    m_valuemapStack = other.m_valuemapStack;
    m_qmakespec = other.m_qmakespec;
    m_qmakespecFull = other.m_qmakespecFull;
    m_qmakespecName = other.m_qmakespecName;
    m_qmakepath = other.m_qmakepath;
    m_qmakefeatures = other.m_qmakefeatures;
    m_featureRoots = other.m_featureRoots;
}

//////// Evaluator tools /////////

uint QMakeEvaluator::getBlockLen(const ushort *&tokPtr)
{
    uint len = *tokPtr++;
    len |= (uint)*tokPtr++ << 16;
    return len;
}

ProString QMakeEvaluator::getStr(const ushort *&tokPtr)
{
    uint len = *tokPtr++;
    ProString ret(m_current.pro->items(), tokPtr - m_current.pro->tokPtr(), len, NoHash);
    ret.setSource(m_current.pro);
    tokPtr += len;
    return ret;
}

ProString QMakeEvaluator::getHashStr(const ushort *&tokPtr)
{
    uint hash = getBlockLen(tokPtr);
    uint len = *tokPtr++;
    ProString ret(m_current.pro->items(), tokPtr - m_current.pro->tokPtr(), len, hash);
    tokPtr += len;
    return ret;
}

void QMakeEvaluator::skipStr(const ushort *&tokPtr)
{
    uint len = *tokPtr++;
    tokPtr += len;
}

void QMakeEvaluator::skipHashStr(const ushort *&tokPtr)
{
    tokPtr += 2;
    uint len = *tokPtr++;
    tokPtr += len;
}

// FIXME: this should not build new strings for direct sections.
// Note that the E_SPRINTF and E_LIST implementations rely on the deep copy.
ProStringList QMakeEvaluator::split_value_list(const QString &vals, const ProFile *source)
{
    QString build;
    ProStringList ret;
    QStack<char> quote;

    const ushort SPACE = ' ';
    const ushort LPAREN = '(';
    const ushort RPAREN = ')';
    const ushort SINGLEQUOTE = '\'';
    const ushort DOUBLEQUOTE = '"';
    const ushort BACKSLASH = '\\';

    if (!source)
        source = currentProFile();

    ushort unicode;
    const QChar *vals_data = vals.data();
    const int vals_len = vals.length();
    for (int x = 0, parens = 0; x < vals_len; x++) {
        unicode = vals_data[x].unicode();
        if (x != (int)vals_len-1 && unicode == BACKSLASH &&
            (vals_data[x+1].unicode() == SINGLEQUOTE || vals_data[x+1].unicode() == DOUBLEQUOTE)) {
            build += vals_data[x++]; //get that 'escape'
        } else if (!quote.isEmpty() && unicode == quote.top()) {
            quote.pop();
        } else if (unicode == SINGLEQUOTE || unicode == DOUBLEQUOTE) {
            quote.push(unicode);
        } else if (unicode == RPAREN) {
            --parens;
        } else if (unicode == LPAREN) {
            ++parens;
        }

        if (!parens && quote.isEmpty() && vals_data[x] == SPACE) {
            ret << ProString(build, NoHash).setSource(source);
            build.clear();
        } else {
            build += vals_data[x];
        }
    }
    if (!build.isEmpty())
        ret << ProString(build, NoHash).setSource(source);
    return ret;
}

static void zipEmpty(ProStringList *value)
{
    for (int i = value->size(); --i >= 0;)
        if (value->at(i).isEmpty())
            value->remove(i);
}

static void insertUnique(ProStringList *varlist, const ProStringList &value)
{
    foreach (const ProString &str, value)
        if (!str.isEmpty() && !varlist->contains(str))
            varlist->append(str);
}

static void removeAll(ProStringList *varlist, const ProString &value)
{
    for (int i = varlist->size(); --i >= 0; )
        if (varlist->at(i) == value)
            varlist->remove(i);
}

static void removeEach(ProStringList *varlist, const ProStringList &value)
{
    foreach (const ProString &str, value)
        if (!str.isEmpty())
            removeAll(varlist, str);
}

static void replaceInList(ProStringList *varlist,
        const QRegExp &regexp, const QString &replace, bool global, QString &tmp)
{
    for (ProStringList::Iterator varit = varlist->begin(); varit != varlist->end(); ) {
        QString val = varit->toQString(tmp);
        QString copy = val; // Force detach and have a reference value
        val.replace(regexp, replace);
        if (!val.isSharedWith(copy)) {
            if (val.isEmpty()) {
                varit = varlist->erase(varit);
            } else {
                (*varit).setValue(val, NoHash);
                ++varit;
            }
            if (!global)
                break;
        } else {
            ++varit;
        }
    }
}

// This is braindead, but we want qmake compat
QString QMakeEvaluator::fixPathToLocalOS(const QString &str) const
{
    QString string = m_option->expandEnvVars(str);

    if (string.length() > 2 && string.at(0).isLetter() && string.at(1) == QLatin1Char(':'))
        string[0] = string[0].toLower();

#if defined(Q_OS_WIN32)
    string.replace(QLatin1Char('/'), QLatin1Char('\\'));
#else
    string.replace(QLatin1Char('\\'), QLatin1Char('/'));
#endif
    return string;
}

//////// Evaluator /////////

static ALWAYS_INLINE void addStr(
        const ProString &str, ProStringList *ret, bool &pending, bool joined)
{
    if (joined) {
        ret->last().append(str, &pending);
    } else {
        if (!pending) {
            pending = true;
            *ret << str;
        } else {
            ret->last().append(str);
        }
    }
}

static ALWAYS_INLINE void addStrList(
        const ProStringList &list, ushort tok, ProStringList *ret, bool &pending, bool joined)
{
    if (!list.isEmpty()) {
        if (joined) {
            ret->last().append(list, &pending, !(tok & TokQuoted));
        } else {
            if (tok & TokQuoted) {
                if (!pending) {
                    pending = true;
                    *ret << ProString();
                }
                ret->last().append(list);
            } else {
                if (!pending) {
                    // Another qmake bizzarity: if nothing is pending and the
                    // first element is empty, it will be eaten
                    if (!list.at(0).isEmpty()) {
                        // The common case
                        pending = true;
                        *ret += list;
                        return;
                    }
                } else {
                    ret->last().append(list.at(0));
                }
                // This is somewhat slow, but a corner case
                for (int j = 1; j < list.size(); ++j) {
                    pending = true;
                    *ret << list.at(j);
                }
            }
        }
    }
}

void QMakeEvaluator::evaluateExpression(
        const ushort *&tokPtr, ProStringList *ret, bool joined)
{
    if (joined)
        *ret << ProString();
    bool pending = false;
    forever {
        ushort tok = *tokPtr++;
        if (tok & TokNewStr)
            pending = false;
        ushort maskedTok = tok & TokMask;
        switch (maskedTok) {
        case TokLine:
            m_current.line = *tokPtr++;
            break;
        case TokLiteral:
            addStr(getStr(tokPtr), ret, pending, joined);
            break;
        case TokHashLiteral:
            addStr(getHashStr(tokPtr), ret, pending, joined);
            break;
        case TokVariable:
            addStrList(values(map(getHashStr(tokPtr))), tok, ret, pending, joined);
            break;
        case TokProperty:
            addStr(propertyValue(getHashStr(tokPtr)).setSource(currentProFile()),
                   ret, pending, joined);
            break;
        case TokEnvVar:
            addStrList(split_value_list(m_option->getEnv(getStr(tokPtr).toQString(m_tmp1))),
                       tok, ret, pending, joined);
            break;
        case TokFuncName: {
            ProString func = getHashStr(tokPtr);
            addStrList(evaluateExpandFunction(func, tokPtr), tok, ret, pending, joined);
            break; }
        default:
            tokPtr--;
            return;
        }
    }
}

void QMakeEvaluator::skipExpression(const ushort *&pTokPtr)
{
    const ushort *tokPtr = pTokPtr;
    forever {
        ushort tok = *tokPtr++;
        switch (tok) {
        case TokLine:
            m_current.line = *tokPtr++;
            break;
        case TokValueTerminator:
        case TokFuncTerminator:
            pTokPtr = tokPtr;
            return;
        case TokArgSeparator:
            break;
        default:
            switch (tok & TokMask) {
            case TokLiteral:
            case TokEnvVar:
                skipStr(tokPtr);
                break;
            case TokHashLiteral:
            case TokVariable:
            case TokProperty:
                skipHashStr(tokPtr);
                break;
            case TokFuncName:
                skipHashStr(tokPtr);
                pTokPtr = tokPtr;
                skipExpression(pTokPtr);
                tokPtr = pTokPtr;
                break;
            default:
                Q_ASSERT_X(false, "skipExpression", "Unrecognized token");
                break;
            }
        }
    }
}

QMakeEvaluator::VisitReturn QMakeEvaluator::visitProBlock(
        ProFile *pro, const ushort *tokPtr)
{
    m_current.pro = pro;
    m_current.line = 0;
    return visitProBlock(tokPtr);
}

QMakeEvaluator::VisitReturn QMakeEvaluator::visitProBlock(
        const ushort *tokPtr)
{
    ProStringList curr;
    bool okey = true, or_op = false, invert = false;
    uint blockLen;
    VisitReturn ret = ReturnTrue;
    while (ushort tok = *tokPtr++) {
        switch (tok) {
        case TokLine:
            m_current.line = *tokPtr++;
            continue;
        case TokAssign:
        case TokAppend:
        case TokAppendUnique:
        case TokRemove:
        case TokReplace:
            visitProVariable(tok, curr, tokPtr);
            curr.clear();
            continue;
        case TokBranch:
            blockLen = getBlockLen(tokPtr);
            if (m_cumulative) {
                if (!okey)
                    m_skipLevel++;
                ret = blockLen ? visitProBlock(tokPtr) : ReturnTrue;
                tokPtr += blockLen;
                blockLen = getBlockLen(tokPtr);
                if (!okey)
                    m_skipLevel--;
                else
                    m_skipLevel++;
                if ((ret == ReturnTrue || ret == ReturnFalse) && blockLen)
                    ret = visitProBlock(tokPtr);
                if (okey)
                    m_skipLevel--;
            } else {
                if (okey)
                    ret = blockLen ? visitProBlock(tokPtr) : ReturnTrue;
                tokPtr += blockLen;
                blockLen = getBlockLen(tokPtr);
                if (!okey)
                    ret = blockLen ? visitProBlock(tokPtr) : ReturnTrue;
            }
            tokPtr += blockLen;
            okey = true, or_op = false; // force next evaluation
            break;
        case TokForLoop:
            if (m_cumulative) { // This is a no-win situation, so just pretend it's no loop
                skipHashStr(tokPtr);
                uint exprLen = getBlockLen(tokPtr);
                tokPtr += exprLen;
                blockLen = getBlockLen(tokPtr);
                ret = visitProBlock(tokPtr);
            } else if (okey != or_op) {
                const ProString &variable = getHashStr(tokPtr);
                uint exprLen = getBlockLen(tokPtr);
                const ushort *exprPtr = tokPtr;
                tokPtr += exprLen;
                blockLen = getBlockLen(tokPtr);
                ret = visitProLoop(variable, exprPtr, tokPtr);
            } else {
                skipHashStr(tokPtr);
                uint exprLen = getBlockLen(tokPtr);
                tokPtr += exprLen;
                blockLen = getBlockLen(tokPtr);
                ret = ReturnTrue;
            }
            tokPtr += blockLen;
            okey = true, or_op = false; // force next evaluation
            break;
        case TokTestDef:
        case TokReplaceDef:
            if (m_cumulative || okey != or_op) {
                const ProString &name = getHashStr(tokPtr);
                blockLen = getBlockLen(tokPtr);
                visitProFunctionDef(tok, name, tokPtr);
            } else {
                skipHashStr(tokPtr);
                blockLen = getBlockLen(tokPtr);
            }
            tokPtr += blockLen;
            okey = true, or_op = false; // force next evaluation
            continue;
        case TokNot:
            invert ^= true;
            continue;
        case TokAnd:
            or_op = false;
            continue;
        case TokOr:
            or_op = true;
            continue;
        case TokCondition:
            if (!m_skipLevel && okey != or_op) {
                if (curr.size() != 1) {
                    if (!m_cumulative || !curr.isEmpty())
                        evalError(fL1S("Conditional must expand to exactly one word."));
                    okey = false;
                } else {
                    okey = isActiveConfig(curr.at(0).toQString(m_tmp2), true) ^ invert;
                }
            }
            or_op = !okey; // tentatively force next evaluation
            invert = false;
            curr.clear();
            continue;
        case TokTestCall:
            if (!m_skipLevel && okey != or_op) {
                if (curr.size() != 1) {
                    if (!m_cumulative || !curr.isEmpty())
                        evalError(fL1S("Test name must expand to exactly one word."));
                    skipExpression(tokPtr);
                    okey = false;
                } else {
                    ret = evaluateConditionalFunction(curr.at(0), tokPtr);
                    switch (ret) {
                    case ReturnTrue: okey = true; break;
                    case ReturnFalse: okey = false; break;
                    default: return ret;
                    }
                    okey ^= invert;
                }
            } else if (m_cumulative) {
                m_skipLevel++;
                if (curr.size() != 1)
                    skipExpression(tokPtr);
                else
                    evaluateConditionalFunction(curr.at(0), tokPtr);
                m_skipLevel--;
            } else {
                skipExpression(tokPtr);
            }
            or_op = !okey; // tentatively force next evaluation
            invert = false;
            curr.clear();
            continue;
        default: {
                const ushort *oTokPtr = --tokPtr;
                evaluateExpression(tokPtr, &curr, false);
                if (tokPtr != oTokPtr)
                    continue;
            }
            Q_ASSERT_X(false, "visitProBlock", "unexpected item type");
        }
        if (ret != ReturnTrue && ret != ReturnFalse)
            break;
    }
    return ret;
}


void QMakeEvaluator::visitProFunctionDef(
        ushort tok, const ProString &name, const ushort *tokPtr)
{
    QHash<ProString, ProFunctionDef> *hash =
            (tok == TokTestDef
             ? &m_functionDefs.testFunctions
             : &m_functionDefs.replaceFunctions);
    hash->insert(name, ProFunctionDef(m_current.pro, tokPtr - m_current.pro->tokPtr()));
}

QMakeEvaluator::VisitReturn QMakeEvaluator::visitProLoop(
        const ProString &_variable, const ushort *exprPtr, const ushort *tokPtr)
{
    VisitReturn ret = ReturnTrue;
    bool infinite = false;
    int index = 0;
    ProString variable;
    ProStringList oldVarVal;
    ProString it_list = expandVariableReferences(exprPtr, 0, true).at(0);
    if (_variable.isEmpty()) {
        if (it_list != statics.strever) {
            evalError(fL1S("Invalid loop expression."));
            return ReturnFalse;
        }
        it_list = ProString(statics.strforever);
    } else {
        variable = map(_variable);
        oldVarVal = values(variable);
    }
    ProStringList list = values(it_list);
    if (list.isEmpty()) {
        if (it_list == statics.strforever) {
            infinite = true;
        } else {
            const QString &itl = it_list.toQString(m_tmp1);
            int dotdot = itl.indexOf(statics.strDotDot);
            if (dotdot != -1) {
                bool ok;
                int start = itl.left(dotdot).toInt(&ok);
                if (ok) {
                    int end = itl.mid(dotdot+2).toInt(&ok);
                    if (ok) {
                        if (start < end) {
                            for (int i = start; i <= end; i++)
                                list << ProString(QString::number(i), NoHash);
                        } else {
                            for (int i = start; i >= end; i--)
                                list << ProString(QString::number(i), NoHash);
                        }
                    }
                }
            }
        }
    }

    m_loopLevel++;
    forever {
        if (infinite) {
            if (!variable.isEmpty())
                m_valuemapStack.top()[variable] = ProStringList(ProString(QString::number(index++), NoHash));
            if (index > 1000) {
                evalError(fL1S("ran into infinite loop (> 1000 iterations)."));
                break;
            }
        } else {
            ProString val;
            do {
                if (index >= list.count())
                    goto do_break;
                val = list.at(index++);
            } while (val.isEmpty()); // stupid, but qmake is like that
            m_valuemapStack.top()[variable] = ProStringList(val);
        }

        ret = visitProBlock(tokPtr);
        switch (ret) {
        case ReturnTrue:
        case ReturnFalse:
            break;
        case ReturnNext:
            ret = ReturnTrue;
            break;
        case ReturnBreak:
            ret = ReturnTrue;
            goto do_break;
        default:
            goto do_break;
        }
    }
  do_break:
    m_loopLevel--;

    if (!variable.isEmpty())
        m_valuemapStack.top()[variable] = oldVarVal;
    return ret;
}

void QMakeEvaluator::visitProVariable(
        ushort tok, const ProStringList &curr, const ushort *&tokPtr)
{
    int sizeHint = *tokPtr++;

    if (curr.size() != 1) {
        skipExpression(tokPtr);
        if (!m_cumulative || !curr.isEmpty())
            evalError(fL1S("Left hand side of assignment must expand to exactly one word."));
        return;
    }
    const ProString &varName = map(curr.first());

    if (tok == TokReplace) {      // ~=
        // DEFINES ~= s/a/b/?[gqi]

        const ProStringList &varVal = expandVariableReferences(tokPtr, sizeHint, true);
        const QString &val = varVal.at(0).toQString(m_tmp1);
        if (val.length() < 4 || val.at(0) != QLatin1Char('s')) {
            evalError(fL1S("the ~= operator can handle only the s/// function."));
            return;
        }
        QChar sep = val.at(1);
        QStringList func = val.split(sep);
        if (func.count() < 3 || func.count() > 4) {
            evalError(fL1S("the s/// function expects 3 or 4 arguments."));
            return;
        }

        bool global = false, quote = false, case_sense = false;
        if (func.count() == 4) {
            global = func[3].indexOf(QLatin1Char('g')) != -1;
            case_sense = func[3].indexOf(QLatin1Char('i')) == -1;
            quote = func[3].indexOf(QLatin1Char('q')) != -1;
        }
        QString pattern = func[1];
        QString replace = func[2];
        if (quote)
            pattern = QRegExp::escape(pattern);

        QRegExp regexp(pattern, case_sense ? Qt::CaseSensitive : Qt::CaseInsensitive);

        if (!m_skipLevel || m_cumulative) {
            // We could make a union of modified and unmodified values,
            // but this will break just as much as it fixes, so leave it as is.
            replaceInList(&valuesRef(varName), regexp, replace, global, m_tmp2);
        }
    } else {
        ProStringList varVal = expandVariableReferences(tokPtr, sizeHint);
        switch (tok) {
        default: // whatever - cannot happen
        case TokAssign:          // =
            if (!m_cumulative) {
                if (!m_skipLevel) {
                    zipEmpty(&varVal);
                    m_valuemapStack.top()[varName] = varVal;
                }
            } else {
                zipEmpty(&varVal);
                if (!varVal.isEmpty()) {
                    // We are greedy for values. But avoid exponential growth.
                    ProStringList &v = valuesRef(varName);
                    if (v.isEmpty()) {
                        v = varVal;
                    } else {
                        ProStringList old = v;
                        v = varVal;
                        QSet<ProString> has;
                        has.reserve(v.size());
                        foreach (const ProString &s, v)
                            has.insert(s);
                        v.reserve(v.size() + old.size());
                        foreach (const ProString &s, old)
                            if (!has.contains(s))
                                v << s;
                    }
                }
            }
            break;
        case TokAppendUnique:    // *=
            if (!m_skipLevel || m_cumulative)
                insertUnique(&valuesRef(varName), varVal);
            break;
        case TokAppend:          // +=
            if (!m_skipLevel || m_cumulative) {
                zipEmpty(&varVal);
                valuesRef(varName) += varVal;
            }
            break;
        case TokRemove:       // -=
            if (!m_cumulative) {
                if (!m_skipLevel)
                    removeEach(&valuesRef(varName), varVal);
            } else {
                // We are stingy with our values, too.
            }
            break;
        }
    }

    if (varName == statics.strTEMPLATE)
        setTemplate();
}

void QMakeEvaluator::setTemplate()
{
    ProStringList &values = valuesRef(statics.strTEMPLATE);
    if (!m_option->user_template.isEmpty()) {
        // Don't allow override
        values = ProStringList(ProString(m_option->user_template, NoHash));
    } else {
        if (values.isEmpty())
            values.append(ProString("app", NoHash));
        else
            values.erase(values.begin() + 1, values.end());
    }
    if (!m_option->user_template_prefix.isEmpty()) {
        QString val = values.first().toQString(m_tmp1);
        if (!val.startsWith(m_option->user_template_prefix)) {
            val.prepend(m_option->user_template_prefix);
            values = ProStringList(ProString(val, NoHash));
        }
    }
}

void QMakeEvaluator::loadDefaults()
{
    ProValueMap &vars = m_valuemapStack.top();

    vars[ProString("LITERAL_WHITESPACE")] << ProString("\t", NoHash);
    vars[ProString("LITERAL_DOLLAR")] << ProString("$", NoHash);
    vars[ProString("LITERAL_HASH")] << ProString("#", NoHash);
    vars[ProString("DIR_SEPARATOR")] << ProString(m_option->dir_sep, NoHash);
    vars[ProString("DIRLIST_SEPARATOR")] << ProString(m_option->dirlist_sep, NoHash);
    vars[ProString("_DATE_")] << ProString(QDateTime::currentDateTime().toString(), NoHash);
    if (!m_option->qmake_abslocation.isEmpty())
        vars[ProString("QMAKE_QMAKE")] << ProString(m_option->qmake_abslocation, NoHash);
#if defined(Q_OS_WIN32)
    vars[ProString("QMAKE_HOST.os")] << ProString("Windows", NoHash);

    DWORD name_length = 1024;
    wchar_t name[1024];
    if (GetComputerName(name, &name_length))
        vars[ProString("QMAKE_HOST.name")] << ProString(QString::fromWCharArray(name), NoHash);

    QSysInfo::WinVersion ver = QSysInfo::WindowsVersion;
    vars[ProString("QMAKE_HOST.version")] << ProString(QString::number(ver), NoHash);
    ProString verStr;
    switch (ver) {
    case QSysInfo::WV_Me: verStr = ProString("WinMe", NoHash); break;
    case QSysInfo::WV_95: verStr = ProString("Win95", NoHash); break;
    case QSysInfo::WV_98: verStr = ProString("Win98", NoHash); break;
    case QSysInfo::WV_NT: verStr = ProString("WinNT", NoHash); break;
    case QSysInfo::WV_2000: verStr = ProString("Win2000", NoHash); break;
    case QSysInfo::WV_2003: verStr = ProString("Win2003", NoHash); break;
    case QSysInfo::WV_XP: verStr = ProString("WinXP", NoHash); break;
    case QSysInfo::WV_VISTA: verStr = ProString("WinVista", NoHash); break;
    default: verStr = ProString("Unknown", NoHash); break;
    }
    vars[ProString("QMAKE_HOST.version_string")] << verStr;

    SYSTEM_INFO info;
    GetSystemInfo(&info);
    ProString archStr;
    switch (info.wProcessorArchitecture) {
# ifdef PROCESSOR_ARCHITECTURE_AMD64
    case PROCESSOR_ARCHITECTURE_AMD64:
        archStr = ProString("x86_64", NoHash);
        break;
# endif
    case PROCESSOR_ARCHITECTURE_INTEL:
        archStr = ProString("x86", NoHash);
        break;
    case PROCESSOR_ARCHITECTURE_IA64:
# ifdef PROCESSOR_ARCHITECTURE_IA32_ON_WIN64
    case PROCESSOR_ARCHITECTURE_IA32_ON_WIN64:
# endif
        archStr = ProString("IA64", NoHash);
        break;
    default:
        archStr = ProString("Unknown", NoHash);
        break;
    }
    vars[ProString("QMAKE_HOST.arch")] << archStr;

# if defined(Q_CC_MSVC) // ### bogus condition, but nobody x-builds for msvc with a different qmake
    QLatin1Char backslash('\\');
    QString paths = m_option->getEnv(QLatin1String("PATH"));
    QString vcBin64 = m_option->getEnv(QLatin1String("VCINSTALLDIR"));
    if (!vcBin64.endsWith(backslash))
        vcBin64.append(backslash);
    vcBin64.append(QLatin1String("bin\\amd64"));
    QString vcBinX86_64 = m_option->getEnv(QLatin1String("VCINSTALLDIR"));
    if (!vcBinX86_64.endsWith(backslash))
        vcBinX86_64.append(backslash);
    vcBinX86_64.append(QLatin1String("bin\\x86_amd64"));
    if (paths.contains(vcBin64, Qt::CaseInsensitive)
            || paths.contains(vcBinX86_64, Qt::CaseInsensitive))
        vars[ProString("QMAKE_TARGET.arch")] << ProString("x86_64", NoHash);
    else
        vars[ProString("QMAKE_TARGET.arch")] << ProString("x86", NoHash);
# endif
#elif defined(Q_OS_UNIX)
    struct utsname name;
    if (!uname(&name)) {
        vars[ProString("QMAKE_HOST.os")] << ProString(name.sysname, NoHash);
        vars[ProString("QMAKE_HOST.name")] << ProString(QString::fromLocal8Bit(name.nodename), NoHash);
        vars[ProString("QMAKE_HOST.version")] << ProString(name.release, NoHash);
        vars[ProString("QMAKE_HOST.version_string")] << ProString(name.version, NoHash);
        vars[ProString("QMAKE_HOST.arch")] << ProString(name.machine, NoHash);
    }
#endif
}

bool QMakeEvaluator::prepareProject(const QString &inDir)
{
    QString superdir;
    if (m_option->do_cache) {
        QString conffile;
        QString cachefile = m_option->cachefile;
        if (cachefile.isEmpty())  { //find it as it has not been specified
            if (m_outputDir.isEmpty())
                goto no_cache;
            superdir = m_outputDir;
            forever {
                QString superfile = superdir + QLatin1String("/.qmake.super");
                if (IoUtils::exists(superfile)) {
                    m_superfile = superfile;
                    break;
                }
                QFileInfo qdfi(superdir);
                if (qdfi.isRoot()) {
                    superdir.clear();
                    break;
                }
                superdir = qdfi.path();
            }
            QString sdir = inDir;
            QString dir = m_outputDir;
            forever {
                conffile = sdir + QLatin1String("/.qmake.conf");
                if (!IoUtils::exists(conffile))
                    conffile.clear();
                cachefile = dir + QLatin1String("/.qmake.cache");
                if (!IoUtils::exists(cachefile))
                    cachefile.clear();
                if (!conffile.isEmpty() || !cachefile.isEmpty()) {
                    m_sourceRoot = sdir;
                    m_buildRoot = dir;
                    break;
                }
                if (dir == superdir)
                    goto no_cache;
                QFileInfo qsdfi(sdir);
                QFileInfo qdfi(dir);
                if (qsdfi.isRoot() || qdfi.isRoot())
                    goto no_cache;
                sdir = qsdfi.path();
                dir = qdfi.path();
            }
        } else {
            m_buildRoot = QFileInfo(cachefile).path();
        }
        m_conffile = conffile;
        m_cachefile = cachefile;
    }
  no_cache:

    // Look for mkspecs/ in source and build. First to win determines the root.
    QString sdir = inDir;
    QString dir = m_outputDir;
    while (dir != m_buildRoot) {
        if ((dir != sdir && QFileInfo(sdir, QLatin1String("mkspecs")).isDir())
                || QFileInfo(dir, QLatin1String("mkspecs")).isDir()) {
            if (dir != sdir)
                m_sourceRoot = sdir;
            m_buildRoot = dir;
            break;
        }
        if (dir == superdir)
            break;
        QFileInfo qsdfi(sdir);
        QFileInfo qdfi(dir);
        if (qsdfi.isRoot() || qdfi.isRoot())
            break;
        sdir = qsdfi.path();
        dir = qdfi.path();
    }

    return true;
}

bool QMakeEvaluator::loadSpec()
{
    loadDefaults();

    QString qmakespec = m_option->expandEnvVars(m_option->qmakespec);

    {
        QMakeEvaluator evaluator(m_option, m_parser, m_handler);
        if (!m_superfile.isEmpty()) {
            valuesRef(ProString("_QMAKE_SUPER_CACHE_")) << ProString(m_superfile, NoHash);
            if (!evaluator.evaluateFileDirect(m_superfile, QMakeHandler::EvalConfigFile, LoadProOnly))
                return false;
        }
        if (!m_conffile.isEmpty()) {
            valuesRef(ProString("_QMAKE_CONF_")) << ProString(m_conffile, NoHash);
            if (!evaluator.evaluateFileDirect(m_conffile, QMakeHandler::EvalConfigFile, LoadProOnly))
                return false;
        }
        if (!m_cachefile.isEmpty()) {
            valuesRef(ProString("_QMAKE_CACHE_")) << ProString(m_cachefile, NoHash);
            if (!evaluator.evaluateFileDirect(m_cachefile, QMakeHandler::EvalConfigFile, LoadProOnly))
                return false;
        }
        if (qmakespec.isEmpty())
            qmakespec = evaluator.first(ProString("QMAKESPEC")).toQString();
        m_qmakepath = evaluator.values(ProString("QMAKEPATH")).toQStringList();
        m_qmakefeatures = evaluator.values(ProString("QMAKEFEATURES")).toQStringList();
    }

    if (qmakespec.isEmpty())
        qmakespec = QLatin1String("default");
    if (IoUtils::isRelativePath(qmakespec)) {
        foreach (const QString &root, qmakeMkspecPaths()) {
            QString mkspec = root + QLatin1Char('/') + qmakespec;
            if (IoUtils::exists(mkspec)) {
                qmakespec = mkspec;
                goto cool;
            }
        }
        m_handler->configError(fL1S("Could not find qmake configuration file"));
        return false;
    }
  cool:
    m_qmakespec = QDir::cleanPath(qmakespec);

    if (!m_superfile.isEmpty()
        && !evaluateFileDirect(m_superfile, QMakeHandler::EvalConfigFile, LoadProOnly)) {
        return false;
    }
    if (!evaluateFeatureFile(QLatin1String("spec_pre.prf")))
        return false;
    QString spec = m_qmakespec + QLatin1String("/qmake.conf");
    if (!evaluateFileDirect(spec, QMakeHandler::EvalConfigFile, LoadProOnly)) {
        m_handler->configError(
                fL1S("Could not read qmake configuration file %1").arg(spec));
        return false;
    }
#ifdef Q_OS_UNIX
    m_qmakespecFull = QFileInfo(m_qmakespec).canonicalFilePath();
#else
    // We can't resolve symlinks as they do on Unix, so configure.exe puts
    // the source of the qmake.conf at the end of the default/qmake.conf in
    // the QMAKESPEC_ORIGINAL variable.
    const ProString &orig_spec = first(ProString("QMAKESPEC_ORIGINAL"));
    m_qmakespecFull = orig_spec.isEmpty() ? m_qmakespec : orig_spec.toQString();
#endif
    valuesRef(ProString("QMAKESPEC")) << ProString(m_qmakespecFull, NoHash);
    m_qmakespecName = IoUtils::fileName(m_qmakespecFull).toString();
    if (!evaluateFeatureFile(QLatin1String("spec_post.prf")))
        return false;
    // The spec extends the feature search path, so invalidate the cache.
    m_featureRoots.clear();
    if (!m_conffile.isEmpty()
        && !evaluateFileDirect(m_conffile, QMakeHandler::EvalConfigFile, LoadProOnly)) {
        return false;
    }
    if (!m_cachefile.isEmpty()
        && !evaluateFileDirect(m_cachefile, QMakeHandler::EvalConfigFile, LoadProOnly)) {
        return false;
    }
    return true;
}

void QMakeEvaluator::setupProject()
{
    setTemplate();
    ProValueMap &vars = m_valuemapStack.top();
    vars[ProString("TARGET")] << ProString(QFileInfo(currentFileName()).baseName(), NoHash);
    vars[ProString("_PRO_FILE_")] << ProString(currentFileName(), NoHash);
    vars[ProString("_PRO_FILE_PWD_")] << ProString(currentDirectory(), NoHash);
    vars[ProString("OUT_PWD")] << ProString(m_outputDir, NoHash);
}

void QMakeEvaluator::visitCmdLine(const QString &cmds)
{
    if (!cmds.isEmpty()) {
        if (ProFile *pro = m_parser->parsedProBlock(fL1S("(command line)"), cmds)) {
            m_locationStack.push(m_current);
            visitProBlock(pro, pro->tokPtr());
            m_current = m_locationStack.pop();
            pro->deref();
        }
    }
}

QMakeEvaluator::VisitReturn QMakeEvaluator::visitProFile(
        ProFile *pro, QMakeHandler::EvalFileType type, LoadFlags flags)
{
    if (!m_cumulative && !pro->isOk())
        return ReturnFalse;

    if (flags & LoadPreFiles) {
        if (!prepareProject(pro->directoryName()))
            return ReturnFalse;

#ifdef PROEVALUATOR_THREAD_SAFE
        m_option->mutex.lock();
#endif
        QMakeBaseEnv **baseEnvPtr = &m_option->baseEnvs[QMakeBaseKey(m_buildRoot)];
        if (!*baseEnvPtr)
            *baseEnvPtr = new QMakeBaseEnv;
        QMakeBaseEnv *baseEnv = *baseEnvPtr;

#ifdef PROEVALUATOR_THREAD_SAFE
        {
            QMutexLocker locker(&baseEnv->mutex);
            m_option->mutex.unlock();
            if (baseEnv->inProgress) {
                QThreadPool::globalInstance()->releaseThread();
                baseEnv->cond.wait(&baseEnv->mutex);
                QThreadPool::globalInstance()->reserveThread();
                if (!baseEnv->isOk)
                    return ReturnFalse;
            } else
#endif
            if (!baseEnv->evaluator) {
#ifdef PROEVALUATOR_THREAD_SAFE
                baseEnv->inProgress = true;
                locker.unlock();
#endif

                QMakeEvaluator *baseEval = new QMakeEvaluator(m_option, m_parser, m_handler);
                baseEnv->evaluator = baseEval;
                baseEval->m_superfile = m_superfile;
                baseEval->m_conffile = m_conffile;
                baseEval->m_cachefile = m_cachefile;
                baseEval->m_sourceRoot = m_sourceRoot;
                baseEval->m_buildRoot = m_buildRoot;
                bool ok = baseEval->loadSpec();

#ifdef PROEVALUATOR_THREAD_SAFE
                locker.relock();
                baseEnv->isOk = ok;
                baseEnv->inProgress = false;
                baseEnv->cond.wakeAll();
#endif

                if (!ok)
                    return ReturnFalse;
            }
#ifdef PROEVALUATOR_THREAD_SAFE
        }
#endif

        initFrom(*baseEnv->evaluator);
    }

    m_handler->aboutToEval(currentProFile(), pro, type);
    m_profileStack.push(pro);
    valuesRef(ProString("PWD")) = ProStringList(ProString(currentDirectory(), NoHash));
    if (flags & LoadPreFiles) {
        setupProject();

        evaluateFeatureFile(QLatin1String("default_pre.prf"));

        visitCmdLine(m_option->precmds);
    }

    visitProBlock(pro, pro->tokPtr());

    if (flags & LoadPostFiles) {
        visitCmdLine(m_option->postcmds);

        evaluateFeatureFile(QLatin1String("default_post.prf"));

        QSet<QString> processed;
        forever {
            bool finished = true;
            ProStringList configs = values(statics.strCONFIG);
            for (int i = configs.size() - 1; i >= 0; --i) {
                QString config = configs.at(i).toQString(m_tmp1).toLower();
                if (!processed.contains(config)) {
                    config.detach();
                    processed.insert(config);
                    if (evaluateFeatureFile(config)) {
                        finished = false;
                        break;
                    }
                }
            }
            if (finished)
                break;
        }
    }
    m_profileStack.pop();
    valuesRef(ProString("PWD")) = ProStringList(ProString(currentDirectory(), NoHash));
    m_handler->doneWithEval(currentProFile());

    return ReturnTrue;
}


QStringList QMakeEvaluator::qmakeMkspecPaths() const
{
    QStringList ret;
    const QString concat = QLatin1String("/mkspecs");

    foreach (const QString &it, m_option->getPathListEnv(QLatin1String("QMAKEPATH")))
        ret << it + concat;

    foreach (const QString &it, m_qmakepath)
        ret << it + concat;

    if (!m_buildRoot.isEmpty())
        ret << m_buildRoot + concat;
    if (!m_sourceRoot.isEmpty())
        ret << m_sourceRoot + concat;

    ret << m_option->propertyValue(ProString("QT_HOST_DATA")) + concat;

    ret.removeDuplicates();
    return ret;
}

QStringList QMakeEvaluator::qmakeFeaturePaths() const
{
    QString mkspecs_concat = QLatin1String("/mkspecs");
    QString features_concat = QLatin1String("/features/");

    QStringList feature_roots;

    foreach (const QString &f, m_option->getPathListEnv(QLatin1String("QMAKEFEATURES")))
        feature_roots += f;

    feature_roots += m_qmakefeatures;

    feature_roots += m_option->propertyValue(ProString("QMAKEFEATURES")).toQString(m_mtmp).split(
            m_option->dirlist_sep, QString::SkipEmptyParts);

    QStringList feature_bases;
    if (!m_buildRoot.isEmpty())
        feature_bases << m_buildRoot;
    if (!m_sourceRoot.isEmpty())
        feature_bases << m_sourceRoot;

    foreach (const QString &item, m_option->getPathListEnv(QLatin1String("QMAKEPATH")))
        feature_bases << (item + mkspecs_concat);

    foreach (const QString &item, m_qmakepath)
        feature_bases << (item + mkspecs_concat);

    if (!m_qmakespecFull.isEmpty()) {
        // The spec is already platform-dependent, so no subdirs here.
        feature_roots << (m_qmakespecFull + features_concat);

        // Also check directly under the root directory of the mkspecs collection
        QDir specdir(m_qmakespecFull);
        while (!specdir.isRoot() && specdir.cdUp()) {
            const QString specpath = specdir.path();
            if (specpath.endsWith(mkspecs_concat)) {
                if (IoUtils::exists(specpath + features_concat))
                    feature_bases << specpath;
                break;
            }
        }
    }

    feature_bases << (m_option->propertyValue(ProString("QT_HOST_DATA")).toQString(m_mtmp)
                      + mkspecs_concat);

    foreach (const QString &fb, feature_bases) {
        foreach (const ProString &sfx, values(ProString("QMAKE_PLATFORM")))
            feature_roots << (fb + features_concat + sfx + QLatin1Char('/'));
        feature_roots << (fb + features_concat);
    }

    for (int i = 0; i < feature_roots.count(); ++i)
        if (!feature_roots.at(i).endsWith((ushort)'/'))
            feature_roots[i].append((ushort)'/');

    feature_roots.removeDuplicates();

    QStringList ret;
    foreach (const QString &root, feature_roots)
        if (IoUtils::exists(root))
            ret << root;
    return ret;
}

ProString QMakeEvaluator::propertyValue(const ProString &name) const
{
    if (name == QLatin1String("QMAKE_MKSPECS"))
        return ProString(qmakeMkspecPaths().join(m_option->dirlist_sep), NoHash);
    ProString ret = m_option->propertyValue(name);
    if (ret.isNull())
        evalError(fL1S("Querying unknown property %1").arg(name.toQString(m_mtmp)));
    return ret;
}

ProFile *QMakeEvaluator::currentProFile() const
{
    if (m_profileStack.count() > 0)
        return m_profileStack.top();
    return 0;
}

QString QMakeEvaluator::currentFileName() const
{
    ProFile *pro = currentProFile();
    if (pro)
        return pro->fileName();
    return QString();
}

QString QMakeEvaluator::currentDirectory() const
{
    ProFile *pro = currentProFile();
    if (pro)
        return pro->directoryName();
    return QString();
}

// The (QChar*)current->constData() constructs below avoid pointless detach() calls
// FIXME: This is inefficient. Should not make new string if it is a straight subsegment
static ALWAYS_INLINE void appendChar(ushort unicode,
    QString *current, QChar **ptr, ProString *pending)
{
    if (!pending->isEmpty()) {
        int len = pending->size();
        current->resize(current->size() + len);
        ::memcpy((QChar*)current->constData(), pending->constData(), len * 2);
        pending->clear();
        *ptr = (QChar*)current->constData() + len;
    }
    *(*ptr)++ = QChar(unicode);
}

static void appendString(const ProString &string,
    QString *current, QChar **ptr, ProString *pending)
{
    if (string.isEmpty())
        return;
    QChar *uc = (QChar*)current->constData();
    int len;
    if (*ptr != uc) {
        len = *ptr - uc;
        current->resize(current->size() + string.size());
    } else if (!pending->isEmpty()) {
        len = pending->size();
        current->resize(current->size() + len + string.size());
        ::memcpy((QChar*)current->constData(), pending->constData(), len * 2);
        pending->clear();
    } else {
        *pending = string;
        return;
    }
    *ptr = (QChar*)current->constData() + len;
    ::memcpy(*ptr, string.constData(), string.size() * 2);
    *ptr += string.size();
}

static void flushCurrent(ProStringList *ret,
    QString *current, QChar **ptr, ProString *pending, bool joined)
{
    QChar *uc = (QChar*)current->constData();
    int len = *ptr - uc;
    if (len) {
        ret->append(ProString(QString(uc, len), NoHash));
        *ptr = uc;
    } else if (!pending->isEmpty()) {
        ret->append(*pending);
        pending->clear();
    } else if (joined) {
        ret->append(ProString());
    }
}

static inline void flushFinal(ProStringList *ret,
    const QString &current, const QChar *ptr, const ProString &pending,
    const ProString &str, bool replaced, bool joined)
{
    int len = ptr - current.data();
    if (len) {
        if (!replaced && len == str.size())
            ret->append(str);
        else
            ret->append(ProString(QString(current.data(), len), NoHash));
    } else if (!pending.isEmpty()) {
        ret->append(pending);
    } else if (joined) {
        ret->append(ProString());
    }
}

ProStringList QMakeEvaluator::expandVariableReferences(
    const ProString &str, int *pos, bool joined)
{
    ProStringList ret;
//    if (ok)
//        *ok = true;
    if (str.isEmpty() && !pos)
        return ret;

    const ushort LSQUARE = '[';
    const ushort RSQUARE = ']';
    const ushort LCURLY = '{';
    const ushort RCURLY = '}';
    const ushort LPAREN = '(';
    const ushort RPAREN = ')';
    const ushort DOLLAR = '$';
    const ushort BACKSLASH = '\\';
    const ushort UNDERSCORE = '_';
    const ushort DOT = '.';
    const ushort SPACE = ' ';
    const ushort TAB = '\t';
    const ushort COMMA = ',';
    const ushort SINGLEQUOTE = '\'';
    const ushort DOUBLEQUOTE = '"';

    ushort unicode, quote = 0, parens = 0;
    const ushort *str_data = (const ushort *)str.constData();
    const int str_len = str.size();

    ProString var, args;

    bool replaced = false;
    bool putSpace = false;
    QString current; // Buffer for successively assembled string segments
    current.resize(str.size());
    QChar *ptr = current.data();
    ProString pending; // Buffer for string segments from variables
    // Only one of the above buffers can be filled at a given time.
    for (int i = pos ? *pos : 0; i < str_len; ++i) {
        unicode = str_data[i];
        if (unicode == DOLLAR) {
            if (str_len > i+2 && str_data[i+1] == DOLLAR) {
                ++i;
                ushort term = 0;
                enum { VAR, ENVIRON, FUNCTION, PROPERTY } var_type = VAR;
                unicode = str_data[++i];
                if (unicode == LSQUARE) {
                    unicode = str_data[++i];
                    term = RSQUARE;
                    var_type = PROPERTY;
                } else if (unicode == LCURLY) {
                    unicode = str_data[++i];
                    var_type = VAR;
                    term = RCURLY;
                } else if (unicode == LPAREN) {
                    unicode = str_data[++i];
                    var_type = ENVIRON;
                    term = RPAREN;
                }
                int name_start = i;
                forever {
                    if (!(unicode & (0xFF<<8)) &&
                       unicode != DOT && unicode != UNDERSCORE &&
                       //unicode != SINGLEQUOTE && unicode != DOUBLEQUOTE &&
                       (unicode < 'a' || unicode > 'z') && (unicode < 'A' || unicode > 'Z') &&
                       (unicode < '0' || unicode > '9'))
                        break;
                    if (++i == str_len)
                        break;
                    unicode = str_data[i];
                    // at this point, i points to either the 'term' or 'next' character (which is in unicode)
                }
                var = str.mid(name_start, i - name_start);
                if (var_type == VAR && unicode == LPAREN) {
                    var_type = FUNCTION;
                    name_start = i + 1;
                    int depth = 0;
                    forever {
                        if (++i == str_len)
                            break;
                        unicode = str_data[i];
                        if (unicode == LPAREN) {
                            depth++;
                        } else if (unicode == RPAREN) {
                            if (!depth)
                                break;
                            --depth;
                        }
                    }
                    args = str.mid(name_start, i - name_start);
                    if (++i < str_len)
                        unicode = str_data[i];
                    else
                        unicode = 0;
                    // at this point i is pointing to the 'next' character (which is in unicode)
                    // this might actually be a term character since you can do $${func()}
                }
                if (term) {
                    if (unicode != term) {
                        evalError(fL1S("Missing %1 terminator [found %2]")
                                  .arg(QChar(term))
                                  .arg(unicode ? QString(unicode) : fL1S("end-of-line")));
//                        if (ok)
//                            *ok = false;
                        if (pos)
                            *pos = str_len;
                        return ProStringList();
                    }
                } else {
                    // move the 'cursor' back to the last char of the thing we were looking at
                    --i;
                }

                ProStringList replacement;
                if (var_type == ENVIRON) {
                    replacement = split_value_list(m_option->getEnv(var.toQString(m_tmp1)));
                } else if (var_type == PROPERTY) {
                    replacement << propertyValue(var);
                } else if (var_type == FUNCTION) {
                    replacement += evaluateExpandFunction(var, args);
                } else if (var_type == VAR) {
                    replacement = values(map(var));
                }
                if (!replacement.isEmpty()) {
                    if (quote || joined) {
                        if (putSpace) {
                            putSpace = false;
                            if (!replacement.at(0).isEmpty()) // Bizarre, indeed
                                appendChar(' ', &current, &ptr, &pending);
                        }
                        appendString(ProString(replacement.join(statics.field_sep), NoHash),
                                     &current, &ptr, &pending);
                    } else {
                        appendString(replacement.at(0), &current, &ptr, &pending);
                        if (replacement.size() > 1) {
                            flushCurrent(&ret, &current, &ptr, &pending, false);
                            int j = 1;
                            if (replacement.size() > 2) {
                                // FIXME: ret.reserve(ret.size() + replacement.size() - 2);
                                for (; j < replacement.size() - 1; ++j)
                                    ret << replacement.at(j);
                            }
                            pending = replacement.at(j);
                        }
                    }
                    replaced = true;
                }
                continue;
            }
        } else if (unicode == BACKSLASH) {
            static const char symbols[] = "[]{}()$\\'\"";
            ushort unicode2 = str_data[i+1];
            if (!(unicode2 & 0xff00) && strchr(symbols, unicode2)) {
                unicode = unicode2;
                ++i;
            }
        } else if (quote) {
            if (unicode == quote) {
                quote = 0;
                continue;
            }
        } else {
            if (unicode == SINGLEQUOTE || unicode == DOUBLEQUOTE) {
                quote = unicode;
                continue;
            } else if (unicode == SPACE || unicode == TAB) {
                if (!joined)
                    flushCurrent(&ret, &current, &ptr, &pending, false);
                else if ((ptr - (QChar*)current.constData()) || !pending.isEmpty())
                    putSpace = true;
                continue;
            } else if (pos) {
                if (unicode == LPAREN) {
                    ++parens;
                } else if (unicode == RPAREN) {
                    --parens;
                } else if (!parens && unicode == COMMA) {
                    if (!joined) {
                        *pos = i + 1;
                        flushFinal(&ret, current, ptr, pending, str, replaced, false);
                        return ret;
                    }
                    flushCurrent(&ret, &current, &ptr, &pending, true);
                    putSpace = false;
                    continue;
                }
            }
        }
        if (putSpace) {
            putSpace = false;
            appendChar(' ', &current, &ptr, &pending);
        }
        appendChar(unicode, &current, &ptr, &pending);
    }
    if (pos)
        *pos = str_len;
    flushFinal(&ret, current, ptr, pending, str, replaced, joined);
    return ret;
}

bool QMakeEvaluator::isActiveConfig(const QString &config, bool regex)
{
    // magic types for easy flipping
    if (config == statics.strtrue)
        return true;
    if (config == statics.strfalse)
        return false;

    if (regex && (config.contains(QLatin1Char('*')) || config.contains(QLatin1Char('?')))) {
        QString cfg = config;
        cfg.detach(); // Keep m_tmp out of QRegExp's cache
        QRegExp re(cfg, Qt::CaseSensitive, QRegExp::Wildcard);

        // mkspecs
        if (re.exactMatch(m_qmakespecName))
            return true;

        // CONFIG variable
        int t = 0;
        foreach (const ProString &configValue, values(statics.strCONFIG)) {
            if (re.exactMatch(configValue.toQString(m_tmp[t])))
                return true;
            t ^= 1;
        }
    } else {
        // mkspecs
        if (m_qmakespecName == config)
            return true;

        // CONFIG variable
        if (values(statics.strCONFIG).contains(ProString(config, NoHash)))
            return true;
    }

    return false;
}

ProStringList QMakeEvaluator::expandVariableReferences(
        const ushort *&tokPtr, int sizeHint, bool joined)
{
    ProStringList ret;
    ret.reserve(sizeHint);
    forever {
        evaluateExpression(tokPtr, &ret, joined);
        switch (*tokPtr) {
        case TokValueTerminator:
        case TokFuncTerminator:
            tokPtr++;
            return ret;
        case TokArgSeparator:
            if (joined) {
                tokPtr++;
                continue;
            }
            // fallthrough
        default:
            Q_ASSERT_X(false, "expandVariableReferences", "Unrecognized token");
            break;
        }
    }
}

QList<ProStringList> QMakeEvaluator::prepareFunctionArgs(const ushort *&tokPtr)
{
    QList<ProStringList> args_list;
    if (*tokPtr != TokFuncTerminator) {
        for (;; tokPtr++) {
            ProStringList arg;
            evaluateExpression(tokPtr, &arg, false);
            args_list << arg;
            if (*tokPtr == TokFuncTerminator)
                break;
            Q_ASSERT(*tokPtr == TokArgSeparator);
        }
    }
    tokPtr++;
    return args_list;
}

QList<ProStringList> QMakeEvaluator::prepareFunctionArgs(const ProString &arguments)
{
    QList<ProStringList> args_list;
    for (int pos = 0; pos < arguments.size(); )
        args_list << expandVariableReferences(arguments, &pos);
    return args_list;
}

ProStringList QMakeEvaluator::evaluateFunction(
        const ProFunctionDef &func, const QList<ProStringList> &argumentsList, bool *ok)
{
    bool oki;
    ProStringList ret;

    if (m_valuemapStack.count() >= 100) {
        evalError(fL1S("ran into infinite recursion (depth > 100)."));
        oki = false;
    } else {
        m_valuemapStack.push(ProValueMap());
        m_locationStack.push(m_current);
        int loopLevel = m_loopLevel;
        m_loopLevel = 0;

        ProStringList args;
        for (int i = 0; i < argumentsList.count(); ++i) {
            args += argumentsList[i];
            m_valuemapStack.top()[ProString(QString::number(i+1))] = argumentsList[i];
        }
        m_valuemapStack.top()[statics.strARGS] = args;
        oki = (visitProBlock(func.pro(), func.tokPtr()) != ReturnFalse); // True || Return
        ret = m_returnValue;
        m_returnValue.clear();

        m_loopLevel = loopLevel;
        m_current = m_locationStack.pop();
        m_valuemapStack.pop();
    }
    if (ok)
        *ok = oki;
    if (oki)
        return ret;
    return ProStringList();
}

QMakeEvaluator::VisitReturn QMakeEvaluator::evaluateBoolFunction(
        const ProFunctionDef &func, const QList<ProStringList> &argumentsList,
        const ProString &function)
{
    bool ok;
    ProStringList ret = evaluateFunction(func, argumentsList, &ok);
    if (ok) {
        if (ret.isEmpty())
            return ReturnTrue;
        if (ret.at(0) != statics.strfalse) {
            if (ret.at(0) == statics.strtrue)
                return ReturnTrue;
            int val = ret.at(0).toQString(m_tmp1).toInt(&ok);
            if (ok) {
                if (val)
                    return ReturnTrue;
            } else {
                evalError(fL1S("Unexpected return value from test '%1': %2")
                          .arg(function.toQString(m_tmp1))
                          .arg(ret.join(QLatin1String(" :: "))));
            }
        }
    }
    return ReturnFalse;
}

ProStringList QMakeEvaluator::evaluateExpandFunction(
        const ProString &func, const ushort *&tokPtr)
{
    QHash<ProString, ProFunctionDef>::ConstIterator it =
            m_functionDefs.replaceFunctions.constFind(func);
    if (it != m_functionDefs.replaceFunctions.constEnd())
        return evaluateFunction(*it, prepareFunctionArgs(tokPtr), 0);

    //why don't the builtin functions just use args_list? --Sam
    return evaluateExpandFunction(func, expandVariableReferences(tokPtr, 5, true));
}

ProStringList QMakeEvaluator::evaluateExpandFunction(
        const ProString &func, const ProString &arguments)
{
    QHash<ProString, ProFunctionDef>::ConstIterator it =
            m_functionDefs.replaceFunctions.constFind(func);
    if (it != m_functionDefs.replaceFunctions.constEnd())
        return evaluateFunction(*it, prepareFunctionArgs(arguments), 0);

    //why don't the builtin functions just use args_list? --Sam
    int pos = 0;
    return evaluateExpandFunction(func, expandVariableReferences(arguments, &pos, true));
}

QMakeEvaluator::VisitReturn QMakeEvaluator::evaluateConditionalFunction(
        const ProString &function, const ProString &arguments)
{
    QHash<ProString, ProFunctionDef>::ConstIterator it =
            m_functionDefs.testFunctions.constFind(function);
    if (it != m_functionDefs.testFunctions.constEnd())
        return evaluateBoolFunction(*it, prepareFunctionArgs(arguments), function);

    //why don't the builtin functions just use args_list? --Sam
    int pos = 0;
    return evaluateConditionalFunction(function, expandVariableReferences(arguments, &pos, true));
}

QMakeEvaluator::VisitReturn QMakeEvaluator::evaluateConditionalFunction(
        const ProString &function, const ushort *&tokPtr)
{
    QHash<ProString, ProFunctionDef>::ConstIterator it =
            m_functionDefs.testFunctions.constFind(function);
    if (it != m_functionDefs.testFunctions.constEnd())
        return evaluateBoolFunction(*it, prepareFunctionArgs(tokPtr), function);

    //why don't the builtin functions just use args_list? --Sam
    return evaluateConditionalFunction(function, expandVariableReferences(tokPtr, 5, true));
}

ProValueMap *QMakeEvaluator::findValues(const ProString &variableName, ProValueMap::Iterator *rit)
{
    for (int i = m_valuemapStack.size(); --i >= 0; ) {
        ProValueMap::Iterator it = m_valuemapStack[i].find(variableName);
        if (it != m_valuemapStack[i].end()) {
            if (it->constBegin() == statics.fakeValue.constBegin())
                return 0;
            *rit = it;
            return &m_valuemapStack[i];
        }
    }
    return 0;
}

ProStringList &QMakeEvaluator::valuesRef(const ProString &variableName)
{
    ProValueMap::Iterator it = m_valuemapStack.top().find(variableName);
    if (it != m_valuemapStack.top().end()) {
        if (it->constBegin() == statics.fakeValue.constBegin())
            it->clear();
        return *it;
    }
    for (int i = m_valuemapStack.size() - 1; --i >= 0; ) {
        ProValueMap::ConstIterator it = m_valuemapStack.at(i).constFind(variableName);
        if (it != m_valuemapStack.at(i).constEnd()) {
            ProStringList &ret = m_valuemapStack.top()[variableName];
            if (it->constBegin() != statics.fakeValue.constBegin())
                ret = *it;
            return ret;
        }
    }
    return m_valuemapStack.top()[variableName];
}

ProStringList QMakeEvaluator::values(const ProString &variableName) const
{
    for (int i = m_valuemapStack.size(); --i >= 0; ) {
        ProValueMap::ConstIterator it = m_valuemapStack.at(i).constFind(variableName);
        if (it != m_valuemapStack.at(i).constEnd()) {
            if (it->constBegin() == statics.fakeValue.constBegin())
                break;
            return *it;
        }
    }
    return ProStringList();
}

ProString QMakeEvaluator::first(const ProString &variableName) const
{
    const ProStringList &vals = values(variableName);
    if (!vals.isEmpty())
        return vals.first();
    return ProString();
}

bool QMakeEvaluator::evaluateFileDirect(
        const QString &fileName, QMakeHandler::EvalFileType type, LoadFlags flags)
{
    if (ProFile *pro = m_parser->parsedProFile(fileName, true)) {
        m_locationStack.push(m_current);
        bool ok = (visitProFile(pro, type, flags) == ReturnTrue);
        m_current = m_locationStack.pop();
        pro->deref();
#ifdef PROEVALUATOR_FULL
        if (ok) {
            ProStringList &iif = m_valuemapStack.first()[ProString("QMAKE_INTERNAL_INCLUDED_FILES")];
            ProString ifn(fileName, NoHash);
            if (!iif.contains(ifn))
                iif << ifn;
        }
#endif
        return ok;
    } else {
        return false;
    }
}

bool QMakeEvaluator::evaluateFile(
        const QString &fileName, QMakeHandler::EvalFileType type, LoadFlags flags)
{
    if (fileName.isEmpty())
        return false;
    foreach (const ProFile *pf, m_profileStack)
        if (pf->fileName() == fileName) {
            evalError(fL1S("circular inclusion of %1").arg(fileName));
            return false;
        }
    return evaluateFileDirect(fileName, type, flags);
}

bool QMakeEvaluator::evaluateFeatureFile(const QString &fileName)
{
    QString fn = fileName;
    if (!fn.endsWith(QLatin1String(".prf")))
        fn += QLatin1String(".prf");

    if (m_featureRoots.isEmpty())
        m_featureRoots = qmakeFeaturePaths();
    int start_root = 0;
    QString currFn = currentFileName();
    if (IoUtils::fileName(currFn) == IoUtils::fileName(fn)) {
        for (int root = 0; root < m_featureRoots.size(); ++root)
            if (currFn == m_featureRoots.at(root) + fn) {
                start_root = root + 1;
                break;
            }
    }
    for (int root = start_root; root < m_featureRoots.size(); ++root) {
        QString fname = m_featureRoots.at(root) + fn;
        if (IoUtils::exists(fname)) {
            fn = fname;
            goto cool;
        }
    }
#ifdef QMAKE_BUILTIN_PRFS
    fn.prepend(QLatin1String(":/qmake/features/"));
    if (QFileInfo(fn).exists())
        goto cool;
#endif
    return false;

  cool:
    ProStringList &already = valuesRef(ProString("QMAKE_INTERNAL_INCLUDED_FEATURES"));
    ProString afn(fn, NoHash);
    if (already.contains(afn))
        return true;
    already.append(afn);

#ifdef PROEVALUATOR_CUMULATIVE
    bool cumulative = m_cumulative;
    m_cumulative = false;
#endif

    // The path is fully normalized already.
    bool ok = evaluateFileDirect(fn, QMakeHandler::EvalFeatureFile, LoadProOnly);

#ifdef PROEVALUATOR_CUMULATIVE
    m_cumulative = cumulative;
#endif
    return ok;
}

bool QMakeEvaluator::evaluateFileInto(const QString &fileName, QMakeHandler::EvalFileType type,
        ProValueMap *values, LoadFlags flags)
{
    QMakeEvaluator visitor(m_option, m_parser, m_handler);
    visitor.m_outputDir = m_outputDir;
    if (!visitor.evaluateFile(fileName, type, flags))
        return false;
    *values = visitor.m_valuemapStack.top();
    return true;
}

void QMakeEvaluator::evalError(const QString &message) const
{
    if (!m_skipLevel)
        m_handler->evalError(m_current.line ? m_current.pro->fileName() : QString(),
                             m_current.line, message);
}

QT_END_NAMESPACE
