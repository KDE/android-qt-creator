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

#include "symbolgroupnode.h"
#include "symbolgroup.h"
#include "symbolgroupvalue.h"
#include "stringutils.h"
#include "base64.h"
#include "containers.h"
#include "extensioncontext.h"

#include <algorithm>

typedef std::vector<int>::size_type VectorIndexType;
typedef std::vector<std::string> StringVector;

enum { BufSize = 2048 };

static inline void indentStream(std::ostream &str, unsigned depth)
{
    for (unsigned d = 0; d < depth; ++d)
        str << "  ";
}

static inline void debugNodeFlags(std::ostream &str, unsigned f)
{
    if (!f)
        return;
    str << " node-flags=" << f;
    if (f & SymbolGroupNode::Uninitialized)
        str << " UNINITIALIZED";
    if (f & SymbolGroupNode::SimpleDumperNotApplicable)
        str << " DumperNotApplicable";
    if (f & SymbolGroupNode::SimpleDumperOk)
        str << " DumperOk";
    if (f & SymbolGroupNode::SimpleDumperFailed)
        str << " DumperFailed";
    if (f & SymbolGroupNode::ExpandedByDumper)
        str << " ExpandedByDumper";
    if (f & SymbolGroupNode::AdditionalSymbol)
        str << " AdditionalSymbol";
    if (f & SymbolGroupNode::Obscured)
        str << " Obscured";
    if (f & SymbolGroupNode::ComplexDumperOk)
        str << " ComplexDumperOk";
    if (f & SymbolGroupNode::WatchNode)
        str << " WatchNode";
    str << ' ';
}

// Some helper to conveniently dump flags to a stream
struct DebugNodeFlags
{
    DebugNodeFlags(unsigned f) : m_f(f) {}
    const unsigned m_f;
};

inline std::ostream &operator<<(std::ostream &str, const DebugNodeFlags &f)
{
    debugNodeFlags(str, f.m_f);
    return str;
}

/*!
  \class AbstractSymbolGroupNode

    Abstract base class for a node of SymbolGroup providing the child list interface.
    \ingroup qtcreatorcdbext
*/
AbstractSymbolGroupNode::AbstractSymbolGroupNode(const std::string &name,
                                                 const std::string &iname) :
    m_name(name), m_iname(iname), m_parent(0), m_flags(0)
{
}

AbstractSymbolGroupNode::~AbstractSymbolGroupNode()
{
}

std::string AbstractSymbolGroupNode::absoluteFullIName() const
{
    std::string rc = iName();
    for (const AbstractSymbolGroupNode *p = m_parent; p; p = p->m_parent) {
        rc.insert(0, 1, SymbolGroupNodeVisitor::iNamePathSeparator);
        rc.insert(0, p->iName());
    }
    return rc;
}

AbstractSymbolGroupNode *AbstractSymbolGroupNode::childAt(unsigned i) const
{
    const AbstractSymbolGroupNodePtrVector &c = children();
    return i < c.size() ? c.at(i) : static_cast<AbstractSymbolGroupNode *>(0);
}

unsigned AbstractSymbolGroupNode::indexByIName(const char *n) const
{
    const AbstractSymbolGroupNodePtrVector &c = children();
    const VectorIndexType size = c.size();
    for (VectorIndexType i = 0; i < size; ++i)
        if ( c.at(i)->iName() == n )
            return unsigned(i);
    return unsigned(-1);
}

AbstractSymbolGroupNode *AbstractSymbolGroupNode::childByIName(const char *n) const
{
    const unsigned index = indexByIName(n);
    if (index != unsigned(-1))
        return children().at(index);
    return 0;
}

unsigned AbstractSymbolGroupNode::indexOf(const AbstractSymbolGroupNode *n) const
{
    const AbstractSymbolGroupNodePtrVector::const_iterator it = std::find(children().begin(), children().end(), n);
    return it != children().end() ? unsigned(it - children().begin()) : unsigned(-1);
}

bool AbstractSymbolGroupNode::accept(SymbolGroupNodeVisitor &visitor,
                                     const std::string &parentIname,
                                     unsigned child, unsigned depth)
{
    // If we happen to be the root node, just skip over
    const bool invisibleRoot = !m_parent;
    const unsigned childDepth = invisibleRoot ? 0 : depth + 1;

    std::string fullIname = parentIname;
    if (!fullIname.empty())
        fullIname.push_back(SymbolGroupNodeVisitor::iNamePathSeparator);
    fullIname += m_iname;

    const SymbolGroupNodeVisitor::VisitResult vr =
            invisibleRoot ? SymbolGroupNodeVisitor::VisitContinue :
                            visitor.visit(this, fullIname, child, depth);
    switch (vr) {
    case SymbolGroupNodeVisitor::VisitStop:
        return true;
    case SymbolGroupNodeVisitor::VisitSkipChildren:
        break;
    case SymbolGroupNodeVisitor::VisitContinue: {
        const AbstractSymbolGroupNodePtrVector &c = children();
        const unsigned childCount = unsigned(c.size());
        for (unsigned i = 0; i < childCount; ++i)
            if (c.at(i)->accept(visitor, fullIname, i, childDepth))
                return true;
        if (!invisibleRoot)
            visitor.childrenVisited(this, depth);
    }
        break;
    }
    return false;
}

void AbstractSymbolGroupNode::debug(std::ostream &str, const std::string &visitingFullIname,
                                    unsigned /* verbosity */, unsigned depth) const
{
    indentStream(str, 2 * depth);
    str << "AbstractSymbolGroupNode " << visitingFullIname
        << " with " << children().size() << " children\n";
}

void AbstractSymbolGroupNode::dumpBasicData(std::ostream &str, const std::string &aName,
                                         const std::string &aFullIname,
                                         const std::string &type /* = "" */,
                                         const std::string &expression /* = "" */)
{
    str << "iname=\"" << aFullIname << "\",name=\"" << aName << '"';
    if (!type.empty())
        str << ",type=\"" << type << '"';
    if (!expression.empty())
        str << ",exp=\"" << expression  << '"';
}

void AbstractSymbolGroupNode::setParent(AbstractSymbolGroupNode *n)
{
    if (m_parent)
        dprintf("Internal error: Attempt to change non-null parent of %s", m_name.c_str());
    m_parent = n;
}

/*! \class BaseSymbolGroupNode

    Base class for a node of SymbolGroup with a flat list of children.
    \ingroup qtcreatorcdbext
*/

BaseSymbolGroupNode::BaseSymbolGroupNode(const std::string &name, const std::string &iname) :
    AbstractSymbolGroupNode(name, iname)
{
}

BaseSymbolGroupNode::~BaseSymbolGroupNode()
{
    removeChildren();
}

void BaseSymbolGroupNode::removeChildAt(unsigned n)
{
    if (VectorIndexType(n) >= m_children.size())
        return;
    const AbstractSymbolGroupNodePtrVector::iterator it = m_children.begin() + n;
    delete *it;
    m_children.erase(it, it + 1);
}

void BaseSymbolGroupNode::removeChildren()
{
    if (!m_children.empty()) {
        const AbstractSymbolGroupNodePtrVectorIterator end = m_children.end();
        for (AbstractSymbolGroupNodePtrVectorIterator it = m_children.begin(); it != end; ++it)
            delete *it;
        m_children.clear();
    }
}

void BaseSymbolGroupNode::addChild(AbstractSymbolGroupNode *c)
{
    c->setParent(this);
    m_children.push_back(c);
}

// ----------- Helpers: Stream DEBUG_SYMBOL_PARAMETERS

