/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "cdbstackframecontext.h"
#include "cdbengine_p.h"
#include "cdbsymbolgroupcontext.h"
#include "cdbdumperhelper.h"
#include "debuggeractions.h"
#include "watchhandler.h"
#include "coreengine.h"

#include <utils/savedaction.h>

#include <QtCore/QDebug>
#include <QtCore/QCoreApplication>

namespace Debugger {
namespace Internal {

enum { OwnerNewItem, OwnerSymbolGroup, OwnerSymbolGroupDumper , OwnerDumper };

typedef QSharedPointer<CdbDumperHelper> SharedPointerCdbDumperHelper;
typedef QList<WatchData> WatchDataList;

// Predicates for parametrizing the symbol group
inline bool truePredicate(const WatchData & /* whatever */) { return true; }
inline bool falsePredicate(const WatchData & /* whatever */) { return false; }
inline bool isDumperPredicate(const WatchData &wd)
{ return (wd.source & CdbStackFrameContext::SourceMask) == OwnerDumper; }

static inline void debugWatchDataList(const QList<WatchData> &l, const char *why = 0)
{
    QDebug nospace = qDebug().nospace();
    if (why)
        nospace << why << '\n';
    foreach(const WatchData &wd, l)
        nospace << wd.toString() << '\n';
    nospace << '\n';
}

// Match an item that is expanded in the watchhandler.
class WatchHandlerExpandedPredicate {
public:
    explicit inline WatchHandlerExpandedPredicate(const WatchHandler *wh) : m_wh(wh) {}
    inline bool operator()(const WatchData &wd) { return m_wh->isExpandedIName(wd.iname); }
private:
    const WatchHandler *m_wh;
};

// Match an item by iname
class MatchINamePredicate {
public:
    explicit inline MatchINamePredicate(const QString &iname) : m_iname(iname) {}
    inline bool operator()(const WatchData &wd) { return wd.iname == m_iname; }
private:
    const QString &m_iname;
};

// Put a sequence of  WatchData into the model for the non-dumper case
class WatchHandlerModelInserter {
public:
    explicit WatchHandlerModelInserter(WatchHandler *wh) : m_wh(wh) {}

    inline WatchHandlerModelInserter & operator*() { return *this; }
    inline WatchHandlerModelInserter &operator=(const WatchData &wd)  {
        m_wh->insertData(wd);
        return *this;
    }
    inline WatchHandlerModelInserter &operator++() { return *this; }

private:
    WatchHandler *m_wh;
};

// Put a sequence of  WatchData into the model for the dumper case.
// Sorts apart a sequence of  WatchData using the Dumpers.
// Puts the stuff for which there is no dumper in the model
// as is and sets ownership to symbol group. The rest goes
// to the dumpers. Additionally, checks for items pointing to a
// dumpeable type and inserts a fake dereferenced item and value.
class WatchHandleDumperInserter {
public:
    explicit WatchHandleDumperInserter(WatchHandler *wh,
                                        const SharedPointerCdbDumperHelper &dumper);

    inline WatchHandleDumperInserter & operator*() { return *this; }
    inline WatchHandleDumperInserter &operator=(WatchData &wd);
    inline WatchHandleDumperInserter &operator++() { return *this; }

private:
    bool expandPointerToDumpable(const WatchData &wd, QString *errorMessage);

