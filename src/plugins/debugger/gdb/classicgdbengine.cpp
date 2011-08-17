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

#include "gdbengine.h"
#include "gdbmi.h"

#include "debuggerstartparameters.h"
#include "abstractgdbadapter.h"
#include "debuggeractions.h"
#include "debuggercore.h"
#include "debuggerstringutils.h"

#include "stackhandler.h"
#include "watchhandler.h"

#include <utils/qtcassert.h>
#include <utils/savedaction.h>
#include <utils/fileutils.h>

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtGui/QMessageBox>

#if !defined(Q_OS_WIN)
#include <dlfcn.h>
#endif


#define PRECONDITION QTC_CHECK(!hasPython())

#define CB(callback) &GdbEngine::callback, STRINGIFY(callback)

enum {
    debugPending = 0,
    debugSubItem = 0,
    debug = 0
};

namespace Debugger {
namespace Internal {

// ----------------- QtDumperHelper::TypeData
DumperHelper::TypeData::TypeData() :
    type(UnknownType),
    isTemplate(false)
{
}

void DumperHelper::TypeData::clear()
{
    isTemplate = false;
    type = UnknownType;
    tmplate.clear();
    inner.clear();
}

// ----------------- QtDumperHelper
DumperHelper::DumperHelper() :
    m_qtVersion(0),
    m_dumperVersion(1.0)
{
    qFill(m_specialSizes, m_specialSizes + SpecialSizeCount, 0);
    setQClassPrefixes(QByteArray());
}

void DumperHelper::clear()
{
    m_nameTypeMap.clear();
    m_qtVersion = 0;
    m_dumperVersion = 1.0;
    m_qtNamespace.clear();
    m_sizeCache.clear();
    qFill(m_specialSizes, m_specialSizes + SpecialSizeCount, 0);
    m_expressionCache.clear();
    setQClassPrefixes(QByteArray());
}

QString DumperHelper::msgDumperOutdated(double requiredVersion, double currentVersion)
{
    return QCoreApplication::translate("QtDumperHelper",
       "Found an outdated version of the debugging helper library (%1); "
       "version %2 is required.").
       arg(currentVersion).arg(requiredVersion);
}

static inline void formatQtVersion(int v, QTextStream &str)
{
    str  << ((v >> 16) & 0xFF) << '.' << ((v >> 8) & 0xFF) << '.' << (v & 0xFF);
}

QString DumperHelper::toString(bool debug) const
{
    if (debug)  {
        QString rc;
        QTextStream str(&rc);
        str << "version=";
        formatQtVersion(m_qtVersion, str);
        str << "dumperversion='" << m_dumperVersion <<  "' namespace='" << m_qtNamespace << "'," << m_nameTypeMap.size() << " known types <type enum>: ";
        const NameTypeMap::const_iterator cend = m_nameTypeMap.constEnd();
        for (NameTypeMap::const_iterator it = m_nameTypeMap.constBegin(); it != cend; ++it) {
            str <<",[" << it.key() << ',' << it.value() << ']';
        }
        str << "\nSpecial size: ";
        for (int i = 0; i < SpecialSizeCount; i++)
            str << ' ' << m_specialSizes[i];
        str << "\nSize cache: ";
        const SizeCache::const_iterator scend = m_sizeCache.constEnd();
        for (SizeCache::const_iterator it = m_sizeCache.constBegin(); it != scend; ++it) {
            str << ' ' << it.key() << '=' << it.value() << '\n';
        }
        str << "\nExpression cache: (" << m_expressionCache.size() << ")\n";
        const ExpressionCache::const_iterator excend = m_expressionCache.constEnd();
        for (ExpressionCache::const_iterator it = m_expressionCache.constBegin(); it != excend; ++it)
            str << "    " << it.key() << ' ' << it.value() << '\n';
        return rc;
    }
    const QString nameSpace = m_qtNamespace.isEmpty()
        ? QCoreApplication::translate("QtDumperHelper", "<none>") : m_qtNamespace;
    return QCoreApplication::translate("QtDumperHelper",
       "%n known types, Qt version: %1, Qt namespace: %2 Dumper version: %3",
       0, QCoreApplication::CodecForTr,
       m_nameTypeMap.size()).arg(qtVersionString(), nameSpace).arg(m_dumperVersion);
}

DumperHelper::Type DumperHelper::simpleType(const QByteArray &simpleType) const
{
    return m_nameTypeMap.value(simpleType, UnknownType);
}

int DumperHelper::qtVersion() const
{
    return m_qtVersion;
}

QByteArray DumperHelper::qtNamespace() const
{
    return m_qtNamespace;
}

int DumperHelper::typeCount() const
{
    return m_nameTypeMap.size();
}

// Look up unnamespaced 'std' types.
static DumperHelper::Type stdType(const QByteArray &type)
{
    if (type == "vector")
        return DumperHelper::StdVectorType;
    if (type == "deque")
        return DumperHelper::StdDequeType;
    if (type == "set")
        return DumperHelper::StdSetType;
    if (type == "stack")
        return DumperHelper::StdStackType;
    if (type == "map")
        return DumperHelper::StdMapType;
    if (type == "basic_string")
        return DumperHelper::StdStringType;
    return DumperHelper::UnknownType;
}

static DumperHelper::Type specialType(QByteArray type)
{
    // Std classes.
    if (type.startsWith("std::"))
        return stdType(type.mid(5));

    // Strip namespace
    // FIXME: that's not a good idea as it makes all namespaces equal.
    const int namespaceIndex = type.lastIndexOf("::");
    if (namespaceIndex == -1) {
        // None ... check for std..
        const DumperHelper::Type sType = stdType(type);
        if (sType != DumperHelper::UnknownType)
            return sType;
    } else {
        type = type.mid(namespaceIndex + 2);
    }

    if (type == "QAbstractItem")
        return DumperHelper::QAbstractItemType;
    if (type == "QMap")
        return DumperHelper::QMapType;
    if (type == "QMapNode")
        return DumperHelper::QMapNodeType;
    if (type == "QMultiMap")
        return DumperHelper::QMultiMapType;
    if (type == "QObject")
        return DumperHelper::QObjectType;
    if (type == "QObjectSignal")
        return DumperHelper::QObjectSignalType;
    if (type == "QObjectSlot")
        return DumperHelper::QObjectSlotType;
    if (type == "QStack")
        return DumperHelper::QStackType;
    if (type == "QVector")
        return DumperHelper::QVectorType;
    if (type == "QWidget")
        return DumperHelper::QWidgetType;
    return DumperHelper::UnknownType;
}

QByteArray DumperHelper::qtVersionString() const
{
    QString rc;
    QTextStream str(&rc);
    formatQtVersion(m_qtVersion, str);
    return rc.toLatin1();
}

// Parse a list of types.
typedef QList<QByteArray> QByteArrayList;

static QByteArray qClassName(const QByteArray &qtNamespace, const char *className)
{
    if (qtNamespace.isEmpty())
        return className;
    QByteArray rc = qtNamespace;
    rc += "::";
    rc += className;
    return rc;
}

static double getDumperVersion(const GdbMi &contents)
{
    const GdbMi dumperVersionG = contents.findChild("dumperversion");
    if (dumperVersionG.type() != GdbMi::Invalid) {
        bool ok;
        const double v = QString::fromAscii(dumperVersionG.data()).toDouble(&ok);
        if (ok)
            return v;
    }
    return 1.0;
}


void DumperHelper::setQClassPrefixes(const QByteArray &qNamespace)
{
    // Prefixes with namespaces
    m_qPointerPrefix = qClassName(qNamespace, "QPointer");
    m_qSharedPointerPrefix = qClassName(qNamespace, "QSharedPointer");
    m_qSharedDataPointerPrefix = qClassName(qNamespace, "QSharedDataPointer");
    m_qWeakPointerPrefix = qClassName(qNamespace, "QWeakPointer");
    m_qListPrefix = qClassName(qNamespace, "QList");
    m_qLinkedListPrefix = qClassName(qNamespace, "QLinkedList");
    m_qVectorPrefix = qClassName(qNamespace, "QVector");
    m_qQueuePrefix = qClassName(qNamespace, "QQueue");
}

bool DumperHelper::parseQuery(const GdbMi &contents)
{
    clear();
    if (debug > 1)
        qDebug() << "parseQuery" << contents.toString(true, 2);

    // Common info, dumper version, etc
    QByteArray ns = contents.findChild("namespace").data();
    setQtNamespace(ns);
    int qtv = 0;
    const GdbMi qtversion = contents.findChild("qtversion");
    if (qtversion.children().size() == 3) {
        qtv = (qtversion.childAt(0).data().toInt() << 16)
                    + (qtversion.childAt(1).data().toInt() << 8)
                    + qtversion.childAt(2).data().toInt();
    }
    m_qtVersion = qtv;
    // Get list of helpers
    QByteArrayList availableSimpleDebuggingHelpers;
    foreach (const GdbMi &item, contents.findChild("dumpers").children())
        availableSimpleDebuggingHelpers.append(item.data());

    // Parse types
    m_nameTypeMap.clear();
    foreach (const QByteArray &type, availableSimpleDebuggingHelpers) {
        const Type t = specialType(type);
        m_nameTypeMap.insert(type, t != UnknownType ? t : SupportedType);
    }

    m_dumperVersion = getDumperVersion(contents);
    // Parse sizes
    foreach (const GdbMi &sizesList, contents.findChild("sizes").children()) {
        const int childCount = sizesList.childCount();
        if (childCount > 1) {
            const int size = sizesList.childAt(0).data().toInt();
            for (int c = 1; c < childCount; c++)
                addSize(sizesList.childAt(c).data(), size);
        }
    }
    // Parse expressions
    foreach (const GdbMi &exprList, contents.findChild("expressions").children())
        if (exprList.childCount() == 2)
            m_expressionCache.insert(exprList.childAt(0).data(),
                                     exprList.childAt(1).data());
    return true;
}

void DumperHelper::addSize(const QByteArray &name, int size)
{
    // Special interest cases
    if (name == "char*") {
        m_specialSizes[PointerSize] = size;
        return;
    }
    const SpecialSizeType st = specialSizeType(name);
    if (st != SpecialSizeCount) {
        m_specialSizes[st] = size;
        return;
    }
    do {
        // CDB helpers
        if (name == "std::string") {
            m_sizeCache.insert("std::basic_string<char,std::char_traits<char>,std::allocator<char> >", size);
            m_sizeCache.insert("basic_string<char,char_traits<char>,allocator<char> >", size);
            break;
        }
        if (name == "std::wstring") {
            m_sizeCache.insert("basic_string<unsigned short,char_traits<unsignedshort>,allocator<unsignedshort> >", size);
            m_sizeCache.insert("std::basic_string<unsigned short,std::char_traits<unsigned short>,std::allocator<unsigned short> >", size);
            break;
        }
    } while (false);
    m_sizeCache.insert(name, size);
}

DumperHelper::Type DumperHelper::type(const QByteArray &typeName) const
{
    const DumperHelper::TypeData td = typeData(typeName);
    return td.type;
}

static bool extractTemplate(const QByteArray &type, QByteArray *tmplate, QByteArray *inner)
{
    // Input "Template<Inner1,Inner2,...>::Foo" will return "Template::Foo" in
    // 'tmplate' and "Inner1@Inner2@..." etc in 'inner'. Result indicates
    // whether parsing was successful
    // Gdb inserts a blank after each comma which we would like to avoid
    tmplate->clear();
    inner->clear();
    if (!type.contains('<'))
        return  false;
    int level = 0;
    bool skipSpace = false;
    const int size = type.size();

    for (int i = 0; i != size; ++i) {
        const char c = type.at(i);
        switch (c) {
        case '<':
            *(level == 0 ? tmplate : inner) += c;
            ++level;
            break;
        case '>':
            --level;
            *(level == 0 ? tmplate : inner) += c;
            break;
        case ',':
            *inner += (level == 1) ? '@' : ',';
            skipSpace = true;
            break;
        default:
            if (!skipSpace || c != ' ') {
                *(level == 0 ? tmplate : inner) += c;
                skipSpace = false;
            }
            break;
        }
    }
    *tmplate = tmplate->trimmed();
    tmplate->replace("<>", "");
    *inner = inner->trimmed();
    // qDebug() << "EXTRACT TEMPLATE: " << *tmplate << *inner << " FROM " << type;
    return !inner->isEmpty();
}

DumperHelper::TypeData DumperHelper::typeData(const QByteArray &typeName) const
{
    TypeData td;
    td.type = UnknownType;
    const Type st = simpleType(typeName);
    if (st != UnknownType) {
        td.isTemplate = false;
        td.type = st;
        return td;
    }
    // Try template
    td.isTemplate = extractTemplate(typeName, &td.tmplate, &td.inner);
    if (!td.isTemplate)
        return td;
    // Check the template type QMap<X,Y> -> 'QMap'
    td.type = simpleType(td.tmplate);
    return td;
}

static QByteArray sizeofTypeExpression(const QByteArray &type)
{
    if (type.endsWith('*'))
        return "sizeof(void*)";
    if (type.endsWith('>'))
        return "sizeof(" + type + ')';
    return "sizeof(" + gdbQuoteTypes(type) + ')';
}

// Format an expression to have the debugger query the
// size. Use size cache if possible
QByteArray DumperHelper::evaluationSizeofTypeExpression(const QByteArray &typeName) const
{
    // Look up special size types
    const SpecialSizeType st = specialSizeType(typeName);
    if (st != SpecialSizeCount) {
        if (const int size = m_specialSizes[st])
            return QByteArray::number(size);
    }
    // Look up size cache
    const SizeCache::const_iterator sit = m_sizeCache.constFind(typeName);
    if (sit != m_sizeCache.constEnd())
        return QByteArray::number(sit.value());
    // Finally have the debugger evaluate
    return sizeofTypeExpression(typeName);
}

DumperHelper::SpecialSizeType DumperHelper::specialSizeType(const QByteArray &typeName) const
{
    if (isPointerType(typeName))
        return PointerSize;
    if (typeName == "int")
        return IntSize;
    if (typeName.startsWith("std::allocator"))
        return StdAllocatorSize;
    if (typeName.startsWith(m_qPointerPrefix))
        return QPointerSize;
    if (typeName.startsWith(m_qSharedPointerPrefix))
        return QSharedPointerSize;
    if (typeName.startsWith(m_qSharedDataPointerPrefix))
        return QSharedDataPointerSize;
    if (typeName.startsWith(m_qWeakPointerPrefix))
        return QWeakPointerSize;
    if (typeName.startsWith(m_qListPrefix))
        return QListSize;
    if (typeName.startsWith(m_qLinkedListPrefix))
        return QLinkedListSize;
    if (typeName.startsWith(m_qVectorPrefix))
        return QVectorSize;
    if (typeName.startsWith(m_qQueuePrefix))
        return QQueueSize;
    return SpecialSizeCount;
}

static inline bool isInteger(const QString &n)
{
    const int size = n.size();
    if (!size)
        return false;
    for (int i = 0; i < size; i++)
        if (!n.at(i).isDigit())
            return false;
    return true;
}

// Return debugger expression to get the offset of a map node.
static inline QByteArray qMapNodeValueOffsetExpression(const QByteArray &type)
{
        return "(size_t)&(('" + type + "'*)0)->value";
}

void DumperHelper::evaluationParameters(const WatchData &data,
    const TypeData &td, QByteArray *inBuffer, QByteArrayList *extraArgsIn) const
{
    enum { maxExtraArgCount = 4 };

    QByteArrayList &extraArgs = *extraArgsIn;

    // See extractTemplate for parameters
    QByteArrayList inners = td.inner.split('@');
    if (inners.at(0).isEmpty())
        inners.clear();
    for (int i = 0; i != inners.size(); ++i)
        inners[i] = inners[i].simplified();

    QString outertype = td.isTemplate ? td.tmplate : data.type;
    // adjust the data extract
    if (outertype == m_qtNamespace + "QWidget")
        outertype = m_qtNamespace + "QObject";

    QString inner = td.inner;
    const QByteArray zero = "0";

    extraArgs.clear();

    if (!inners.empty()) {
        // "generic" template dumpers: passing sizeof(argument)
        // gives already most information the dumpers need
        const int count = qMin(int(maxExtraArgCount), inners.size());
        for (int i = 0; i < count; i++)
            extraArgs.push_back(evaluationSizeofTypeExpression(inners.at(i)));
    }

    // Pad with zeros
    while (extraArgs.size() < maxExtraArgCount)
        extraArgs.push_back("0");

    // in rare cases we need more or less:
    switch (td.type) {
    case QAbstractItemType:
        if (data.dumperFlags.isEmpty()) {
            qWarning("Internal error: empty dumper state '%s'.", data.iname.constData());
        } else {
            inner = data.dumperFlags.mid(1);
        }
        break;
    case QObjectSlotType:
    case QObjectSignalType: {
            // we need the number out of something like
            // iname="local.ob.slots.2" // ".deleteLater()"?
            const int pos = data.iname.lastIndexOf('.');
            const QByteArray slotNumber = data.iname.mid(pos + 1);
            QTC_CHECK(slotNumber.toInt() != -1);
            extraArgs[0] = slotNumber;
        }
        break;
    case QMapType:
    case QMultiMapType: {
            QByteArray nodetype;
            if (m_qtVersion >= 0x040500) {
                nodetype = m_qtNamespace + "QMapNode";
                nodetype += data.type.mid(outertype.size());
            } else {
                // FIXME: doesn't work for QMultiMap
                nodetype  = data.type + "::Node";
            }
            //qDebug() << "OUTERTYPE: " << outertype << " NODETYPE: " << nodetype
            //    << "QT VERSION" << m_qtVersion << ((4 << 16) + (5 << 8) + 0);
            extraArgs[2] = evaluationSizeofTypeExpression(nodetype);
            extraArgs[3] = qMapNodeValueOffsetExpression(nodetype);
        }
        break;
    case QMapNodeType:
        extraArgs[2] = evaluationSizeofTypeExpression(data.type);
        extraArgs[3] = qMapNodeValueOffsetExpression(data.type);
        break;
    case StdVectorType:
        //qDebug() << "EXTRACT TEMPLATE: " << outertype << inners;
        if (inners.at(0) == "bool")
            outertype = "std::vector::bool";
        break;
    case StdDequeType:
        extraArgs[1] = "0";
        break;
    case StdStackType:
        // remove 'std::allocator<...>':
        extraArgs[1] = "0";
        break;
    case StdSetType:
        // remove 'std::less<...>':
        extraArgs[1] = "0";
        // remove 'std::allocator<...>':
        extraArgs[2] = "0";
        break;
    case StdMapType: {
            // We need the offset of the second item in the value pair.
            // We read the type of the pair from the allocator argument because
            // that gets the constness "right" (in the sense that gdb/cdb can
            // read it back: "std::allocator<std::pair<Key,Value> >"
            // -> "std::pair<Key,Value>". Different debuggers have varying
            // amounts of terminating blanks...
            extraArgs[2].clear();
            extraArgs[3] = "0";
            QByteArray pairType = inners.at(3);
            int bracketPos = pairType.indexOf('<');
            if (bracketPos != -1)
                pairType.remove(0, bracketPos + 1);
            // We don't want the comparator and the allocator confuse gdb.
            const char closingBracket = '>';
            bracketPos = pairType.lastIndexOf(closingBracket);
            if (bracketPos != -1)
                bracketPos = pairType.lastIndexOf(closingBracket, bracketPos - pairType.size() - 1);
            if (bracketPos != -1)
                pairType.truncate(bracketPos + 1);
            extraArgs[2] = "(size_t)&(('";
            extraArgs[2] += pairType;
            extraArgs[2] += "'*)0)->second";
    }
        break;
    case StdStringType:
        //qDebug() << "EXTRACT TEMPLATE: " << outertype << inners;
        if (inners.at(0) == "char")
            outertype = "std::string";
        else if (inners.at(0) == "wchar_t")
            outertype = "std::wstring";
        qFill(extraArgs, zero);
        break;
    case UnknownType:
        qWarning("Unknown type encountered in %s.\n", Q_FUNC_INFO);
        break;
    case SupportedType:
    case QVectorType:
    case QStackType:
    case QObjectType:
    case QWidgetType:
        break;
    }

    // Look up expressions in the cache
    if (!m_expressionCache.empty()) {
        const ExpressionCache::const_iterator excCend = m_expressionCache.constEnd();
        const QByteArrayList::iterator eend = extraArgs.end();
        for (QByteArrayList::iterator it = extraArgs.begin(); it != eend; ++it) {
            QByteArray &e = *it;
            if (!e.isEmpty() && e != zero && !isInteger(e)) {
                const ExpressionCache::const_iterator eit = m_expressionCache.constFind(e);
                if (eit != excCend)
                    e = eit.value();
            }
        }
    }

    inBuffer->clear();
    inBuffer->append(outertype.toUtf8());
    inBuffer->append('\0');
    inBuffer->append(data.iname);
    inBuffer->append('\0');
    inBuffer->append(data.exp);
    inBuffer->append('\0');
    inBuffer->append(inner.toUtf8());
    inBuffer->append('\0');
    inBuffer->append(data.iname);
    inBuffer->append('\0');

    if (debug)
        qDebug() << '\n' << Q_FUNC_INFO << '\n' << data.toString() << "\n-->" << outertype << td.type << extraArgs;
}

QDebug operator<<(QDebug in, const DumperHelper::TypeData &d)
{
    QDebug nsp = in.nospace();
    nsp << " type=" << d.type << " tpl=" << d.isTemplate;
    if (d.isTemplate)
        nsp << d.tmplate << '<' << d.inner << '>';
    return in;
}

static bool isAccessSpecifier(const QByteArray &ba)
{
    return ba == "private" || ba == "protected" || ba == "public";
}

// reads a MI-encoded item frome the consolestream
static bool parseConsoleStream(const GdbResponse &response, GdbMi *contents)
{
    QByteArray out = response.consoleStreamOutput;

    int markerPos = out.indexOf('"') + 1; // position of 'success marker'
    if (markerPos == 0 || out.at(markerPos) == 'f') {  // 't' or 'f'
        // custom dumper produced no output
        return false;
    }

    out = out.mid(markerPos +  1);
    out = out.left(out.lastIndexOf('"'));
    // optimization: dumper output never needs real C unquoting
    out.replace('\\', "");

    contents->fromStringMultiple(out);
    //qDebug() << "CONTENTS" << contents->toString(true);
    return contents->isValid();
}

void GdbEngine::updateLocalsClassic()
{
    PRECONDITION;
    m_pendingWatchRequests = 0;
    m_pendingBreakpointRequests = 0;
    m_processedNames.clear();

    if (0 && debugPending)
        qDebug() << "\nRESET PENDING";
    //m_toolTipCache.clear();
    clearToolTip();
    watchHandler()->beginCycle();

    QByteArray level = QByteArray::number(currentFrame());
    // '2' is 'list with type and value'
    QByteArray cmd = "-stack-list-arguments 2 " + level + ' ' + level;
    postCommand(cmd, WatchUpdate,
        CB(handleStackListArgumentsClassic));
    // '2' is 'list with type and value'
    postCommand("-stack-list-locals 2", WatchUpdate,
        CB(handleStackListLocalsClassic)); // stage 2/2
}

static inline QString msgRetrievingWatchData(int pending)
{
    return GdbEngine::tr("Retrieving data for watch view (%n requests pending)...", 0, pending);
}

void GdbEngine::runDirectDebuggingHelperClassic(const WatchData &data, bool dumpChildren)
{
    Q_UNUSED(dumpChildren)
    QByteArray type = data.type;
    QByteArray cmd;

    if (type == "QString" || type.endsWith("::QString"))
        cmd = "qdumpqstring (&(" + data.exp + "))";
    else if (type == "QStringList" || type.endsWith("::QStringList"))
        cmd = "qdumpqstringlist (&(" + data.exp + "))";

    QVariant var;
    var.setValue(data);
    postCommand(cmd, WatchUpdate, CB(handleDebuggingHelperValue3Classic), var);

    showStatusMessage(msgRetrievingWatchData(m_pendingWatchRequests + 1), 10000);
}

void GdbEngine::runDebuggingHelperClassic(const WatchData &data0, bool dumpChildren)
{
    PRECONDITION;
    if (m_debuggingHelperState != DebuggingHelperAvailable) {
        runDirectDebuggingHelperClassic(data0, dumpChildren);
        return;
    }
    WatchData data = data0;

    // Avoid endless loops created by faulty dumpers.
    QByteArray processedName = QByteArray::number(dumpChildren) + '-' + data.iname;
    if (m_processedNames.contains(processedName)) {
        showMessage(
            _("<Breaking endless loop for " + data.iname + '>'), LogMiscInput);
        data.setAllUnneeded();
        data.setValue(_("<unavailable>"));
        data.setHasChildren(false);
        insertData(data);
        return;
    }
    m_processedNames.insert(processedName);

    QByteArray params;
    QList<QByteArray> extraArgs;
    const DumperHelper::TypeData td = m_dumperHelper.typeData(data0.type);
    m_dumperHelper.evaluationParameters(data, td, &params, &extraArgs);

    //int protocol = (data.iname.startsWith("watch") && data.type == "QImage") ? 3 : 2;
    //int protocol = data.iname.startsWith("watch") ? 3 : 2;
    const int protocol = 2;
    //int protocol = isDisplayedIName(data.iname) ? 3 : 2;

    QByteArray addr;
    if (data.address)
        addr = "(void*)" + data.hexAddress();
    else if (data.exp.isEmpty()) // happens e.g. for QAbstractItem
        addr = QByteArray(1, '0');
    else
        addr = "&(" + data.exp + ')';

    sendWatchParameters(params);

    QByteArray cmd = "call (void*)qDumpObjectData440("
        + QByteArray::number(protocol)
        + ",0,"
        + addr
        + ','
        + (dumpChildren ? '1' : '0');
    foreach (const QByteArray &ex, extraArgs)
        cmd += ',' + ex;
    cmd += ')';

    postCommand(cmd, WatchUpdate | NonCriticalResponse);

    showStatusMessage(msgRetrievingWatchData(m_pendingWatchRequests + 1), 10000);

    // retrieve response
    postCommand("p (char*)&qDumpOutBuffer", WatchUpdate,
        CB(handleDebuggingHelperValue2Classic), qVariantFromValue(data));
}

void GdbEngine::createGdbVariableClassic(const WatchData &data)
{
    PRECONDITION;
    if (data.iname == "local.flist.0") {
        int i = 1;
        Q_UNUSED(i);
    }
    postCommand("-var-delete \"" + data.iname + '"', WatchUpdate);
    QByteArray exp = data.exp;
    if (exp.isEmpty() && data.address)
        exp = "*(" + gdbQuoteTypes(data.type) + "*)" + data.hexAddress();
    QVariant val = QVariant::fromValue<WatchData>(data);
    postCommand("-var-create \"" + data.iname + "\" * \"" + exp + '"',
        WatchUpdate, CB(handleVarCreate), val);
}

void GdbEngine::updateSubItemClassic(const WatchData &data0)
{
    PRECONDITION;
    WatchData data = data0;
    if (debugSubItem)
        qDebug() << "UPDATE SUBITEM:" << data.toString();
    QTC_ASSERT(data.isValid(), return);

    // in any case we need the type first
    if (data.isTypeNeeded()) {
        // This should only happen if we don't have a variable yet.
        // Let's play safe, though.
        if (!data.variable.isEmpty()) {
            // Update: It does so for out-of-scope watchers.
#            if 1
            qDebug() << "FIXME: GdbEngine::updateSubItem:"
                 << data.toString() << "should not happen";
#            else
            data.setType(WatchData::msgNotInScope());
            data.setValue(WatchData::msgNotInScope());
            data.setHasChildren(false);
            insertData(data);
            return;
#            endif
        }
        // The WatchVarCreate handler will receive type information
        // and re-insert a WatchData item with correct type, so
        // we will not re-enter this bit.
        // FIXME: Concurrency issues?
        createGdbVariableClassic(data);
        return;
    }

    // We should have a type now. This is relied upon further below.
    QTC_ASSERT(!data.type.isEmpty(), return);

    // A common case that can be easily solved.
    if (data.isChildrenNeeded() && isPointerType(data.type)
        && !hasDebuggingHelperForType(data.type)) {
        // We sometimes know what kind of children pointers have.
        if (debugSubItem)
            qDebug() << "IT'S A POINTER";

        if (debuggerCore()->boolSetting(AutoDerefPointers)) {
            // Try automatic dereferentiation
            data.exp = "(*(" + data.exp + "))";
            data.type = data.type + '.'; // FIXME: fragile HACK to avoid recursion
            if (data.value.startsWith("0x"))
                data.value = "@" + data.value;
            insertData(data);
        } else {
            data.setChildrenUnneeded();
            insertData(data);
            WatchData data1;
            data1.iname = data.iname + ".*";
            data1.name = QLatin1Char('*') + data.name;
            data1.exp = "(*(" + data.exp + "))";
            data1.type = stripPointerType(data.type);
            data1.setValueNeeded();
            data1.setChildrenUnneeded();
            insertData(data1);
        }
        return;
    }

    if (data.isValueNeeded() && hasDebuggingHelperForType(data.type)) {
        if (debugSubItem)
            qDebug() << "UPDATE SUBITEM: CUSTOMVALUE";
        runDebuggingHelperClassic(data,
            watchHandler()->isExpandedIName(data.iname));
        return;
    }

/*
    if (data.isValueNeeded() && data.exp.isEmpty()) {
        if (debugSubItem)
            qDebug() << "UPDATE SUBITEM: NO EXPRESSION?";
        data.setError("<no expression given>");
        insertData(data);
        return;
    }
*/

    if (data.isValueNeeded() && data.variable.isEmpty()) {
        if (debugSubItem)
            qDebug() << "UPDATE SUBITEM: VARIABLE NEEDED FOR VALUE";
        createGdbVariableClassic(data);
        // the WatchVarCreate handler will re-insert a WatchData
        // item, with valueNeeded() set.
        return;
    }

    if (data.isValueNeeded()) {
        QTC_ASSERT(!data.variable.isEmpty(), return); // tested above
        if (debugSubItem)
            qDebug() << "UPDATE SUBITEM: VALUE";
        QByteArray cmd = "-var-evaluate-expression \"" + data.iname + '"';
        postCommand(cmd, WatchUpdate,
            CB(handleEvaluateExpressionClassic), QVariant::fromValue(data));
        return;
    }

    if (data.isChildrenNeeded() && hasDebuggingHelperForType(data.type)) {
        if (debugSubItem)
            qDebug() << "UPDATE SUBITEM: CUSTOMVALUE WITH CHILDREN";
        runDebuggingHelperClassic(data, true);
        return;
    }

    if (data.isChildrenNeeded() && data.variable.isEmpty()) {
        if (debugSubItem)
            qDebug() << "UPDATE SUBITEM: VARIABLE NEEDED FOR CHILDREN";
        createGdbVariableClassic(data);
        // the WatchVarCreate handler will re-insert a WatchData
        // item, with childrenNeeded() set.
        return;
    }

    if (data.isChildrenNeeded()) {
        QTC_ASSERT(!data.variable.isEmpty(), return); // tested above
        QByteArray cmd = "-var-list-children --all-values \"" + data.variable + '"';
        postCommand(cmd, WatchUpdate,
            CB(handleVarListChildrenClassic), QVariant::fromValue(data));
        return;
    }

    if (data.isHasChildrenNeeded() && hasDebuggingHelperForType(data.type)) {
        if (debugSubItem)
            qDebug() << "UPDATE SUBITEM: CUSTOMVALUE WITH CHILDREN";
        runDebuggingHelperClassic(data, watchHandler()->isExpandedIName(data.iname));
        return;
    }

//#if !X
    if (data.isHasChildrenNeeded() && data.variable.isEmpty()) {
        if (debugSubItem)
            qDebug() << "UPDATE SUBITEM: VARIABLE NEEDED FOR CHILDCOUNT";
        createGdbVariableClassic(data);
        // the WatchVarCreate handler will re-insert a WatchData
        // item, with childrenNeeded() set.
        return;
    }
//#endif

    if (data.isHasChildrenNeeded()) {
        QTC_ASSERT(!data.variable.isEmpty(), return); // tested above
        QByteArray cmd = "-var-list-children --all-values \"" + data.variable + '"';
        postCommand(cmd, Discardable,
            CB(handleVarListChildrenClassic), QVariant::fromValue(data));
        return;
    }

    qDebug() << "FIXME: UPDATE SUBITEM:" << data.toString();
    QTC_ASSERT(false, return);
}

void GdbEngine::handleDebuggingHelperValue2Classic(const GdbResponse &response)
{
    PRECONDITION;
    WatchData data = response.cookie.value<WatchData>();
    QTC_ASSERT(data.isValid(), return);

    // The real dumper might have aborted without giving any answers.
    // Remove traces of the question, too.
    if (m_cookieForToken.contains(response.token - 1)) {
        m_cookieForToken.remove(response.token - 1);
        showMessage(_("DETECTING LOST COMMAND %1").arg(response.token - 1));
        --m_pendingWatchRequests;
        data.setError(WatchData::msgNotInScope());
        insertData(data);
        return;
    }

    //qDebug() << "CUSTOM VALUE RESULT:" << response.toString();
    //qDebug() << "FOR DATA:" << data.toString() << response.resultClass;
    if (response.resultClass != GdbResultDone) {
        qDebug() << "STRANGE CUSTOM DUMPER RESULT DATA:" << data.toString();
        return;
    }

    GdbMi contents;
    if (!parseConsoleStream(response, &contents)) {
        data.setError(WatchData::msgNotInScope());
        insertData(data);
        return;
    }

    setWatchDataType(data, response.data.findChild("type"));
    setWatchDataDisplayedType(data, response.data.findChild("displaytype"));
    QList<WatchData> list;
    parseWatchData(watchHandler()->expandedINames(), data, contents, &list);
    //for (int i = 0; i != list.size(); ++i)
    //    qDebug() << "READ: " << list.at(i).toString();
    watchHandler()->insertBulkData(list);
}

void GdbEngine::handleDebuggingHelperValue3Classic(const GdbResponse &response)
{
    if (response.resultClass == GdbResultDone) {
        WatchData data = response.cookie.value<WatchData>();
        QByteArray out = response.consoleStreamOutput;
        while (out.endsWith(' ') || out.endsWith('\n'))
            out.chop(1);
        QList<QByteArray> list = out.split(' ');
        if (list.isEmpty()) {
            data.setError(WatchData::msgNotInScope());
            data.setAllUnneeded();
            insertData(data);
        } else if (data.type == "QString"
                || data.type.endsWith("::QString")) {
            QList<QByteArray> list = out.split(' ');
            QString str;
            int l = out.isEmpty() ? 0 : list.size();
            for (int i = 0; i < l; ++i)
                 str.append(list.at(i).toInt());
            data.setValue(_c('"') + str + _c('"'));
            data.setHasChildren(false);
            data.setAllUnneeded();
            insertData(data);
        } else if (data.type == "QStringList"
                || data.type.endsWith("::QStringList")) {
            if (out.isEmpty()) {
                data.setValue(tr("<0 items>"));
                data.setHasChildren(false);
                data.setAllUnneeded();
                insertData(data);
            } else {
                int l = list.size();
                //: In string list
                data.setValue(tr("<%n items>", 0, l));
                data.setHasChildren(!list.empty());
                data.setAllUnneeded();
                insertData(data);
                for (int i = 0; i < l; ++i) {
                    WatchData data1;
                    data1.name = _("[%1]").arg(i);
                    data1.type = data.type.left(data.type.size() - 4);
                    data1.iname = data.iname + '.' + QByteArray::number(i);
                    const QByteArray &addressSpec = list.at(i);
                    if (addressSpec.startsWith("0x")) {
                        data.setHexAddress(addressSpec);
                    } else {
                        data.dumperFlags = addressSpec; // Item model dumpers pull tricks
                    }
                    data1.exp = "((" + gdbQuoteTypes(data1.type) + "*)" + addressSpec + ')';
                    data1.setHasChildren(false);
                    data1.setValueNeeded();
                    QByteArray cmd = "qdumpqstring (" + data1.exp + ')';
                    QVariant var;
                    var.setValue(data1);
                    postCommand(cmd, WatchUpdate,
                        CB(handleDebuggingHelperValue3Classic), var);
                }
            }
        } else {
            data.setError(WatchData::msgNotInScope());
            data.setAllUnneeded();
            insertData(data);
        }
    } else {
        WatchData data = response.cookie.value<WatchData>();
        data.setError(WatchData::msgNotInScope());
        data.setAllUnneeded();
        insertData(data);
    }
}

void GdbEngine::tryLoadDebuggingHelpersClassic()
{
    PRECONDITION;
    if (m_gdbAdapter->dumperHandling() == AbstractGdbAdapter::DumperNotAvailable) {
        // Load at least gdb macro based dumpers.
        m_debuggingHelperState = DebuggingHelperLoadTried;
        postCommand(Utils::FileReader::fetchQrc(_(":/gdb/gdbmacros.txt")));
        return;
    }

    if (debugPending)
        qDebug() << "TRY LOAD CUSTOM DUMPERS";
    m_debuggingHelperState = DebuggingHelperUnavailable;
    if (!checkDebuggingHelpersClassic())
        return;

    m_debuggingHelperState = DebuggingHelperLoadTried;
    QByteArray dlopenLib;
    const DebuggerStartMode startMode = startParameters().startMode;
    if (startMode == AttachToRemote || startMode == StartRemoteGdb)
        dlopenLib = startParameters().remoteDumperLib;
    else
        dlopenLib = qtDumperLibraryName().toLocal8Bit();

    // Do not use STRINGIFY for RTLD_NOW as we really want to expand that to a number.
#if defined(Q_OS_WIN) || defined(Q_OS_SYMBIAN)
    // We are using Python on Windows and Symbian.
    QTC_CHECK(false);
#elif defined(Q_OS_MAC)
    //postCommand("sharedlibrary libc"); // for malloc
    //postCommand("sharedlibrary libdl"); // for dlopen
    const QByteArray flag = QByteArray::number(RTLD_NOW);
    postCommand("call (void)dlopen(\"" + GdbMi::escapeCString(dlopenLib)
                + "\", " + flag + ")",
        CB(handleDebuggingHelperSetup));
    //postCommand("sharedlibrary " + dotEscape(dlopenLib));
#else
    //postCommand("p dlopen");
    const QByteArray flag = QByteArray::number(RTLD_NOW);
    postCommand("sharedlibrary libc"); // for malloc
    postCommand("sharedlibrary libdl"); // for dlopen
    postCommand("call (void*)dlopen(\"" + GdbMi::escapeCString(dlopenLib)
                + "\", " + flag + ")",
        CB(handleDebuggingHelperSetup));
    // Some older systems like CentOS 4.6 prefer this:
    postCommand("call (void*)__dlopen(\"" + GdbMi::escapeCString(dlopenLib)
                + "\", " + flag + ")",
        CB(handleDebuggingHelperSetup));
    postCommand("sharedlibrary " + dotEscape(dlopenLib));
#endif
    tryQueryDebuggingHelpersClassic();
}

void GdbEngine::tryQueryDebuggingHelpersClassic()
{
    PRECONDITION;
    // Retrieve list of dumpable classes.
    postCommand("call (void*)qDumpObjectData440(1,0,0,0,0,0,0,0)");
    postCommand("p (char*)&qDumpOutBuffer",
        CB(handleQueryDebuggingHelperClassic));
}

// Called from CoreAdapter and AttachAdapter
void GdbEngine::updateAllClassic()
{
    PRECONDITION;
    if (debugPending)
        qDebug() << "UPDATING ALL\n";
    QTC_ASSERT(state() == InferiorUnrunnable || state() == InferiorStopOk,
        qDebug() << state());
    tryLoadDebuggingHelpersClassic();
    reloadModulesInternal();
    postCommand("-stack-list-frames", WatchUpdate,
        CB(handleStackListFrames),
        QVariant::fromValue<StackCookie>(StackCookie(false, true)));
    stackHandler()->setCurrentIndex(0);
    if (supportsThreads())
        postCommand("-thread-list-ids", WatchUpdate, CB(handleThreadListIds), 0);
    reloadRegisters();
    updateLocals();
}

void GdbEngine::setDebuggingHelperStateClassic(DebuggingHelperState s)
{
    PRECONDITION;
    m_debuggingHelperState = s;
}

void GdbEngine::handleStackListArgumentsClassic(const GdbResponse &response)
{
    PRECONDITION;
    // stage 1/2

    // Linux:
    // 12^done,stack-args=
    //   [frame={level="0",args=[
    //     {name="argc",type="int",value="1"},
    //     {name="argv",type="char **",value="(char **) 0x7..."}]}]
    // Mac:
    // 78^done,stack-args=
    //    {frame={level="0",args={
    //      varobj=
    //        {exp="this",value="0x38a2fab0",name="var21",numchild="3",
    //             type="CurrentDocumentFind *  const",typecode="PTR",
    //             dynamic_type="",in_scope="true",block_start_addr="0x3938e946",
    //             block_end_addr="0x3938eb2d"},
    //      varobj=
    //         {exp="before",value="@0xbfffb9f8: {d = 0x3a7f2a70}",
    //              name="var22",numchild="1",type="const QString  ...} }}}
    //
    // In both cases, iterating over the children of stack-args/frame/args
    // is ok.
    m_currentFunctionArgs.clear();
    if (response.resultClass == GdbResultDone) {
        const GdbMi list = response.data.findChild("stack-args");
        const GdbMi frame = list.findChild("frame");
        const GdbMi args = frame.findChild("args");
        m_currentFunctionArgs = args.children();
    } else {
        // Seems to occur on "RedHat 4 based Linux" gdb 7.0.1:
        // ^error,msg="Cannot access memory at address 0x0"
        showMessage(_("UNEXPECTED RESPONSE: ") + response.toString());
    }
}

void GdbEngine::handleStackListLocalsClassic(const GdbResponse &response)
{
    PRECONDITION;
    // stage 2/2

    // There could be shadowed variables
    QList<GdbMi> locals = response.data.findChild("locals").children();
    locals += m_currentFunctionArgs;
    QMap<QByteArray, int> seen;
    // If desired, retrieve list of uninitialized variables looking at
    // the current frame. This is invoked first time after a stop from
    // handleStop1, which passes on the frame as cookie. The whole stack
    // is not known at this point.
    QStringList uninitializedVariables;
    if (debuggerCore()->action(UseCodeModel)->isChecked()) {
        const StackFrame frame =
            qVariantCanConvert<Debugger::Internal::StackFrame>(response.cookie)
                ? qVariantValue<Debugger::Internal::StackFrame>(response.cookie)
                : stackHandler()->currentFrame();
        if (frame.isUsable())
            getUninitializedVariables(debuggerCore()->cppCodeModelSnapshot(),
                                      frame.function, frame.file, frame.line,
                                      &uninitializedVariables);
    }
    QList<WatchData> list;
    foreach (const GdbMi &item, locals) {
        const WatchData data = localVariable(item, uninitializedVariables, &seen);
        if (data.isValid())
            list.push_back(data);
    }

    if (!m_resultVarName.isEmpty()) {
        WatchData rd;
        rd.iname = "return.0";
        rd.name = "return";
        rd.exp = m_resultVarName;
        list.append(rd);
    }

    watchHandler()->insertBulkData(list);
    watchHandler()->updateWatchers();
}

bool GdbEngine::checkDebuggingHelpersClassic()
{
    PRECONDITION;
    if (!qtDumperLibraryEnabled())
        return false;
    const QString lib = qtDumperLibraryName();
    if (QFileInfo(lib).exists())
        return true;
    const QStringList &locations = qtDumperLibraryLocations();
    const QString loc = locations.join(QLatin1String(", "));
    const QString msg = tr("The debugging helper library was not found at %1.")
            .arg(loc);
    showMessage(msg);
    // This can happen for remote debugging.
    if (!locations.isEmpty())
        showQtDumperLibraryWarning(msg); // This might build the library.
    return QFileInfo(lib).exists();
}

void GdbEngine::handleQueryDebuggingHelperClassic(const GdbResponse &response)
{
    const double dumperVersionRequired = 1.0;
    //qDebug() << "DATA DUMPER TRIAL:" << response.toString();

    GdbMi contents;
    QTC_ASSERT(parseConsoleStream(response, &contents), qDebug() << response.toString());
    const bool ok = m_dumperHelper.parseQuery(contents)
        && m_dumperHelper.typeCount();
    if (ok) {
        // Get version and sizes from dumpers. Expression cache
        // currently causes errors.
        const double dumperVersion = getDumperVersion(contents);
        if (dumperVersion < dumperVersionRequired) {
            showQtDumperLibraryWarning(
                DumperHelper::msgDumperOutdated(dumperVersionRequired, dumperVersion));
            m_debuggingHelperState = DebuggingHelperUnavailable;
            return;
        }
        m_debuggingHelperState = DebuggingHelperAvailable;
        const QString successMsg = tr("Dumper version %1, %n custom dumpers found.",
            0, m_dumperHelper.typeCount()).arg(dumperVersion);
        showStatusMessage(successMsg);

        // Sanity check for Qt version of dumpers and debuggee.
        QByteArray ns = m_dumperHelper.qtNamespace();
        postCommand("-var-create A@ * '" + ns + "qVersion'()",
            CB(handleDebuggingHelperVersionCheckClassic));
        postCommand("-var-delete A@");
    } else {
        // Retry if thread has not terminated yet.
        m_debuggingHelperState = DebuggingHelperUnavailable;
        showStatusMessage(tr("Debugging helpers not found."));
    }
    //qDebug() << m_dumperHelper.toString(true);
    //qDebug() << m_availableSimpleDebuggingHelpers << "DATA DUMPERS AVAILABLE";
}

void GdbEngine::handleDebuggingHelperVersionCheckClassic(const GdbResponse &response)
{
    if (response.resultClass == GdbResultDone) {
        QString value = _(response.data.findChild("value").data());
        QString debuggeeQtVersion = value.section(QLatin1Char('"'), 1, 1);
        QString dumperQtVersion = m_dumperHelper.qtVersionString();
        if (debuggeeQtVersion.isEmpty()) {
            showMessage(_("DUMPER VERSION CHECK SKIPPED, NO qVersion() OUTPUT IN")
                + response.toString());
        } else if (dumperQtVersion.isEmpty()) {
            showMessage(_("DUMPER VERSION CHECK SKIPPED, NO VERSION STRING"));
        } else if (dumperQtVersion != debuggeeQtVersion) {
            showMessageBox(QMessageBox::Warning,
                tr("Debugging helpers: Qt version mismatch"),
                tr("The Qt version used to build the debugging helpers (%1) "
                   "does not match the Qt version used to build the debugged "
                   "application (%2).\nThis might yield incorrect results.")
                    .arg(dumperQtVersion).arg(debuggeeQtVersion));
        } else {
            showMessage(_("DUMPER VERSION CHECK SUCCESSFUL: ")
                         + dumperQtVersion);
        }
    } else {
        showMessage("DUMPER VERSION CHECK NOT COMPLETED");
    }
}

void GdbEngine::handleVarListChildrenHelperClassic(const GdbMi &item,
    const WatchData &parent, int sortId)
{
    //qDebug() <<  "VAR_LIST_CHILDREN: PARENT" << parent.toString();
    //qDebug() <<  "VAR_LIST_CHILDREN: ITEM" << item.toString();
    QByteArray exp = item.findChild("exp").data();
    QByteArray name = item.findChild("name").data();
    if (isAccessSpecifier(exp)) {
        // Suppress 'private'/'protected'/'public' level.
        WatchData data;
        data.variable = name;
        data.iname = parent.iname;
        //data.iname = data.variable;
        data.exp = parent.exp;
        data.setTypeUnneeded();
        data.setValueUnneeded();
        data.setHasChildrenUnneeded();
        data.setChildrenUnneeded();
        QByteArray cmd = "-var-list-children --all-values \"" + data.variable + '"';
        //iname += '.' + exp;
        postCommand(cmd, WatchUpdate,
            CB(handleVarListChildrenClassic), QVariant::fromValue(data));
    } else if (!startsWithDigit(exp)
            && item.findChild("numchild").data() == "0") {
        // Happens for structs without data, e.g. interfaces.
        WatchData data;
        data.name = _(exp);
        data.iname = parent.iname + '.' + data.name.toLatin1();
        data.variable = name;
        setWatchDataType(data, item.findChild("type"));
        setWatchDataValue(data, item);
        setWatchDataAddress(data, item.findChild("addr"), GdbMi());
        data.setHasChildren(false);
        insertData(data);
    } else if (parent.iname.endsWith('.')) {
        // Happens with anonymous unions.
        WatchData data;
        data.iname = name;
        QByteArray cmd = "-var-list-children --all-values \"" + data.variable + '"';
        postCommand(cmd, WatchUpdate,
            CB(handleVarListChildrenClassic), QVariant::fromValue(data));
    } else if (exp == "staticMetaObject") {
        //    && item.findChild("type").data() == "const QMetaObject")
        // FIXME: Namespaces?
        // { do nothing }    FIXME: make configurable?
        // special "clever" hack to avoid clutter in the GUI.
        // I am not sure this is a good idea...
    } else {
        // Suppress 'private'/'protected'/'public' level.
        WatchData data;
        data.iname = parent.iname + '.' + exp;
        data.variable = name;
        data.sortId = sortId;
        setWatchDataType(data, item.findChild("type"));
        setWatchDataValue(data, item);
        setWatchDataAddress(data, item.findChild("addr"), GdbMi());
        setWatchDataChildCount(data, item.findChild("numchild"));
        if (!watchHandler()->isExpandedIName(data.iname))
            data.setChildrenUnneeded();

        data.name = _(exp);
        if (data.type == data.name) {
            if (isPointerType(parent.type)) {
                data.exp = "*(" + parent.exp + ')';
                data.name = _("*") + parent.name;
            } else {
                // A type we derive from? gdb crashes when creating variables here
                data.exp = parent.exp;
            }
        } else if (exp.startsWith('*')) {
            // A pointer
            data.exp = "*(" + parent.exp + ')';
        } else if (startsWithDigit(data.name)) {
            // An array. No variables needed?
            data.name = _c('[') + data.name + _c(']');
            data.exp = parent.exp + '[' + exp + ']';
        } else if (0 && parent.name.endsWith(_c('.'))) {
            // Happens with anonymous unions
            data.exp = parent.exp + data.name.toLatin1();
            //data.name = "<anonymous union>";
        } else if (exp.isEmpty()) {
            // Happens with anonymous unions
            data.exp = parent.exp;
            data.name = tr("<n/a>");
            data.iname = parent.iname + ".@";
            data.type = tr("<anonymous union>").toUtf8();
        } else {
            // A structure. Hope there's nothing else...
            data.exp = '(' + parent.exp + ")." + data.name.toLatin1();
        }

        if (hasDebuggingHelperForType(data.type)) {
            // we do not trust gdb if we have a custom dumper
            data.setValueNeeded();
            data.setHasChildrenNeeded();
        }

        //qDebug() <<  "VAR_LIST_CHILDREN: PARENT 3" << parent.toString();
        //qDebug() <<  "VAR_LIST_CHILDREN: APPENDEE" << data.toString();
        insertData(data);
    }
}

void GdbEngine::handleVarListChildrenClassic(const GdbResponse &response)
{
    //WatchResultCounter dummy(this, WatchVarListChildren);
    WatchData data = response.cookie.value<WatchData>();
    if (!data.isValid())
        return;
    if (response.resultClass == GdbResultDone) {
        //qDebug() <<  "VAR_LIST_CHILDREN: PARENT" << data.toString();
        QList<GdbMi> children = response.data.findChild("children").children();

        for (int i = 0; i != children.size(); ++i)
            handleVarListChildrenHelperClassic(children.at(i), data, i);

        if (children.isEmpty()) {
            // happens e.g. if no debug information is present or
            // if the class really has no children
            WatchData data1;
            data1.iname = data.iname + ".child";
            //: About variable's value
            data1.value = tr("<no information>");
            data1.hasChildren = false;
            data1.setAllUnneeded();
            insertData(data1);
            data.setAllUnneeded();
            insertData(data);
        } else if (data.variable.endsWith("private")
                || data.variable.endsWith("protected")
                || data.variable.endsWith("public")) {
            // this skips the spurious "public", "private" etc levels
            // gdb produces
        } else {
            data.setChildrenUnneeded();
            insertData(data);
        }
    } else {
        data.setError(QString::fromLocal8Bit(response.data.findChild("msg").data()));
    }
}

void GdbEngine::handleEvaluateExpressionClassic(const GdbResponse &response)
{
    WatchData data = response.cookie.value<WatchData>();
    QTC_ASSERT(data.isValid(), qDebug() << "HUH?");
    if (response.resultClass == GdbResultDone) {
        //if (col == 0)
        //    data.name = response.data.findChild("value").data();
        //else
            setWatchDataValue(data, response.data);
    } else {
        data.setError(QString::fromLocal8Bit(response.data.findChild("msg").data()));
    }
    //qDebug() << "HANDLE EVALUATE EXPRESSION:" << data.toString();
    insertData(data);
    //updateWatchModel2();
}

} // namespace Internal
} // namespace Debugger