std::ostream &operator<<(std::ostream &str, const DEBUG_SYMBOL_PARAMETERS &parameters)
{
    str << "parent=";
    if (parameters.ParentSymbol == DEBUG_ANY_ID) {
        str << "DEBUG_ANY_ID";
    } else {
        str << parameters.ParentSymbol ;
    }
    if (parameters.Flags != 0 && parameters.Flags != 1)
        str << " flags=" << parameters.Flags;
    // Detailed flags:
    if (parameters.Flags & DEBUG_SYMBOL_EXPANDED)
        str << " EXPANDED";
    if (parameters.Flags & DEBUG_SYMBOL_READ_ONLY)
        str << " READONLY";
    if (parameters.Flags & DEBUG_SYMBOL_IS_ARRAY)
        str << " ARRAY";
    if (parameters.Flags & DEBUG_SYMBOL_IS_FLOAT)
        str << " FLOAT";
    if (parameters.Flags & DEBUG_SYMBOL_IS_ARGUMENT)
        str << " ARGUMENT";
    if (parameters.Flags & DEBUG_SYMBOL_IS_LOCAL)
        str << " LOCAL";
    str << " typeId=" << parameters.TypeId;
    if (parameters.SubElements)
        str << " subElements=" << parameters.SubElements;
    return str;
}

/*! \struct DumpParameters

    All parameters for GDBMI dumping of a symbol group in one struct.
    The debugging engine passes maps of type names/inames to special
    integer values indicating hex/dec, etc.
    \ingroup qtcreatorcdbext
*/

DumpParameters::DumpParameters() : dumpFlags(0)
{
}

// typeformats: decode hex-encoded name, value pairs:
// '414A=2,...' -> map of "AB:2".
DumpParameters::FormatMap DumpParameters::decodeFormatArgument(const std::string &f)
{
    FormatMap rc;
    const std::string::size_type size = f.size();
    // Split 'hexname=4,'
    for (std::string::size_type pos = 0; pos < size ; ) {
        // Cut out key
        const std::string::size_type equalsPos = f.find('=', pos);
        if (equalsPos == std::string::npos)
            return rc;
        const std::string name = stringFromHex(f.c_str() + pos, f.c_str() + equalsPos);
        // Search for number
        const std::string::size_type numberPos = equalsPos + 1;
        std::string::size_type nextPos = f.find(',', numberPos);
        if (nextPos == std::string::npos)
            nextPos = size;
        int format;
        if (!integerFromString(f.substr(numberPos, nextPos - numberPos), &format))
            return rc;
        rc.insert(FormatMap::value_type(name, format));
        pos = nextPos + 1;
    }
    return rc;
}

int DumpParameters::format(const std::string &type, const std::string &iname) const
{
    if (!individualFormats.empty()) {
        const FormatMap::const_iterator iit = individualFormats.find(iname);
        if (iit != individualFormats.end())
            return iit->second;
    }
    if (!typeFormats.empty()) {
        const FormatMap::const_iterator tit = typeFormats.find(type);
        if (tit != typeFormats.end())
            return tit->second;
    }
    return -1;
}

enum PointerFormats // Watch data pointer format requests
{
    FormatAuto = 0,
    FormatLatin1String = 1,
    FormatUtf8String = 2,
    FormatUtf16String = 3,
    FormatUcs4String = 4
};

enum DumpEncoding // WatchData encoding of GDBMI values
{
    DumpEncodingAscii = 0,
    DumpEncodingBase64 = 1,
    DumpEncodingBase64_Utf16 = 2,
    DumpEncodingBase64_Ucs4 = 3,
    DumpEncodingHex_Latin1 = 6,
    DumpEncodingHex_Utf16 = 7,
    DumpEncodingHex_Ucs4_LittleEndian = 8,
    DumpEncodingHex_Utf8_LittleEndian = 9,
    DumpEncodingHex_Ucs4_BigEndian = 10,
    DumpEncodingHex_Utf16_BigEndian = 11,
    DumpEncodingHex_Utf16_LittleEndian = 12
};

/* Recode arrays/pointers of char*, wchar_t according to users
 * sepcification. Handles char formats for 'char *', '0x834478 "hallo.."'
 * and 'wchar_t *', '0x834478 "hallo.."'.
 * This is done by retrieving the address and the length (in characters)
 * of the CDB output, converting it to memory size, fetching the data
 * from memory, zero-terminating and recoding it using the encoding
 * defined in watchutils.cpp.
 * As a special case, if there is no user-defined format and the
 * CDB output contains '?' indicating non-printable characters,
 * switch to latin1 (8bit) such that the watchmodel formatting options trigger. */

bool DumpParameters::recode(const std::string &type,
                            const std::string &iname,
                            const SymbolGroupValueContext &ctx,
                            std::wstring *value, int *encoding) const
{
    // We basically handle char formats for 'char *', '0x834478 "hallo.."'
    // and 'wchar_t *', '0x834478 "hallo.."'
    // Determine address and length from the pointer value output,
    // read the raw memory and recode if that is possible.
    if (type.empty() || type.at(type.size() - 1) != '*')
        return false;
    int newFormat = format(type, iname);
    if (value->compare(0, 2, L"0x"))
        return false;
    const std::wstring::size_type quote1 = value->find(L'"', 2);
    if (quote1 == std::wstring::npos)
        return false;
    // The user did not specify any format, still, there are '?'
    // (indicating non-printable) in what the debugger prints. In that case,
    // append a hex dump to the normal output. If there are no '?'-> all happy.
    if (newFormat < FormatLatin1String) {
        const bool hasNonPrintable = value->find(L'?', quote1 + 1) != std::wstring::npos;
        if (!hasNonPrintable)
            return false; // All happy, no need to re-encode
        // Pass as on 8-bit such that Watchmodel's reformatting can trigger.
        newFormat = FormatLatin1String;
    }
    const std::wstring::size_type quote2 = value->find(L'"', quote1 + 1);
    if (quote2 == std::wstring::npos)
        return false;
    std::wstring::size_type length = quote2 - quote1 - 1;
    if (!length)
        return false;
    // Get address from value
    ULONG64 address = 0;
    if (!integerFromWString(value->substr(0, quote1 - 1), &address) || !address)
        return false;
    // Get real size if this is for example a wchar_t *.
    const unsigned elementSize = SymbolGroupValue::sizeOf(SymbolGroupValue::stripPointerType(type).c_str());
    if (!elementSize)
        return false;
    length *= elementSize;
    // Allocate real length + 8 bytes ('\0') for largest format (Ucs4).
    // '\0' is not listed in the CDB output.
    const std::wstring::size_type allocLength = length + 8;
    unsigned char *buffer = new unsigned char[allocLength];
    std::fill(buffer, buffer + allocLength, 0);
    ULONG obtained = 0;
    if (FAILED(ctx.dataspaces->ReadVirtual(address, buffer, ULONG(length), &obtained))) {
        delete [] buffer;
        return false;
    }
    // Recode raw memory
    switch (newFormat) {
    case FormatLatin1String:
        *value = dataToHexW(buffer, buffer + length + 1); // Latin1 + 0
        *encoding = DumpEncodingHex_Latin1;
        break;
    case FormatUtf8String:
        *value = dataToHexW(buffer, buffer + length + 1); // UTF8 + 0
        *encoding = DumpEncodingHex_Utf8_LittleEndian;
        break;
    case FormatUtf16String: // Paranoia: make sure buffer is terminated at 2 byte borders
        if (length % 2) {
            length &= ~1;
            buffer[length] = '\0';
            buffer[length + 1] = '\0';
        }
        *value = base64EncodeToWString(buffer, length + 2);
        *encoding = DumpEncodingBase64_Utf16;
        break;
    case FormatUcs4String: // Paranoia: make sure buffer is terminated at 4 byte borders
        if (length % 4) {
            length &= ~3;
            std::fill(buffer + length, buffer + length + 4, 0);
        }
        *value = dataToHexW(buffer, buffer + length + 2); // UTF16 + 0
        *encoding = DumpEncodingHex_Ucs4_LittleEndian;
        break;
    }
    delete [] buffer;
    return true;
}