    const QRegExp m_hexNullPattern;
    WatchHandler *m_wh;
    const SharedPointerCdbDumperHelper m_dumper;
    QList<WatchData> m_dumperResult;
};

WatchHandleDumperInserter::WatchHandleDumperInserter(WatchHandler *wh,
                                                       const SharedPointerCdbDumperHelper &dumper) :
    m_hexNullPattern(QLatin1String("0x0+")),
    m_wh(wh),
    m_dumper(dumper)
{
    Q_ASSERT(m_hexNullPattern.isValid());
}

// Prevent recursion of the model by setting value and type
static inline bool fixDumperType(WatchData *wd, const WatchData *source = 0)
{
    const bool missing = wd->isTypeNeeded() || wd->type.isEmpty();
    if (missing) {
        static const QString unknownType = QCoreApplication::translate("CdbStackFrameContext", "<Unknown Type>");
        wd->setType(source ? source->type : unknownType, false);
    }
    return missing;
}

static inline bool fixDumperValue(WatchData *wd, const WatchData *source = 0)
{
    const bool missing = wd->isValueNeeded();
    if (missing) {
        if (source && source->isValueKnown()) {
            wd->setValue(source->value);
        } else {
            static const QString unknownValue = QCoreApplication::translate("CdbStackFrameContext", "<Unknown Value>");
            wd->setValue(unknownValue);
        }
    }
    return missing;
}

// When querying an item, the queried item is sometimes returned in incomplete form.
// Take over values from source.
static inline void fixDumperResult(const WatchData &source,
                                   QList<WatchData> *result,
                                   bool suppressGrandChildren)
{

    const int size = result->size();
    if (!size)
        return;
    if (debugCDBWatchHandling)
        debugWatchDataList(*result, suppressGrandChildren ? ">fixDumperResult suppressGrandChildren" : ">fixDumperResult");
    WatchData &returned = result->front();
    if (returned.iname != source.iname)
        return;
    fixDumperType(&returned, &source);
    fixDumperValue(&returned, &source);
    // Indicate owner and known children
    returned.source = OwnerDumper;
    if (returned.isChildrenKnown() && returned.isHasChildrenKnown() && returned.hasChildren)
        returned.source |= CdbStackFrameContext::ChildrenKnownBit;
    if (size == 1)
        return;
    // If the model queries the expanding item by pretending childrenNeeded=1,
    // refuse the request as the children are already known
    returned.source |= CdbStackFrameContext::ChildrenKnownBit;
    // Fix the children: If the address is missing, we cannot query any further.
    const QList<WatchData>::iterator wend = result->end();
    QList<WatchData>::iterator it = result->begin();
    for (++it; it != wend; ++it) {
        WatchData &wd = *it;
        // Indicate owner and known children
        it->source = OwnerDumper;
        if (it->isChildrenKnown() && it->isHasChildrenKnown() && it->hasChildren)
            it->source |= CdbStackFrameContext::ChildrenKnownBit;
        // Cannot dump items with missing addresses or missing types
        const bool typeFixed = fixDumperType(&wd); // Order of evaluation!
        if ((wd.addr.isEmpty() && wd.isSomethingNeeded()) || typeFixed) {
            wd.setHasChildren(false);
            wd.setAllUnneeded();
        } else {
            // Hack: Suppress endless recursion of the model. To be fixed,
            // the model should not query non-visible items.
            if (suppressGrandChildren && (wd.isChildrenNeeded() || wd.isHasChildrenNeeded()))
                wd.setHasChildren(false);
        }
    }
    if (debugCDBWatchHandling)
        debugWatchDataList(*result, "<fixDumperResult");
}

// Is this a non-null pointer to a dumpeable item with a value
// "0x4343 class QString *" ? - Insert a fake '*' dereferenced item
// and run dumpers on it. If that succeeds, insert the fake items owned by dumpers,
// which will trigger the ignore predicate.
// Note that the symbol context does not create '*' dereferenced items for
// classes (see note in its header documentation).
bool WatchHandleDumperInserter::expandPointerToDumpable(const WatchData &wd, QString *errorMessage)
{
    if (debugCDBWatchHandling)
        qDebug() << ">expandPointerToDumpable" << wd.toString();

    bool handled = false;
    do {
        if (wd.error || !isPointerType(wd.type))
            break;
        const int classPos = wd.value.indexOf(" class ");
        if (classPos == -1)
            break;
        const QString hexAddrS = wd.value.mid(0, classPos);
        if (m_hexNullPattern.exactMatch(hexAddrS))
            break;
        const QString type = stripPointerType(wd.value.mid(classPos + 7));
        WatchData derefedWd;
        derefedWd.setType(type);
        derefedWd.setAddress(hexAddrS);
        derefedWd.name = QString(QLatin1Char('*'));
        derefedWd.iname = wd.iname + ".*";
        derefedWd.source = OwnerDumper | CdbStackFrameContext::ChildrenKnownBit;
        const CdbDumperHelper::DumpResult dr = m_dumper->dumpType(derefedWd, true, &m_dumperResult, errorMessage);
        if (dr != CdbDumperHelper::DumpOk)
            break;
        fixDumperResult(derefedWd, &m_dumperResult, false);
        // Insert the pointer item with 1 additional child + its dumper results
        // Note: formal arguments might already be expanded in the symbol group.
        WatchData ptrWd = wd;
        ptrWd.source = OwnerDumper | CdbStackFrameContext::ChildrenKnownBit;
        ptrWd.setHasChildren(true);
        ptrWd.setChildrenUnneeded();
        m_wh->insertData(ptrWd);
        m_wh->insertBulkData(m_dumperResult);
        handled = true;
    } while (false);
    if (debugCDBWatchHandling)
        qDebug() << "<expandPointerToDumpable returns " << handled << *errorMessage;
    return handled;
}

WatchHandleDumperInserter &WatchHandleDumperInserter::operator=(WatchData &wd)
{
    if (debugCDBWatchHandling)
        qDebug() << "WatchHandleDumperInserter::operator=" << wd.toString();
    // Check pointer to dumpeable, dumpeable, insert accordingly.
    QString errorMessage;
    if (expandPointerToDumpable(wd, &errorMessage)) {
        // Nasty side effect: Modify owner for the ignore predicate
        wd.source = OwnerDumper;
        return *this;
    }
    // Expanded by internal dumper? : ok
    if ((wd.source & CdbStackFrameContext::SourceMask) == OwnerSymbolGroupDumper) {
        m_wh->insertData(wd);
        return *this;
    }
    // Try library dumpers.
    switch (m_dumper->dumpType(wd, true, &m_dumperResult, &errorMessage)) {
    case CdbDumperHelper::DumpOk:
        if (debugCDBWatchHandling)
            qDebug() << "dumper triggered";
        // Dumpers omit types for complicated templates
        fixDumperResult(wd, &m_dumperResult, false);
        // Discard the original item and insert the dumper results
        m_wh->insertBulkData(m_dumperResult);
        // Nasty side effect: Modify owner for the ignore predicate
        wd.source = OwnerDumper;
        break;
    case CdbDumperHelper::DumpNotHandled:
    case CdbDumperHelper::DumpError:
        wd.source = OwnerSymbolGroup;
        m_wh->insertData(wd);
        break;
    }
    return *this;
}

// -----------CdbStackFrameContext
CdbStackFrameContext::CdbStackFrameContext(const QSharedPointer<CdbDumperHelper> &dumper,
                                           CdbSymbolGroupContext *symbolContext) :
        m_useDumpers(dumper->isEnabled() && debuggerCore()->boolSetting(UseDebuggingHelpers)),
        m_dumper(dumper),
        m_symbolContext(symbolContext)
{
}

bool CdbStackFrameContext::assignValue(const QString &iname, const QString &value,
                                       QString *newValue /* = 0 */, QString *errorMessage)
{
    return m_symbolContext->assignValue(iname, value, newValue, errorMessage);
}

bool CdbStackFrameContext::populateModelInitially(WatchHandler *wh, QString *errorMessage)
{
    if (debugCDBWatchHandling)
        qDebug() << "populateModelInitially dumpers=" << m_useDumpers;
    // Recurse down items that are initially expanded in the view, stop processing for
    // dumper items.
    const CdbSymbolGroupRecursionContext rctx(m_symbolContext, OwnerSymbolGroupDumper);
    const bool rc = m_useDumpers ?
        CdbSymbolGroupContext::populateModelInitiallyHelper(rctx,
                                                      WatchHandleDumperInserter(wh, m_dumper),
                                                      WatchHandlerExpandedPredicate(wh),
                                                      isDumperPredicate,
                                                      errorMessage) :
        CdbSymbolGroupContext::populateModelInitiallyHelper(rctx,
                                                      WatchHandlerModelInserter(wh),
                                                      WatchHandlerExpandedPredicate(wh),
                                                      falsePredicate,
                                                      errorMessage);
    return rc;
}

bool CdbStackFrameContext::completeData(const WatchData &incompleteLocal,
                                        WatchHandler *wh,
                                        QString *errorMessage)
{
    if (debugCDBWatchHandling)
        qDebug() << ">completeData src=" << incompleteLocal.source << incompleteLocal.toString();

    const CdbSymbolGroupRecursionContext rctx(m_symbolContext, OwnerSymbolGroupDumper);
    // Expand symbol group items, recurse one level from desired item
    if (!m_useDumpers) {
        return CdbSymbolGroupContext::completeDataHelper(rctx, incompleteLocal,
                                                   WatchHandlerModelInserter(wh),
                                                   MatchINamePredicate(incompleteLocal.iname),
                                                   falsePredicate,
                                                   errorMessage);
    }

    // Expand artifical dumper items
    if ((incompleteLocal.source & CdbStackFrameContext::SourceMask) == OwnerDumper) {
        // If the model queries the expanding item by pretending childrenNeeded=1,
        // refuse the request if the children are already known
        if (incompleteLocal.state == WatchData::ChildrenNeeded && (incompleteLocal.source & CdbStackFrameContext::ChildrenKnownBit)) {
            WatchData local = incompleteLocal;
            local.setChildrenUnneeded();
            wh->insertData(local);
            return true;
        }
        QList<WatchData> dumperResult;
        const CdbDumperHelper::DumpResult dr = m_dumper->dumpType(incompleteLocal, true, &dumperResult, errorMessage);
        if (dr == CdbDumperHelper::DumpOk) {
            // Hack to stop endless model recursion
            const bool suppressGrandChildren = !wh->isExpandedIName(incompleteLocal.iname);
            fixDumperResult(incompleteLocal, &dumperResult, suppressGrandChildren);
            wh->insertBulkData(dumperResult);
        } else {
            const QString msg = QString::fromLatin1("Unable to further expand dumper watch data: '%1' (%2): %3/%4").arg(incompleteLocal.name, incompleteLocal.type).arg(int(dr)).arg(*errorMessage);
            qWarning("%s", qPrintable(msg));
            WatchData wd = incompleteLocal;
            if (wd.isValueNeeded())
                wd.setValue(QCoreApplication::translate("CdbStackFrameContext", "<Unknown>"));
            wd.setHasChildren(false);
            wd.setAllUnneeded();
            wh->insertData(wd);
        }
        return true;
    }

    // Expand symbol group items, recurse one level from desired item
    return CdbSymbolGroupContext::completeDataHelper(rctx, incompleteLocal,
                                               WatchHandleDumperInserter(wh, m_dumper),
                                               MatchINamePredicate(incompleteLocal.iname),
                                               isDumperPredicate,
                                               errorMessage);
}

CdbStackFrameContext::~CdbStackFrameContext()
{
    delete m_symbolContext;
}

bool CdbStackFrameContext::editorToolTip(const QString &iname,
                                         QString *value,
                                         QString *errorMessage)
{
    value->clear();
    unsigned long index;
    if (!m_symbolContext->lookupPrefix(iname, &index)) {
        *errorMessage = QString::fromLatin1("%1 not found.").arg(iname);
        return false;
    }
    // Check dumpers. Should actually be just one item.

    WatchData wd;
    const unsigned rc = m_symbolContext->watchDataAt(index, &wd);
    if (m_useDumpers && !wd.error
        && (0u == (rc & CdbCore::SymbolGroupContext::InternalDumperMask))
        && m_dumper->state() != CdbDumperHelper::Disabled) {
        QList<WatchData> result;
        if (CdbDumperHelper::DumpOk == m_dumper->dumpType(wd, false, &result, errorMessage))  {
            foreach (const WatchData &dwd, result) {
                if (!value->isEmpty())
                    value->append(QLatin1Char('\n'));
                value->append(dwd.toToolTip());
            }
            return true;
        } // Dumped ok
    }     // has Dumpers
    *value = wd.toToolTip();
    return true;
}

} // namespace Internal
} // namespace Debugger