std::ostream &operator<<(std::ostream &os, const DumpParameters &d)
{
    if (d.dumpFlags & DumpParameters::DumpHumanReadable)
        os << ", human-readable";
    if (d.dumpFlags & DumpParameters::DumpComplexDumpers)
        os << ", complex dumpers";
    if (!d.typeFormats.empty()) {
        os << ", type formats: ";
        DumpParameters::FormatMap::const_iterator cend = d.typeFormats.end();
        for (DumpParameters::FormatMap::const_iterator it = d.typeFormats.begin(); it != cend; ++it)
            os << ' ' << it->first << ':' << it->second;
        os << '\n';
    }
    if (!d.individualFormats.empty()) {
        os << ", individual formats: ";
        DumpParameters::FormatMap::const_iterator cend = d.individualFormats.end();
        for (DumpParameters::FormatMap::const_iterator it = d.typeFormats.begin(); it != cend; ++it)
            os << ' ' << it->first << ':' << it->second;
        os << '\n';
    }
    return os;
}

// --------- ErrorSymbolGroupNode
ErrorSymbolGroupNode::ErrorSymbolGroupNode(const std::string &name, const std::string &iname) :
    BaseSymbolGroupNode(name, iname)
{
}

int ErrorSymbolGroupNode::dump(std::ostream &str, const std::string &fullIname,
                               const DumpParameters &, const SymbolGroupValueContext &)
{
    dumpBasicData(str, name(), fullIname, "<unknown>", std::string());
    str << ",valueencoded=\"0\",value=\"<Error>\",valueenabled=\"false\",valueeditable=\"false\"";
    return 0;
}

void ErrorSymbolGroupNode::debug(std::ostream &os, const std::string &visitingFullIname,
                                 unsigned , unsigned depth) const
{
    indentStream(os, 2 * depth);
    os << "ErrorSymbolGroupNode '" << name() << "','" << iName() << "', '" << visitingFullIname << "'\n";
}

/*! \class SymbolGroupNode

 \brief 'Real' node within a symbol group, identified by its index in IDebugSymbolGroup.

 Provides accessors for fixed-up symbol group value and a dumping facility
 consisting of:
 \list
 \o 'Simple' dumping done when running the DumpVisitor. This produces one
    line of formatted output shown for the class. These values
    values are always displayed, while still allowing for expansion of the structure
    in the debugger.
    It also pre-determines some information for complex dumping (type, container).
 \o 'Complex' dumping: Obscures the symbol group children by fake children, for
    example container children, to be run when calling SymbolGroup::dump with an iname.
    The fake children are appended to the child list (other children are just marked as
    obscured for GDBMI dumping so that SymbolGroupValue expressions still work as before).
 \endlist

 The dumping is mostly based on SymbolGroupValue expressions.
 in the debugger. Evaluating those dumpers might expand symbol nodes, which are
 then marked as 'ExpandedByDumper'. This stops the dump recursion to prevent
 outputting data that were not explicitly expanded by the watch handler.
 \ingroup qtcreatorcdbext */

SymbolGroupNode::SymbolGroupNode(SymbolGroup *symbolGroup,
                                 ULONG index,
                                 const std::string &module,
                                 const std::string &name,
                                 const std::string &iname) :
    BaseSymbolGroupNode(name, iname),
    m_symbolGroup(symbolGroup),
    m_module(module), m_index(index), m_dumperType(-1), m_dumperContainerSize(-1), m_dumperSpecialInfo(0)
{
    memset(&m_parameters, 0, sizeof(DEBUG_SYMBOL_PARAMETERS));
    m_parameters.ParentSymbol = DEBUG_ANY_ID;
}

const SymbolGroupNode *SymbolGroupNode::symbolGroupNodeParent() const
{
    if (const AbstractSymbolGroupNode *p = parent())
        return p->asSymbolGroupNode();
    return 0;
}

SymbolGroupNode *SymbolGroupNode::symbolGroupNodeParent()
{
    if (AbstractSymbolGroupNode *p = parent())
        return p->asSymbolGroupNode();
    return 0;
}

bool SymbolGroupNode::isArrayElement() const
{
    if (const SymbolGroupNode *p = symbolGroupNodeParent())
        return (p->m_parameters.Flags & DEBUG_SYMBOL_IS_ARRAY) != 0;
    return false;
}

// Notify about expansion of a node:
// Adapt our index and those of our children if we are behind it.
// Return true if a modification was required to be able to terminate the
// recursion.
bool SymbolGroupNode::notifyIndexesMoved(ULONG index, bool inserted, ULONG offset)
{
    typedef AbstractSymbolGroupNodePtrVector::const_reverse_iterator ReverseIt;

    if (SymbolGroupValue::verbose > 2)
        DebugPrint() << "notifyIndexesMoved: #" << this->index() << " '" << name()
                     << "' index=" << index << " insert="
                     << inserted << " offset=" << offset;
    // Looping backwards over the children. If a subtree has no modifications,
    // (meaning all other indexes are smaller) we can stop.
    const ReverseIt rend = children().rend();
    for (ReverseIt it = children().rbegin(); it != rend; ++it) {
        if (SymbolGroupNode *c = (*it)->asSymbolGroupNode())
            if (!c->notifyIndexesMoved(index, inserted, offset))
                return false;
    }

    // Correct our own + parent index if applicable.
    if (m_index == DEBUG_ANY_ID || m_index < index)
        return false;

    if (inserted) {
        m_index += offset;
    } else {
        m_index -= offset;
    }
    if (m_parameters.ParentSymbol != DEBUG_ANY_ID && m_parameters.ParentSymbol >= index) {
        if (inserted) {
            m_parameters.ParentSymbol += offset;
        } else {
            m_parameters.ParentSymbol -= offset;
        }
    }
    return true;
}

// Fix names: fix complicated template base names
static inline void fixName(std::string *name)
{
    // Long template base classes 'std::tree_base<Key....>' -> 'std::tree<>'
    // for nice display
    const std::string::size_type templatePos = name->find('<');
    if (templatePos != std::string::npos) {
        name->erase(templatePos + 1, name->size() - templatePos - 1);
        name->push_back('>');
    }
}

// Fix inames: arrays and long, complicated template base names
static inline void fixIname(unsigned &id, std::string *iname)
{
    // Fix array iname "[0]" -> "0" for sorting to work correctly
    if (!iname->empty() && iname->at(0) == '[') {
        const std::string::size_type last = iname->size() - 1;
        if (iname->at(last) == ']') {
            iname->erase(last, 1);
            iname->erase(0, 1);
            return;
        }
    }
    // Long template base classes 'std::tree_base<Key....' -> 'tree@t1',
    // usable as identifier and command line parameter
    const std::string::size_type templatePos = iname->find('<');
    if (templatePos != std::string::npos) {
        iname->erase(templatePos, iname->size() - templatePos);
        if (iname->compare(0, 5, "std::") == 0)
            iname->erase(0, 5);
        iname->append("@t");
        iname->append(toString(id++));
    }
}

// Fix up names and inames
static inline void fixNames(bool isTopLevel, StringVector *names, StringVector *inames)
{
    if (names->empty())
        return;
    unsigned unnamedId = 1;
    unsigned templateId = 1;
    /* 1) Fix name="__formal", which occurs when someone writes "void foo(int /* x * /)..."
     * 2) Fix array inames for sorting: "[6]" -> name="[6]",iname="6"
     * 3) For toplevels: Fix shadowed variables in the order the debugger expects them:
       \code
       int x;             // Occurrence (1), should be reported as name="x <shadowed 1>"/iname="x#1"
       if (true) {
          int x = 5; (2)  // Occurrence (2), should be reported as name="x"/iname="x"
       }
      \endcode */
    StringVector::iterator nameIt = names->begin();
    const StringVector::iterator namesEnd = names->end();
    for (StringVector::iterator iNameIt = inames->begin(); nameIt != namesEnd ; ++nameIt, ++iNameIt) {
        std::string &name = *nameIt;
        std::string &iname = *iNameIt;
        if (name.empty() || name == "__formal") {
            const std::string number = toString(unnamedId++);
            name = "<unnamed "  + number + '>';
            iname = "unnamed#" + number;
        } else {
            fixName(&name);
            fixIname(templateId, &iname);
        }
        if (isTopLevel) {
            if (const StringVector::size_type shadowCount = std::count(nameIt + 1, namesEnd, name)) {
                const std::string number = toString(shadowCount);
                name += " <shadowed ";
                name += number;
                name += '>';
                iname += '#';
                iname += number;
            }
        }
    }
}

// Index: Index of symbol, parameterOffset: Looking only at a part of the symbol array, offset
void SymbolGroupNode::parseParameters(VectorIndexType index,
                                      VectorIndexType parameterOffset,
                                      const SymbolGroup::SymbolParameterVector &vec)
{
    static char buf[BufSize];
    ULONG obtainedSize;

    const bool isTopLevel = index == DEBUG_ANY_ID;
    if (isTopLevel) {
        m_parameters.Flags |= DEBUG_SYMBOL_EXPANDED;
    } else {
        m_parameters = vec.at(index - parameterOffset);
        if (m_parameters.SubElements == 0 || !(m_parameters.Flags & DEBUG_SYMBOL_EXPANDED))
            return; // No children
    }
    if (m_parameters.SubElements > 1)
        reserveChildren(m_parameters.SubElements);

    const VectorIndexType size = vec.size();
    // Scan the top level elements
    StringVector names;
    names.reserve(size);
    // Pass 1) Determine names. We need the complete set first in order to do some corrections.
    const VectorIndexType startIndex = isTopLevel ? 0 : index + 1;
    for (VectorIndexType pos = startIndex - parameterOffset; pos < size ; ++pos) {
        if (vec.at(pos).ParentSymbol == index) {
            const VectorIndexType symbolGroupIndex = pos + parameterOffset;
            if (FAILED(m_symbolGroup->debugSymbolGroup()->GetSymbolName(ULONG(symbolGroupIndex), buf, BufSize, &obtainedSize)))
                buf[0] = '\0';
            names.push_back(std::string(buf));
        }
    }
    // 2) Fix names
    StringVector inames = names;
    fixNames(isTopLevel, &names, &inames);
    // Pass 3): Add nodes with fixed names
    StringVector::size_type nameIndex = 0;
    for (VectorIndexType pos = startIndex - parameterOffset; pos < size ; ++pos) {
        if (vec.at(pos).ParentSymbol == index) {
            const VectorIndexType symbolGroupIndex = pos + parameterOffset;
            SymbolGroupNode *child = new SymbolGroupNode(m_symbolGroup,
                                                         ULONG(symbolGroupIndex),
                                                         m_module,
                                                         names.at(nameIndex),
                                                         inames.at(nameIndex));
            child->parseParameters(symbolGroupIndex, parameterOffset, vec);
            addChild(child);
            nameIndex++;
        }
    }
    if (isTopLevel)
        m_parameters.SubElements = ULONG(children().size());
}

SymbolGroupNode *SymbolGroupNode::create(SymbolGroup *sg, const std::string &module,
                                         const std::string &name, const SymbolGroup::SymbolParameterVector &vec)
{
    SymbolGroupNode *rc = new SymbolGroupNode(sg, DEBUG_ANY_ID, module, name, name);
    rc->parseParameters(DEBUG_ANY_ID, 0, vec);
    return rc;
}

// Fix some oddities in CDB values

static inline bool isHexDigit(wchar_t c)
{
    return (c >= L'0' && c <= L'9') || (c >= L'a' && c <= L'f') || (c >= L'A' && c <= L'F');
}

static void fixValue(const std::string &type, std::wstring *value)
{
    // Pointers/Unsigned integers: fix '0x00000000`00000AD bla' ... to "0xAD bla"
    const bool isHexNumber = value->size() > 3 && value->compare(0, 2, L"0x") == 0 && isHexDigit(value->at(2));
    if (isHexNumber) {
        // Remove dumb 64bit separator
        if (value->size() > 10 && value->at(10) == L'`')
            value->erase(10, 1);
        const std::string::size_type firstNonNullDigit = value->find_first_not_of(L"0", 2);
        // No on-null digits: plain null ptr.
        if (firstNonNullDigit == std::string::npos || value->at(firstNonNullDigit) == ' ') {
            *value = L"0x0";
        } else {
        // Strip
            if (firstNonNullDigit > 2)
                value->erase(2, firstNonNullDigit - 2);
        }
    }

    // Strip a vtable "0x13f37b7c8 module!Class::`vftable'" to a plain pointer.
    if (SymbolGroupValue::isVTableType(type)) {
        const std::wstring::size_type blankPos = value->find(L' ', 2);
        if (blankPos != std::wstring::npos)
            value->erase(blankPos, value->size() - blankPos);
        return;
    }

    // Pointers: fix '0x00000000`00000AD class bla' ... to "0xAD", but leave
    // 'const char *' values as is ('0x00000000`00000AD "hallo").
    if (!type.empty() && type.at(type.size() - 1) == L'*') {
        // Strip ' Class bla"
        std::wstring::size_type classPos = value->find(L" struct", 2);
        if (classPos == std::string::npos)
            classPos = value->find(L" class", 2);
        if (classPos != std::string::npos)
            value->erase(classPos, value->size() - classPos);
        return;
    }

    // unsigned hex ints that are not pointers: Convert to decimal as not to confuse watch model:
    if (isHexNumber) {
        ULONG64 uv;
        std::wistringstream str(*value);
        str >> std::hex >> uv;
        if (!str.fail()) {
            *value = toWString(uv);
            return;
        }
    }

    // Integers: fix '0n10' -> '10'
    if (value->size() >= 3 && value->compare(0, 2, L"0n") == 0
        && (isdigit(value->at(2)) || value->at(2) == L'-')) {
        value->erase(0, 2);
        return;
    }
    // Fix long class names on std containers 'class std::tree<...>' -> 'class std::tree<>'
    if (value->compare(0, 6, L"class ") == 0 || value->compare(0, 7, L"struct ") == 0) {
        const std::string::size_type openTemplate = value->find(L'<');
        if (openTemplate != std::string::npos) {
            value->erase(openTemplate + 1, value->size() - openTemplate - 2);
            return;
        }
    }
}

// Check for ASCII-encode-able stuff. Plain characters + tabs at the most, no newline.
static bool isSevenBitClean(const wchar_t *buf, size_t size)
{
    const wchar_t *bufEnd = buf + size;
    for (const wchar_t *bufPtr = buf; bufPtr < bufEnd; bufPtr++) {
        const wchar_t c = *bufPtr;
        if (c > 127 || (c < 32 && c != 9))
            return false;
    }
    return true;
}

std::string SymbolGroupNode::type() const
{
    static char buf[BufSize];
    const HRESULT hr = m_symbolGroup->debugSymbolGroup()->GetSymbolTypeName(m_index, buf, BufSize, NULL);
    return SUCCEEDED(hr) ? std::string(buf) : std::string();
}

unsigned SymbolGroupNode::size() const
{
    DEBUG_SYMBOL_ENTRY entry;
    if (SUCCEEDED(m_symbolGroup->debugSymbolGroup()->GetSymbolEntryInformation(m_index, &entry)))
        return entry.Size;
    return 0;
}

ULONG64 SymbolGroupNode::address() const
{
    ULONG64 address = 0;
    const HRESULT hr = m_symbolGroup->debugSymbolGroup()->GetSymbolOffset(m_index, &address);
    if (SUCCEEDED(hr))
        return address;
    return 0;
}

std::wstring SymbolGroupNode::symbolGroupRawValue() const
{
    // Determine size and return allocated buffer
    const ULONG maxValueSize = 262144;
    ULONG obtainedSize = 0;
    HRESULT hr = m_symbolGroup->debugSymbolGroup()->GetSymbolValueTextWide(m_index, NULL, maxValueSize, &obtainedSize);
    if (FAILED(hr))
        return std::wstring();
    if (obtainedSize > maxValueSize)
        obtainedSize = maxValueSize;
    wchar_t *buffer = new wchar_t[obtainedSize];
    hr = m_symbolGroup->debugSymbolGroup()->GetSymbolValueTextWide(m_index, buffer, obtainedSize, &obtainedSize);
    if (FAILED(hr)) // Whoops, should not happen
        buffer[0] = 0;
    const std::wstring rc(buffer);
    delete [] buffer;
    return rc;
}

std::wstring SymbolGroupNode::symbolGroupFixedValue() const
{
    std::wstring value = symbolGroupRawValue();
    fixValue(type(), &value);
    return value;
}

// A quick check if symbol is valid by checking for inaccessible value
bool SymbolGroupNode::isMemoryAccessible() const
{
    static const char notAccessibleValueC[] = "<Memory access error>";
    char buffer[sizeof(notAccessibleValueC)];
    ULONG obtained = 0;
    if (FAILED(symbolGroup()->debugSymbolGroup()->GetSymbolValueText(m_index, buffer, sizeof(notAccessibleValueC), &obtained)))
            return false;
    if (obtained < sizeof(notAccessibleValueC))
        return true;
    return strcmp(buffer, notAccessibleValueC) != 0;
}

// Complex dumpers: Get container/fake children
void SymbolGroupNode::runComplexDumpers(const SymbolGroupValueContext &ctx)
{
    if (symbolGroupDebug) {
        DebugPrint dp;
        dp << "SymbolGroupNode::runComplexDumpers "  << name() << '/'
           << absoluteFullIName() << ' ' << m_index << ' ' << DebugNodeFlags(flags())
           << " type: ";
        formatKnownTypeFlags(dp, static_cast<KnownType>(m_dumperType));
        if (m_dumperSpecialInfo)
            dp << std::hex << std::showbase << " Special " << m_dumperSpecialInfo;
    }

    if ((testFlags(ComplexDumperOk) || !testFlags(SimpleDumperOk)))
        return;

    const bool isContainer = (m_dumperType & KT_ContainerType) != 0;
    const bool otherDumper = (m_dumperType & KT_HasComplexDumper) != 0;
    if (!isContainer && !otherDumper)
        return;
    if (isContainer && m_dumperContainerSize <= 0)
        return;

    addFlags(ComplexDumperOk);
    const AbstractSymbolGroupNodePtrVector ctChildren = otherDumper ?
             dumpComplexType(this, m_dumperType, m_dumperSpecialInfo, ctx) :
             containerChildren(this, m_dumperType, m_dumperContainerSize, ctx);
    m_dumperContainerSize = int(ctChildren.size()); // Just in case...
    if (ctChildren.empty())
        return;

    clearFlags(ExpandedByDumper);
    // Mark current children as obscured. We cannot show both currently
    // as this would upset the numerical sorting of the watch model
    AbstractSymbolGroupNodePtrVectorConstIterator cend = children().end();
    for (AbstractSymbolGroupNodePtrVectorConstIterator it = children().begin(); it != cend; ++it)
        (*it)->addFlags(Obscured);
    // Add children and mark them as referenced by us.
    cend = ctChildren.end();
    for (AbstractSymbolGroupNodePtrVectorConstIterator it = ctChildren.begin(); it != cend; ++it)
        addChild(*it);
}

// Run dumpers, format simple in-line dumper value and retrieve fake children
bool SymbolGroupNode::runSimpleDumpers(const SymbolGroupValueContext &ctx)
{
    if (symbolGroupDebug)
        DebugPrint() << ">SymbolGroupNode::runSimpleDumpers "  << name() << '/'
                        << absoluteFullIName() << ' ' << m_index << DebugNodeFlags(flags());
    if (testFlags(Uninitialized))
        return false;
    if (testFlags(SimpleDumperOk))
        return true;
    if (testFlags(SimpleDumperMask))
        return false;
    addFlags(dumpSimpleType(this , ctx, &m_dumperValue,
                            &m_dumperType, &m_dumperContainerSize, &m_dumperSpecialInfo));
    if (symbolGroupDebug)
        DebugPrint() << "<SymbolGroupNode::runSimpleDumpers " << name() << " '"
                     << wStringToString(m_dumperValue) << "' Type="
                     << m_dumperType << ' ' << DebugNodeFlags(flags());
    return testFlags(SimpleDumperOk);
}

std::wstring SymbolGroupNode::simpleDumpValue(const SymbolGroupValueContext &ctx)
{
    if (testFlags(Uninitialized))
        return L"<not in scope>";
    if (runSimpleDumpers(ctx))
        return m_dumperValue;
    return symbolGroupFixedValue();
}

int SymbolGroupNode::dump(std::ostream &str, const std::string &visitingFullIname,
                           const DumpParameters &p, const SymbolGroupValueContext &ctx)
{
    return dumpNode(str, name(), visitingFullIname, p, ctx);
}

// Return a watch expression basically as "*(type *)(address)"
static inline std::string watchExpression(ULONG64 address,
                                          const std::string &type,
                                          int /* kType */,
                                          const std::string &module)
{
    std::ostringstream str;
    str << "*(";
    // Try to make watch expressions faster by at least qualifying templates with
    // the local module. We cannot do expensive type lookup here.
    // We could insert a placeholder here for non-POD types indicating
    // that a type lookup should be done when inserting watches?
    if (!module.empty() && type.find('>') != std::string::npos)
        str << module << '!';
    str << SymbolGroupValue::pointerType(SymbolGroupValue::stripClassPrefixes(type)) << ')' << std::hex << std::showbase << address;
    return str.str();
}

int SymbolGroupNode::dumpNode(std::ostream &str,
                              const std::string &aName,
                              const std::string &aFullIName,
                              const DumpParameters &dumpParameters,
                              const SymbolGroupValueContext &ctx)
{
    const std::string t = type();
    const ULONG64 addr = address();
    SymbolGroupNode::dumpBasicData(str, aName, aFullIName, t,
                                   watchExpression(addr, t, m_dumperType, m_module));

    if (addr)
        str << ",addr=\"" << std::hex << std::showbase << addr << std::noshowbase << std::dec << '"';
    if (const ULONG s = size())
        str << ",size=\"" << s << '"';
    const bool uninitialized = flags() & Uninitialized;
    bool valueEditable = !uninitialized;
    bool valueEnabled = !uninitialized;

    // Shall it be recoded?
    std::wstring value = simpleDumpValue(ctx);
    int encoding = 0;
    if (dumpParameters.recode(t, aFullIName, ctx, &value, &encoding)) {
        str << ",valueencoded=\"" << encoding
            << "\",value=\"" << gdbmiWStringFormat(value) <<'"';
    } else { // As is: ASCII or base64?
        if (isSevenBitClean(value.c_str(), value.size())) {
            str << ",valueencoded=\"" << DumpEncodingAscii << "\",value=\""
                << gdbmiWStringFormat(value) << '"';
        } else {
            str << ",valueencoded=\"" << DumpEncodingBase64_Utf16 << "\",value=\"";
            base64Encode(str, reinterpret_cast<const unsigned char *>(value.c_str()), value.size() * sizeof(wchar_t));
            str << '"';
        }
    }
    // Children: Dump all known non-obscured or subelements
    unsigned childCountGuess = 0;
    if (!uninitialized) {
        if (m_dumperContainerSize > 0) {
            childCountGuess = m_dumperContainerSize; // See Obscured handling
        } else {
            if (children().empty()) {
                childCountGuess = m_parameters.SubElements; // Guess
            } else {
                childCountGuess = unsigned(children().size());
            }
        }
    }
    // No children..suppose we are editable and enabled.
    if (m_parameters.Flags & DEBUG_SYMBOL_READ_ONLY) {
        valueEditable = false;
    } else {
        if (childCountGuess != 0 && !(m_dumperType & KT_Editable))
            valueEditable = false;
    }
    str << ",valueenabled=\"" << (valueEnabled ? "true" : "false") << '"'
        << ",valueeditable=\"" << (valueEditable ? "true" : "false") << '"';
    return childCountGuess;
}

void SymbolGroupNode::debug(std::ostream &str,
                            const std::string &visitingFullIname,
                            unsigned verbosity, unsigned depth) const
{
    indentStream(str, depth);
    const std::string fullIname = absoluteFullIName();
    str << "AbsIname=" << fullIname << '"';
    if (fullIname != visitingFullIname)
        str << ",VisitIname=\"" <<visitingFullIname;
    str << "\",module=\"" << m_module << "\",index=" << m_index;
    if (const VectorIndexType childCount = children().size())
        str << ", Children=" << childCount;
    str << ' ' << m_parameters << DebugNodeFlags(flags());
    if (verbosity) {
        str << ",name=\"" << name() << "\", Address=0x" << std::hex << address() << std::dec
            << " Type=\"" << type() << '"';
        if (m_dumperType >= 0) {
            str << " ,dumperType=" << m_dumperType;
            if (m_dumperType & KT_Qt_Type)
                str << " qt";
            if (m_dumperType & KT_STL_Type)
                str << " STL";
            if (m_dumperType & KT_ContainerType)
                str << " container(" << m_dumperContainerSize << ')';
        }
        if (!testFlags(Uninitialized))
            str << " Value=\"" << gdbmiWStringFormat(symbolGroupRawValue()) << '"';
        str << '\n'; // Potentially multiline
    }
    str << '\n';
}

static inline std::string msgCannotCast(const std::string &nodeName,
                                        const std::string &fromType,
                                        const std::string &toType,
                                        const std::string &why)
{
    std::ostringstream str;
    str << "Cannot cast node '" << nodeName << "' from '" << fromType
        << "' to '" << toType << "': " << why;
    return str.str();
}

static std::string msgExpandFailed(const std::string &name, const std::string &iname,
                                   ULONG index, const std::string &why)
{
    std::ostringstream str;
    str << "Expansion of '" << name << "'/'" << iname << " (index: " << index
        << ") failed: " << why;
    return str.str();
}

// Expand!
bool SymbolGroupNode::expand(std::string *errorMessage)
{
    if (symbolGroupDebug)
        DebugPrint() << "SymbolGroupNode::expand "  << name()
                     <<'/' << absoluteFullIName() << ' '
                    << m_index << DebugNodeFlags(flags());
    if (isExpanded()) {
        // Clear the flag indication dumper expansion on a second, explicit request
        clearFlags(ExpandedByDumper);
        return true;
    }
    if (!canExpand()) {
        *errorMessage = msgExpandFailed(name(), absoluteFullIName(), m_index,
                                        "No subelements to expand in node.");
        return false;
    }
    if (flags() & Uninitialized) {
        *errorMessage = msgExpandFailed(name(), absoluteFullIName(), m_index,
                                        "Refusing to expand uninitialized node.");
        return false;
    }

    const HRESULT hr = m_symbolGroup->debugSymbolGroup()->ExpandSymbol(m_index, TRUE);

    if (FAILED(hr)) {
        *errorMessage = msgExpandFailed(name(), absoluteFullIName(), m_index, msgDebugEngineComFailed("ExpandSymbol", hr));
        ExtensionContext::instance().report('X', 0, 0, "Error", "%s", errorMessage->c_str());
        return false;
    }
    SymbolGroup::SymbolParameterVector parameters;
    // Retrieve parameters (including self, re-retrieve symbol parameters to get new 'expanded' flag
    // and corrected SubElement count (might be estimate))
    if (!SymbolGroup::getSymbolParameters(m_symbolGroup->debugSymbolGroup(),
                                          m_index, m_parameters.SubElements + 1,
                                          &parameters, errorMessage)) {
        *errorMessage = msgExpandFailed(name(), absoluteFullIName(), m_index, *errorMessage);
        return false;
    }
    // Before inserting children, correct indexes on whole group
    m_symbolGroup->root()->notifyIndexesMoved(m_index + 1, true, parameters.at(0).SubElements);
    // Parse parameters, correct our own) and create child nodes.
    parseParameters(m_index, m_index, parameters);
    return true;
}

bool SymbolGroupNode::expandRunComplexDumpers(const SymbolGroupValueContext &ctx,
                                              std::string *errorMessage)
{
    if (isExpanded() || testFlags(ComplexDumperOk))
        return true;
    if (!expand(errorMessage))
        return false;
    // Run simple dumpers to obtain type and run complex dumpers
    if (runSimpleDumpers(ctx) && testFlags(SimpleDumperOk))
        runComplexDumpers(ctx);
    return true;
}

bool SymbolGroupNode::typeCast(const std::string &desiredType, std::string *errorMessage)
{
    const std::string fromType = type();
    if (fromType == desiredType)
        return true;
    if (isExpanded()) {
        *errorMessage = msgCannotCast(absoluteFullIName(), fromType, desiredType, "Already expanded");
        return false;
    }
    HRESULT hr = m_symbolGroup->debugSymbolGroup()->OutputAsType(m_index, desiredType.c_str());
    if (FAILED(hr)) {
        *errorMessage = msgCannotCast(absoluteFullIName(), fromType, desiredType, msgDebugEngineComFailed("OutputAsType", hr));
        return false;
    }
    hr = m_symbolGroup->debugSymbolGroup()->GetSymbolParameters(m_index, 1, &m_parameters);
    if (FAILED(hr)) { // Should never fail
        *errorMessage = msgCannotCast(absoluteFullIName(), fromType, desiredType, msgDebugEngineComFailed("GetSymbolParameters", hr));
        return false;
    }
    return true;
}

// Find the index of the next symbol in the group, that is, right sibling.
// Go up the tree if we are that last sibling. 0 indicates none found (last sibling)
ULONG SymbolGroupNode::nextSymbolIndex() const
{
    const SymbolGroupNode *sParent = symbolGroupNodeParent();
    if (!sParent)
        return 0;
    const unsigned myIndex = sParent->indexOf(this);
    const AbstractSymbolGroupNodePtrVector &siblings = sParent->children();
    // Find any 'real' SymbolGroupNode to our right.
    const unsigned size = unsigned(siblings.size());
    for (unsigned i = myIndex + 1; i < size; ++i)
        if (const SymbolGroupNode *s = siblings.at(i)->asSymbolGroupNode())
            return s->index();
    return sParent->nextSymbolIndex();
}

// Remove self off parent and return the indexes to be shifted or unsigned(-1).
bool SymbolGroupNode::removeSelf(SymbolGroupNode *root, std::string *errorMessage)
{
    SymbolGroupNode *sParent = symbolGroupNodeParent();
    if (m_index == DEBUG_ANY_ID || !sParent) {
        *errorMessage = "SymbolGroupNode::removeSelf: Internal error 1.";
        return false;
    }
    const unsigned parentPosition = sParent->indexOf(this);
    if (parentPosition == unsigned(-1)) {
        *errorMessage = "SymbolGroupNode::removeSelf: Internal error 2.";
        return false;
    }
    // Determine the index for the following symbols to be subtracted by finding the next
    // 'real' SymbolGroupNode child (right or up the tree-right) and taking its index
    const ULONG oldIndex = index();
    const ULONG nextSymbolIdx = nextSymbolIndex();
    const ULONG offset = nextSymbolIdx >  oldIndex ? nextSymbolIdx - oldIndex : 0;
    if (SymbolGroupValue::verbose)
        DebugPrint() << "SymbolGroupNode::removeSelf #" << index() << " '" << name()
                     << "' at pos=" << parentPosition<< " (" << sParent->children().size()
                     << ") of parent '" << sParent->name()
                     << "' nextIndex=" << nextSymbolIdx << " offset=" << offset << *errorMessage;
    // Remove from symbol group
    const HRESULT hr = symbolGroup()->debugSymbolGroup()->RemoveSymbolByIndex(oldIndex);
    if (FAILED(hr)) {
        *errorMessage = msgDebugEngineComFailed("RemoveSymbolByIndex", hr);
        return false;
    }
    // Move the following symbol indexes: We no longer exist below here.
    sParent ->removeChildAt(parentPosition);
    if (offset)
        root->notifyIndexesMoved(oldIndex + 1, false, offset);
    return true;
}

static inline std::string msgCannotAddSymbol(const std::string &name, const std::string &why)
{
    std::ostringstream str;
    str << "Cannot add symbol '" << name << "': " << why;
    return str.str();
}

// For root nodes, only: Add a new symbol by name
SymbolGroupNode *SymbolGroupNode::addSymbolByName(const std::string &module,
                                                  const std::string &name,
                                                  const std::string &displayName,
                                                  const std::string &iname,
                                                  std::string *errorMessage)
{
    ULONG index = DEBUG_ANY_ID; // Append
    HRESULT hr = m_symbolGroup->debugSymbolGroup()->AddSymbol(name.c_str(), &index);
    if (FAILED(hr)) {
        *errorMessage = msgCannotAddSymbol(name, msgDebugEngineComFailed("AddSymbol", hr));
        ExtensionContext::instance().report('X', 0, 0, "Error", "%s", errorMessage->c_str());
        return 0;
    }
    if (index == DEBUG_ANY_ID) { // Occasionally happens for unknown or 'complicated' types
        *errorMessage = msgCannotAddSymbol(name, "DEBUG_ANY_ID was returned as symbol index by AddSymbol.");
        ExtensionContext::instance().report('X', 0, 0, "Error", "%s", errorMessage->c_str());
        return 0;
    }
    SymbolParameterVector parameters(1, DEBUG_SYMBOL_PARAMETERS());
    hr = m_symbolGroup->debugSymbolGroup()->GetSymbolParameters(index, 1, &(*parameters.begin()));
    if (FAILED(hr)) { // Should never fail
        std::ostringstream str;
        str << "Cannot retrieve 1 symbol parameter entry at " << index << ": "
            << msgDebugEngineComFailed("GetSymbolParameters", hr);
        *errorMessage = msgCannotAddSymbol(name, str.str());
        return 0;
    }
    // Paranoia: Check for cuckoo's eggs (which should not happen)
    if (parameters.front().ParentSymbol != m_index) {
        *errorMessage = msgCannotAddSymbol(name, "Parent id mismatch");
        return 0;
    }
    SymbolGroupNode *node = new SymbolGroupNode(m_symbolGroup, index,
                                                module,
                                                displayName.empty() ? name : displayName,
                                                iname.empty() ? name : iname);
    node->parseParameters(0, 0, parameters);
    node->addFlags(AdditionalSymbol);
    addChild(node);
    return node;
}

std::string SymbolGroupNode::msgAssignError(const std::string &nodeName,
                                            const std::string &value,
                                            const std::string &why)
{
    std::ostringstream str;
    str << "Unable to assign '" << value << "' to '" << nodeName << "': " << why;
    return str.str();
}

// Simple type
bool SymbolGroupNode::assign(const std::string &value, std::string *errorMessage /* = 0 */)
{
    const HRESULT hr =
        m_symbolGroup->debugSymbolGroup()->WriteSymbol(m_index, const_cast<char *>(value.c_str()));
    if (FAILED(hr)) {
        if (errorMessage)
            *errorMessage = SymbolGroupNode::msgAssignError(name(), value, msgDebugEngineComFailed("WriteSymbol", hr));
        return false;
    }
    return true;
}

// Utility returning a pair ('[42]','42') as name/iname pair
// for a node representing an array index
typedef std::pair<std::string, std::string> StringStringPair;

static inline StringStringPair arrayIndexNameIname(int index)
{
    StringStringPair rc(std::string(), toString(index));
    rc.first = std::string(1, '[');
    rc.first += rc.second;
    rc.first.push_back(']');
    return rc;
}

/*! \class ReferenceSymbolGroupNode

    Artificial node referencing another (real) SymbolGroupNode (added symbol or
    symbol from within an expanded linked list structure). Forwards the
    dumping to the referenced node using its own name.
    \ingroup qtcreatorcdbext */

ReferenceSymbolGroupNode::ReferenceSymbolGroupNode(const std::string &name,
                                                   const std::string &iname,
                                                   SymbolGroupNode *referencedNode) :
    AbstractSymbolGroupNode(name, iname), m_referencedNode(referencedNode)
{
}

// Convenience to create a node name name='[1]', iname='1' for arrays
ReferenceSymbolGroupNode *ReferenceSymbolGroupNode::createArrayNode(int index,
                                                                    SymbolGroupNode *referencedNode)
{
    const StringStringPair nameIname = arrayIndexNameIname(index);
    return new ReferenceSymbolGroupNode(nameIname.first, nameIname.second, referencedNode);
}

int ReferenceSymbolGroupNode::dump(std::ostream &str, const std::string &visitingFullIname,
                                    const DumpParameters &p, const SymbolGroupValueContext &ctx)
{
    // Let the referenced node dump with our iname/name
    return m_referencedNode->dumpNode(str, name(), visitingFullIname, p, ctx);
}

void ReferenceSymbolGroupNode::debug(std::ostream &str, const std::string &visitingFullIname,
                                     unsigned verbosity, unsigned depth) const
{
    indentStream(str, 2 * depth);
    str << "Node " << name() << '/' << visitingFullIname << " referencing\n";
    m_referencedNode->debug(str, visitingFullIname, verbosity, depth);
}

/*! \class MapNodeSymbolGroupNode

  \brief A [fake] map node with a fake array index and key/value entries consisting
         of ReferenceSymbolGroupNode.
  \ingroup qtcreatorcdbext
*/

MapNodeSymbolGroupNode::MapNodeSymbolGroupNode(const std::string &name,
                                               const std::string &iname,
                                               ULONG64 address,
                                               const std::string &type,
                                               AbstractSymbolGroupNode *key,
                                               AbstractSymbolGroupNode *value) :
    BaseSymbolGroupNode(name, iname), m_address(address), m_type(type)
{
    addChild(key);
    addChild(value);
}

MapNodeSymbolGroupNode
    *MapNodeSymbolGroupNode::create(int index, ULONG64 address,
                                    const std::string &type,
                                    SymbolGroupNode *key, SymbolGroupNode *value)
{
    const StringStringPair nameIname = arrayIndexNameIname(index);
    const std::string keyName = "key";
    ReferenceSymbolGroupNode *keyRN = new ReferenceSymbolGroupNode(keyName, keyName, key);
    const std::string valueName = "value";
    ReferenceSymbolGroupNode *valueRN = new ReferenceSymbolGroupNode(valueName, valueName, value);
    return new MapNodeSymbolGroupNode(nameIname.first, nameIname.second, address, type, keyRN, valueRN);
}

int MapNodeSymbolGroupNode::dump(std::ostream &str, const std::string &visitingFullIname,
                                  const DumpParameters &, const SymbolGroupValueContext &)
{
    SymbolGroupNode::dumpBasicData(str, name(), visitingFullIname);
    if (m_address)
        str << ",addr=\"0x" << std::hex << m_address << '"';
    str << ",type=\"" << m_type << "\",valueencoded=\"0\",value=\"\",valueenabled=\"false\""
           ",valueeditable=\"false\"";
    return 2;
}

void MapNodeSymbolGroupNode::debug(std::ostream &os, const std::string &visitingFullIname,
                                   unsigned /* verbosity */, unsigned depth) const
{
    indentStream(os, 2 * depth);
    os << "MapNode " << name() << '/' << visitingFullIname << '\n';
}

/*! \class SymbolGroupNodeVisitor

    Visitor that takes care of iterating over the nodes and
    building the full iname path ('local.foo.bar') that is required for
    GDBMI dumping. The full name depends on the path on which a node was reached
    for referenced nodes (a linked list element can be reached via array index
    or by expanding the whole structure).
    visit() is not called for the (invisible) root node, but starting with the
    root's children with depth=0.
    Return VisitStop from visit() to terminate the recursion.
    \ingroup qtcreatorcdbext
*/

// "local.vi" -> "local"
std::string SymbolGroupNodeVisitor::parentIname(const std::string &iname)
{
    const std::string::size_type lastSep = iname.rfind(SymbolGroupNodeVisitor::iNamePathSeparator);
    return lastSep == std::string::npos ? std::string() : iname.substr(0, lastSep);
}

/*! \class DebugSymbolGroupNodeVisitor
    \brief Debug output visitor.
    \ingroup qtcreatorcdbext
*/

DebugSymbolGroupNodeVisitor::DebugSymbolGroupNodeVisitor(std::ostream &os, unsigned verbosity) :
    m_os(os), m_verbosity(verbosity)
{
}

SymbolGroupNodeVisitor::VisitResult
    DebugSymbolGroupNodeVisitor::visit(AbstractSymbolGroupNode *node,
                                       const std::string &aFullIname,
                                       unsigned /* child */, unsigned depth)
{
    node->debug(m_os, aFullIname, m_verbosity, depth);
    return VisitContinue;
}

/*! \class DebugFilterSymbolGroupNodeVisitor
    \brief Debug filtering output visitor.
    \ingroup qtcreatorcdbext
*/

DebugFilterSymbolGroupNodeVisitor::DebugFilterSymbolGroupNodeVisitor(std::ostream &os,
                                                                     const std::string &filter,
                                                                     const unsigned verbosity) :
    DebugSymbolGroupNodeVisitor(os, verbosity), m_filter(filter)
{
}

SymbolGroupNodeVisitor::VisitResult
    DebugFilterSymbolGroupNodeVisitor::visit(AbstractSymbolGroupNode *node,
                                             const std::string &fullIname,
                                             unsigned child, unsigned depth)
{
    if (fullIname.find(m_filter) == std::string::npos
        && node->name().find(m_filter) == std::string::npos)
        return SymbolGroupNodeVisitor::VisitContinue;
    return DebugSymbolGroupNodeVisitor::visit(node, fullIname, child, depth);
}

/*! \class DumpSymbolGroupNodeVisitor

    GDBMI dump output visitor used to report locals values back to the
    debugging engine.  \ingroup qtcreatorcdbext
*/

DumpSymbolGroupNodeVisitor::DumpSymbolGroupNodeVisitor(std::ostream &os,
                                                       const SymbolGroupValueContext &context,
                                                       const DumpParameters &parameters) :
    m_os(os), m_context(context), m_parameters(parameters),
    m_lastDepth(unsigned(-1))
{
}

SymbolGroupNodeVisitor::VisitResult
    DumpSymbolGroupNodeVisitor::visit(AbstractSymbolGroupNode *node,
                                      const std::string &fullIname,
                                      unsigned /* child */, unsigned depth)
{
    // Show container children only, no additional symbol below root (unless it is a watch node).
    if (node->testFlags(SymbolGroupNode::Obscured))
        return VisitSkipChildren;
    if (node->testFlags(SymbolGroupNode::AdditionalSymbol) && !node->testFlags(SymbolGroupNode::WatchNode))
        return VisitSkipChildren;
    // Recurse to children only if expanded by explicit watchmodel request
    // and initialized.
    bool visitChildren = depth < 1; // Report only one level for Qt Creator.
    // Visit children of a SymbolGroupNode only if not expanded by its dumpers.
    if (visitChildren)
        if (const SymbolGroupNode *realNode = node->resolveReference()->asSymbolGroupNode())
            if (!realNode->isExpanded() || realNode->testFlags(SymbolGroupNode::Uninitialized|SymbolGroupNode::ExpandedByDumper))
                    visitChildren = false;
    // Comma between same level children given obscured children
    if (depth == m_lastDepth) {
        m_os << ',';
    } else {
        m_lastDepth = depth;
    }
    if (m_parameters.humanReadable()) {
        m_os << '\n';
        indentStream(m_os, depth * 2);
    }
    m_os << '{';
    const int childCount = node->dump(m_os, fullIname, m_parameters, m_context);
    m_os << ",numchild=\"" << childCount << '"';
    if (!childCount)
        visitChildren = false;
    if (visitChildren) { // open children array
        m_os << ",children=[";
    } else {               // No children, close array.
        m_os << '}';
    }
    if (m_parameters.humanReadable())
        m_os << '\n';
    return visitChildren ? VisitContinue : VisitSkipChildren;
}

void DumpSymbolGroupNodeVisitor::childrenVisited(const AbstractSymbolGroupNode *n, unsigned)
{
    m_os << "]}"; // Close children array and self
    if (m_parameters.humanReadable())
        m_os << "   /* end of '" << n->absoluteFullIName() << "' */\n";
}
