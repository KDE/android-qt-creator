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

#include "containers.h"
#include "symbolgroupvalue.h"
#include "symbolgroup.h"
#include "stringutils.h"

#include <functional>
#include <iterator>

typedef AbstractSymbolGroupNode::AbstractSymbolGroupNodePtrVector AbstractSymbolGroupNodePtrVector;
typedef std::vector<SymbolGroupValue> SymbolGroupValueVector;
typedef std::vector<int>::size_type VectorIndexType;

// Read a pointer array from debuggee memory (ULONG64/32 according pointer size)
static void *readPointerArray(ULONG64 address, unsigned count, const SymbolGroupValueContext &ctx)
{
    const unsigned pointerSize = SymbolGroupValue::pointerSize();
    const ULONG allocSize = pointerSize * count;
    ULONG bytesRead = 0;
    void *data = new unsigned char[allocSize];
    const HRESULT hr = ctx.dataspaces->ReadVirtual(address, data, allocSize, &bytesRead);
    if (FAILED(hr) || bytesRead != allocSize) {
        delete [] data;
        return 0;
    }
    return data;
}

template <class UInt>
inline void dumpHexArray(std::ostream &os, const UInt *a, int count)
{
    os << std::showbase << std::hex;
    std::copy(a, a + count, std::ostream_iterator<UInt>(os, ", "));
    os << std::noshowbase << std::dec;
}

static inline void dump32bitPointerArray(std::ostream &os, const void *a, int count)
{
    dumpHexArray(os, reinterpret_cast<const ULONG32 *>(a), count);
}

static inline void dump64bitPointerArray(std::ostream &os, const void *a, int count)
{
    dumpHexArray(os, reinterpret_cast<const ULONG64 *>(a), count);
}

// Return size from an STL vector (last/first iterators).
static inline int msvcStdVectorSize(const SymbolGroupValue &v)
{
    if (const SymbolGroupValue myFirstPtrV = v["_Myfirst"]) {
        if (const SymbolGroupValue myLastPtrV = v["_Mylast"]) {
            const ULONG64 firstPtr = myFirstPtrV.pointerValue();
            const ULONG64 lastPtr = myLastPtrV.pointerValue();
            if (!firstPtr || lastPtr < firstPtr)
                return -1;
            if (lastPtr == firstPtr)
                return 0;
            // Subtract the pointers: We need to do the pointer arithmetics ourselves
            // as we get char *pointers.
            const std::string innerType = SymbolGroupValue::stripPointerType(myFirstPtrV.type());
            const size_t size = SymbolGroupValue::sizeOf(innerType.c_str());
            if (size == 0)
                return -1;
            return static_cast<int>((lastPtr - firstPtr) / size);
        }
    }
    return -1;
}

// Return size of container or -1
int containerSize(KnownType kt, SymbolGroupNode *n, const SymbolGroupValueContext &ctx)
{
    QTC_TRACE_IN
    if ((kt & KT_ContainerType) == 0)
        return -1;
    const int ct = containerSize(kt, SymbolGroupValue(n, ctx));
    QTC_TRACE_OUT
    return ct;
}

// Determine size of containers
int containerSize(KnownType kt, const SymbolGroupValue &v)
{
    switch (kt) {
    case KT_QStringList:
        if (const SymbolGroupValue base = v[unsigned(0)])
            return containerSize(KT_QList, base);
        break;
    case KT_QList:
        if (const SymbolGroupValue dV = v["d"]) {
            if (const SymbolGroupValue beginV = dV["begin"]) {
                const int begin = beginV.intValue();
                const int end = dV["end"].intValue();
                if (begin >= 0 && end >= begin)
                    return end - begin;
            }
        }
        break;
    case KT_QLinkedList:
    case KT_QHash:
    case KT_QMap:
    case KT_QVector:
        if (const SymbolGroupValue sizeV = v["d"]["size"])
            return sizeV.intValue();
        break;
    case KT_QMultiHash:
        if (const SymbolGroupValue qHash = v[unsigned(0)])
            return containerSize(KT_QHash, qHash);
        break;
    case KT_QQueue:
        if (const SymbolGroupValue qList= v[unsigned(0)])
            return containerSize(KT_QList, qList);
        break;
    case KT_QStack:
        if (const SymbolGroupValue qVector = v[unsigned(0)])
            return containerSize(KT_QVector, qVector);
        break;
    case KT_QSet:
        if (const SymbolGroupValue base = v[unsigned(0)])
            return containerSize(KT_QHash, base);
        break;
    case KT_QMultiMap:
        if (const SymbolGroupValue base = v[unsigned(0)])
            return containerSize(KT_QMap, base);
        break;
    case KT_StdVector: {
        if (const SymbolGroupValue base = v[unsigned(0)]) {
            const int msvc10Size = msvcStdVectorSize(base);
            if (msvc10Size >= 0)
                return msvc10Size;
        }
        const int msvc8Size = msvcStdVectorSize(v);
        if (msvc8Size >= 0)
            return msvc8Size;
    }
        break;
    case KT_StdList:
        if (const SymbolGroupValue sizeV =  v["_Mysize"]) // VS 8
            return sizeV.intValue();
        if (const SymbolGroupValue sizeV = v[unsigned(0)][unsigned(0)]["_Mysize"]) // VS10
            return sizeV.intValue();
        break;
    case KT_StdDeque: {
        const SymbolGroupValue msvc10sizeV =  v[unsigned(0)]["_Mysize"]; // VS10
        if (msvc10sizeV)
            return msvc10sizeV.intValue();
        const SymbolGroupValue msvc8sizeV =  v["_Mysize"]; // VS8
        if (msvc8sizeV)
            return msvc8sizeV.intValue();
    }
        break;
    case KT_StdStack:
        if (const SymbolGroupValue deque =  v[unsigned(0)])
            return containerSize(KT_StdDeque, deque);
        break;
    case KT_StdSet:
    case KT_StdMap:
    case KT_StdMultiMap:
        if (const SymbolGroupValue baseV = v[unsigned(0)]) {
            if (const SymbolGroupValue sizeV = baseV["_Mysize"]) // VS 8
                return sizeV.intValue();
            if (const SymbolGroupValue sizeV = baseV[unsigned(0)][unsigned(0)]["_Mysize"]) // VS 10
                return sizeV.intValue();
        }
        break;
    }
    return -1;
}

/* Generate a list of children by invoking the functions to obtain the value
 * and the next link */
template <class ValueFunction, class NextFunction>
AbstractSymbolGroupNodePtrVector linkedListChildList(SymbolGroupValue headNode,
                                                     int count,
                                                     ValueFunction valueFunc,
                                                     NextFunction nextFunc)
{
    AbstractSymbolGroupNodePtrVector rc;
    rc.reserve(count);
    for (int i =0; i < count && headNode; i++) {
        if (const SymbolGroupValue value = valueFunc(headNode)) {
            rc.push_back(ReferenceSymbolGroupNode::createArrayNode(i, value.node()));
            headNode = nextFunc(headNode);
        } else {
            break;
        }
    }
    return rc;
}

// Helper function for linkedListChildList that returns a member by name
class MemberByName : public std::unary_function<const SymbolGroupValue &, SymbolGroupValue>
{
public:
    explicit MemberByName(const char *name) : m_name(name) {}
    SymbolGroupValue operator()(const SymbolGroupValue &v) { return v[m_name]; }

private:
    const char *m_name;
};

// std::list<T>: Dummy head node and then a linked list of "_Next", "_Myval".
static inline AbstractSymbolGroupNodePtrVector stdListChildList(SymbolGroupNode *n, int count,
                                                        const SymbolGroupValueContext &ctx)
{
    if (count)
        if (const SymbolGroupValue head = SymbolGroupValue(n, ctx)[unsigned(0)][unsigned(0)]["_Myhead"]["_Next"])
            return linkedListChildList(head, count, MemberByName("_Myval"), MemberByName("_Next"));
    return AbstractSymbolGroupNodePtrVector();
}

// QLinkedList<T>: Dummy head node and then a linked list of "n", "t".
static inline AbstractSymbolGroupNodePtrVector qLinkedListChildList(SymbolGroupNode *n, int count,
                                                        const SymbolGroupValueContext &ctx)
{
    if (count)
        if (const SymbolGroupValue head = SymbolGroupValue(n, ctx)["e"]["n"])
            return linkedListChildList(head, count, MemberByName("t"), MemberByName("n"));
    return AbstractSymbolGroupNodePtrVector();
}

// Symbol Name/(Expression) of a pointed-to instance ('Foo' at 0x10') ==> '*(Foo *)0x10'
static inline std::string pointedToSymbolName(ULONG64 address, const std::string &type)
{
    std::ostringstream str;
    str << "*(" << type;
    if (!endsWith(type, '*'))
        str << ' ';
    str << "*)" << std::showbase << std::hex << address;
    return str.str();
}

/* Helper for array-type containers:
 * Add a series of "*(innertype *)0x (address + n * size)" fake child symbols.
 * for a function generating a sequence of addresses. */

template <class AddressFunc>
AbstractSymbolGroupNodePtrVector arrayChildList(SymbolGroup *sg, AddressFunc addressFunc,
                                        const std::string &innerType, int count)
{
    AbstractSymbolGroupNodePtrVector rc;
    if (!count)
        return rc;
    std::string errorMessage;
    rc.reserve(count);
    for (int i = 0; i < count; i++) {
        const std::string name = pointedToSymbolName(addressFunc(), innerType);
        if (SymbolGroupNode *child = sg->addSymbol(name, std::string(), &errorMessage)) {
            rc.push_back(ReferenceSymbolGroupNode::createArrayNode(i, child));
        } else {
            break;
        }
    }
    return rc;
}

// Helper function for arrayChildList() taking a reference to an address and simply generating
// a sequence of address, address + delta, address + 2 * delta...
class AddressSequence
{
public:
    explicit inline AddressSequence(ULONG64 &address, ULONG delta) : m_address(address), m_delta(delta) {}
    inline ULONG64 operator()()
    {
        const ULONG64 rc = m_address;
        m_address += m_delta;
        return rc;
    }

private:
    ULONG64 &m_address;
    const ULONG m_delta;
};

static inline AbstractSymbolGroupNodePtrVector arrayChildList(SymbolGroup *sg, ULONG64 address,
                                               const std::string &innerType, int count)
{
    if (const unsigned innerTypeSize = SymbolGroupValue::sizeOf(innerType.c_str()))
        return arrayChildList(sg, AddressSequence(address, innerTypeSize),
                              innerType, count);
    return AbstractSymbolGroupNodePtrVector();
}

// std::vector<T>
static inline AbstractSymbolGroupNodePtrVector
    stdVectorChildList(SymbolGroupNode *n, int count, const SymbolGroupValueContext &ctx)
{
    if (count) {
        // std::vector<T>: _Myfirst is a pointer of T*. Get address
        // element to obtain address.
        const SymbolGroupValue vec(n, ctx);
        SymbolGroupValue myFirst = vec[unsigned(0)]["_Myfirst"]; // MSVC2010
        if (!myFirst)
            myFirst = vec["_Myfirst"]; // MSVC2008
        if (myFirst)
            if (const ULONG64 address = myFirst.pointerValue())
                return arrayChildList(n->symbolGroup(), address,
                                      SymbolGroupValue::stripPointerType(myFirst.type()), count);
    }
    return AbstractSymbolGroupNodePtrVector();
}

// Helper for std::deque<>: From the array of deque blocks, read out the values.
template<class AddressType>
AbstractSymbolGroupNodePtrVector
    stdDequeChildrenHelper(SymbolGroup *sg,
                           const AddressType *blockArray, ULONG64 blockArraySize,
                           const std::string &innerType, ULONG64 innerTypeSize,
                           ULONG64 startOffset, ULONG64 dequeSize, int count)
{
    AbstractSymbolGroupNodePtrVector rc;
    rc.reserve(count);
    std::string errorMessage;
    // Determine block number and offset in the block array T[][dequeSize]
    // and create symbol by address.
    for (int i = 0; i < count; i++) {
        // see <deque>-header: std::deque<T>::iterator::operator*
        const ULONG64 offset = startOffset + i;
        ULONG64 block = offset / dequeSize;
        if (block >= blockArraySize)
            block -= blockArraySize;
        const ULONG64 blockOffset = offset % dequeSize;
        const ULONG64 address = blockArray[block] + innerTypeSize * blockOffset;
        if (SymbolGroupNode *n = sg->addSymbol(pointedToSymbolName(address, innerType), std::string(), &errorMessage)) {
            rc.push_back(ReferenceSymbolGroupNode::createArrayNode(i, n));
        } else {
            return AbstractSymbolGroupNodePtrVector();
        }
    }
    return rc;
}

// std::deque<>
static inline AbstractSymbolGroupNodePtrVector
    stdDequeDirectChildList(const SymbolGroupValue &deque, int count)
{
    if (!count)
        return AbstractSymbolGroupNodePtrVector();
    // From MSVC10 on, there is an additional base class
    const ULONG64 arrayAddress = deque["_Map"].pointerValue();
    const int startOffset = deque["_Myoff"].intValue();
    const int mapSize  = deque["_Mapsize"].intValue();
    if (!arrayAddress || startOffset < 0 || mapSize <= 0)
        return AbstractSymbolGroupNodePtrVector();
    const std::vector<std::string> innerTypes = deque.innerTypes();
    if (innerTypes.empty())
        return AbstractSymbolGroupNodePtrVector();
    // Get the deque size (block size) which is an unavailable static member
    // (cf <deque> for the actual expression).
    const unsigned innerTypeSize = SymbolGroupValue::sizeOf(innerTypes.front().c_str());
    if (!innerTypeSize)
        return AbstractSymbolGroupNodePtrVector();
    const int dequeSize = innerTypeSize <= 1 ? 16 : innerTypeSize <= 2 ?
                               8 : innerTypeSize <= 4 ? 4 : innerTypeSize <= 8 ? 2 : 1;
    // Read out map array (pointing to the blocks)
    void *mapArray = readPointerArray(arrayAddress, mapSize, deque.context());
    if (!mapArray)
        return AbstractSymbolGroupNodePtrVector();
    const AbstractSymbolGroupNodePtrVector rc = SymbolGroupValue::pointerSize() == 8 ?
        stdDequeChildrenHelper(deque.node()->symbolGroup(),
                               reinterpret_cast<const ULONG64 *>(mapArray), mapSize,
                               innerTypes.front(), innerTypeSize, startOffset, dequeSize, count) :
        stdDequeChildrenHelper(deque.node()->symbolGroup(),
                               reinterpret_cast<const ULONG32 *>(mapArray), mapSize,
                               innerTypes.front(), innerTypeSize, startOffset, dequeSize, count);
    delete [] mapArray;
    return rc;
}

// std::deque<>
static inline AbstractSymbolGroupNodePtrVector
    stdDequeChildList(const SymbolGroupValue &v, int count)
{
    // MSVC10 has a base class. If that fails, try direct (MSVC2008)
    const AbstractSymbolGroupNodePtrVector msvc10rc = stdDequeDirectChildList(v[unsigned(0)], count);
    return msvc10rc.empty() ? stdDequeDirectChildList(v, count) : msvc10rc;
}

/* Helper class for std::map<>,std::set<> based on std::__Tree:
 * We locally rebuild the structure in using instances of below class 'StdMapNode'
 * with 'left' and 'right' pointers and the values. Reason being that while it is
 * possible to write the iteration in terms of class SymbolGroupValue, it involves
 * going back up the tree over the flat node->parent pointers. Doing that in the debugger
 * sometimes ends up in nirvana, apparently due to it not being able to properly expand it.
 * StdMapNode has a buildMap() to build a hierarchy from a __Tree value,
 * begin() to return the first node and next() to iterate. The latter are modeled
 * after the _Tree::iterator base classes. (_Tree::begin, _Tree::iterator::operator++() */

class StdMapNode
{
private:
    StdMapNode(const StdMapNode &);
    StdMapNode &operator=(const StdMapNode &);

public:
    explicit StdMapNode(StdMapNode *p, const SymbolGroupValue &node, const SymbolGroupValue &value);
    ~StdMapNode() { delete m_left; delete m_right; }

    // Iterator helpers: Return first and move to next
    const StdMapNode *begin() const { return StdMapNode::leftMost(this); }
    static const StdMapNode *next(const StdMapNode *s);

    const SymbolGroupValue &value() const { return m_value; }

    // Build the hierarchy
    static StdMapNode *buildMap(const SymbolGroupValue &n);

    // Debug helpers
    void debug(std::ostream &os, unsigned depth = 0) const;

private:
    static StdMapNode *buildMapRecursion(const SymbolGroupValue &n, ULONG64 headAddress, StdMapNode *parent);
    static const StdMapNode *leftMost(const StdMapNode *n);

    StdMapNode *const m_parent;
    StdMapNode *m_left;
    StdMapNode *m_right;
    const SymbolGroupValue m_node;
    const SymbolGroupValue m_value;
};

StdMapNode::StdMapNode(StdMapNode *p, const SymbolGroupValue &n, const SymbolGroupValue &v) :
    m_parent(p), m_left(0), m_right(0), m_node(n), m_value(v)
{
}

const StdMapNode *StdMapNode::leftMost(const StdMapNode *n)
{
    for ( ; n->m_left ; n = n->m_left ) ;
    return n;
}

const StdMapNode *StdMapNode::next(const StdMapNode *s)
{
    if (s->m_right) // If we have a right node, return its left-most
        return StdMapNode::leftMost(s->m_right);
    do { // Climb looking for 'right' subtree, that is, we are left of it
        StdMapNode *parent = s->m_parent;
        if (!parent || parent->m_right != s)
            return parent;
        s = parent;
    } while (true);
    return 0;
}

StdMapNode *StdMapNode::buildMapRecursion(const SymbolGroupValue &n, ULONG64 headAddress, StdMapNode *parent)
{
    const SymbolGroupValue value = n["_Myval"];
    if (!value)
        return 0;
    StdMapNode *node = new StdMapNode(parent, n, value);
    // Get left and right nodes. A node pointing to head terminates the recursion
    if (const SymbolGroupValue left = n["_Left"])
        if (const ULONG64 leftAddr = left.pointerValue())
            if (leftAddr != headAddress)
                node->m_left = buildMapRecursion(left, headAddress, node);
    if (const SymbolGroupValue right = n["_Right"])
        if (const ULONG64 rightAddr = right.pointerValue())
            if (rightAddr != headAddress)
                node->m_right = buildMapRecursion(right, headAddress, node);
    return node;
}

StdMapNode *StdMapNode::buildMap(const SymbolGroupValue &n)
{
    // Goto root of tree (see _Tree::_Root())
    if (const SymbolGroupValue head = n["_Myhead"])
        if (const ULONG64 headAddress = head.pointerValue())
            return buildMapRecursion(head["_Parent"], headAddress, 0);
    return 0;
}

static inline void indentStream(std::ostream &os, unsigned indent)
{
    for (unsigned i = 0; i < indent; i++)
        os << ' ';
}

// Debugging helper for a SymbolGroupValue containing a __Tree::node of
// a map (assuming a std::pair inside).
static inline void debugMSVC2010MapNode(const SymbolGroupValue &n, std::ostream &os, unsigned indent = 0)
{
    indentStream(os, indent);
    os << "Node at " << std::hex << std::showbase << n.address()
       << std::dec << std::noshowbase
       << " Value='" << wStringToString(n.value()) << "', Parent=" << wStringToString(n["_Parent"].value())
       << ", Left=" << wStringToString(n["_Left"].value())
       << ", Right=" << wStringToString(n["_Right"].value())
       << ", nil='" <<  wStringToString(n["_Isnil"].value());
    if (const SymbolGroupValue pairBase = n["_Myval"][unsigned(0)]) {
        os << "', key='"  << wStringToString(pairBase["first"].value())
           << "', value='"   << wStringToString(pairBase["second"].value())
           << '\'';
    } else {
        os << "', key='"  << wStringToString(n["_Myval"].value()) << '\'';
    }
    os << '\n';
}

void StdMapNode::debug(std::ostream &os, unsigned depth) const
{
    indentStream(os, 2 * depth);
    os << "StdNode=" << this << " Left=" << m_left  << " Right=" << m_right << '\n';
    debugMSVC2010MapNode(m_node, os, 2 * depth);
    if (m_left)
        m_left->debug(os, depth + 1);
    if (m_right)
        m_right->debug(os, depth + 1);
}

// Helper for std::map<>,std::set<> based on std::__Tree:
// Return the list of children (pair for maps, direct children for set)
static inline SymbolGroupValueVector
    stdTreeChildList(const SymbolGroupValue &tree, int count, bool *isMSVC2010In = 0)
{
    if (!count)
        return SymbolGroupValueVector();
    // MSVC2010: "class _Tree : public _Tree_val: public _Tree_nod".
    // MSVC2008: Direct class
    const bool isMSVC2010 = tree[unsigned(0)][unsigned(0)]["_Mysize"].intValue() == count;
    if (isMSVC2010In)
        *isMSVC2010In = isMSVC2010;
    const SymbolGroupValue treeNode = isMSVC2010 ? tree[unsigned(0)][unsigned(0)] : tree;
    if (!treeNode)
        return SymbolGroupValueVector();
    // Build the tree and iterate it.
    const StdMapNode *nodeTree = StdMapNode::buildMap(treeNode);
    if (!nodeTree)
        return SymbolGroupValueVector();
    SymbolGroupValueVector rc;
    rc.reserve(count);
    int i = 0;
    for (const StdMapNode *n = nodeTree->begin() ; n && i < count; n = StdMapNode::next(n), i++)
        rc.push_back(n->value());
    delete nodeTree;
    if (rc.size() != count)
        return SymbolGroupValueVector();
    return rc;
}

// std::set<>: Children directly contained in list
static inline AbstractSymbolGroupNodePtrVector
    stdSetChildList(const SymbolGroupValue &set, int count)
{
    const SymbolGroupValueVector children = stdTreeChildList(set[unsigned(0)], count);
    if (int(children.size()) != count)
        return AbstractSymbolGroupNodePtrVector();
    AbstractSymbolGroupNodePtrVector rc;
    rc.reserve(count);
    for (int i = 0; i < count; i++)
        rc.push_back(ReferenceSymbolGroupNode::createArrayNode(i, children.at(i).node()));
    return rc;
}

// std::map<K,V>: A list of std::pair<K,V> (derived from std::pair_base<K,V>)
static inline AbstractSymbolGroupNodePtrVector
    stdMapChildList(const SymbolGroupValue &map, int count)
{
    bool isMSVC2010 = true;
    const SymbolGroupValueVector children = stdTreeChildList(map[unsigned(0)], count, &isMSVC2010);
    if (int(children.size()) != count)
        return AbstractSymbolGroupNodePtrVector();
    AbstractSymbolGroupNodePtrVector rc;
    rc.reserve(count);
    for (int i = 0; i < count; i++) {
        // MSVC2010 introduces a std::pair_base.
        const SymbolGroupValue pairBase = isMSVC2010?
                    children.at(i)[unsigned(0)] : children.at(i);
        const SymbolGroupValue key = pairBase["first"];
        const SymbolGroupValue value = pairBase["second"];
        if (key && value) {
            rc.push_back(MapNodeSymbolGroupNode::create(i, pairBase.address(),
                                                        pairBase.type(),
                                                        key.node(), value.node()));
        } else {
            return AbstractSymbolGroupNodePtrVector();
        }
    }
    return rc;
}

// QVector<T>
static inline AbstractSymbolGroupNodePtrVector
    qVectorChildList(SymbolGroupNode *n, int count, const SymbolGroupValueContext &ctx)
{
    if (count) {
        // QVector<T>: p/array is declared as array of T. Dereference first
        // element to obtain address.
        const SymbolGroupValue vec(n, ctx);
        if (const SymbolGroupValue firstElementV = vec["p"]["array"][unsigned(0)])
            if (const ULONG64 arrayAddress = firstElementV.address())
                    return arrayChildList(n->symbolGroup(), arrayAddress, firstElementV.type(), count);
    }
    return AbstractSymbolGroupNodePtrVector();
}

// Helper function for arrayChildList() for use with QLists of large types that are an
// array of pointers to allocated elements: Generate a pointer sequence by reading out the array.
template <class AddressType>
class AddressArraySequence
{
public:
    explicit inline AddressArraySequence(const AddressType *array) : m_array(array) {}
    inline ULONG64 operator()() { return *m_array++; }

private:
    const AddressType *m_array;
};

// QList<>.
static inline AbstractSymbolGroupNodePtrVector
    qListChildList(const SymbolGroupValue &v, int count)
{
    // QList<T>: d/array is declared as array of void *[]. Dereference first
    // element to obtain address.
    if (!count)
        return AbstractSymbolGroupNodePtrVector();
    const SymbolGroupValue dV = v["d"];
    if (!dV)
        return AbstractSymbolGroupNodePtrVector();
    const int begin = dV["begin"].intValue();
    if (begin < 0)
        return AbstractSymbolGroupNodePtrVector();
    const SymbolGroupValue firstElementV = dV["array"][unsigned(0)];
    if (!firstElementV)
        return AbstractSymbolGroupNodePtrVector();
     ULONG64 arrayAddress = firstElementV.address();
     if (!arrayAddress)
         return AbstractSymbolGroupNodePtrVector();
     const std::vector<std::string> innerTypes = v.innerTypes();
     if (innerTypes.size() != 1)
         return AbstractSymbolGroupNodePtrVector();
     const std::string &innerType = innerTypes.front();
     const unsigned innerTypeSize = SymbolGroupValue::sizeOf(innerType.c_str());
     if (!innerTypeSize)
         return AbstractSymbolGroupNodePtrVector();
     /* QList<> is:
      * 1) An array of 'void *[]' where T values are coerced into the elements for
      *    POD/pointer types and small, movable or primitive Qt types. That is, smaller
      *    elements are also aligned at 'void *' boundaries.
      * 2) An array of 'T *[]' (pointer to allocated instances) for anything else
      *    (QTypeInfo<T>::isLarge || QTypeInfo<T>::isStatic)
      *    isStatic depends on QTypeInfo specializations and hardcoded flags for types. */
     const unsigned pointerSize = SymbolGroupValue::pointerSize();
     arrayAddress += begin * pointerSize;
     if (SymbolGroupValue::isPointerType(innerType)) // Quick check: Any pointer is T[]
         return arrayChildList(v.node()->symbolGroup(),
                               AddressSequence(arrayAddress, pointerSize),
                               innerType, count);
     // Check condition for large||static.
     bool isLargeOrStatic = innerTypeSize > pointerSize;
     if (!isLargeOrStatic) {
         const KnownType kt = knownType(innerType, false); // inner type, no 'class ' prefix.
         if (kt != KT_Unknown && !(knownType(innerType, false) & (KT_Qt_PrimitiveType|KT_Qt_MovableType)))
             isLargeOrStatic = true;
     }
     if (isLargeOrStatic) {
         // Retrieve the pointer array ourselves to avoid having to evaluate '*(class foo**)'
         if (void *data = readPointerArray(arrayAddress, count, v.context()))  {
             // Generate sequence of addresses from pointer array
             const AbstractSymbolGroupNodePtrVector rc = pointerSize == 8 ?
                         arrayChildList(v.node()->symbolGroup(), AddressArraySequence<ULONG64>(reinterpret_cast<const ULONG64 *>(data)), innerType, count) :
                         arrayChildList(v.node()->symbolGroup(), AddressArraySequence<ULONG32>(reinterpret_cast<const ULONG32 *>(data)), innerType, count);
             delete [] data;
             return rc;
         }
         return AbstractSymbolGroupNodePtrVector();
     }
     return arrayChildList(v.node()->symbolGroup(),
                           AddressSequence(arrayAddress, pointerSize),
                           innerType, count);
}

// Return the list of buckets of a 'QHash<>' as 'QHashData::Node *' values from
// the list of addresses passed in
template<class AddressType>
SymbolGroupValueVector hashBuckets(SymbolGroup *sg, const std::string hashNodeType,
                                   const AddressType *pointerArray,
                                   int numBuckets,
                                   AddressType ePtr,
                                   const SymbolGroupValueContext &ctx)
{
    SymbolGroupValueVector rc;
    rc.reserve(numBuckets);
    const AddressType *end = pointerArray + numBuckets;
    std::string errorMessage;
    // Skip 'e' special values as they are used as placeholder for reserve(d)
    // empty array elements.
    for (const AddressType *p = pointerArray; p < end; p++) {
        if (*p != ePtr) {
            const std::string name = pointedToSymbolName(*p, hashNodeType);
            if (SymbolGroupNode *child = sg->addSymbol(name, std::string(), &errorMessage)) {
                rc.push_back(SymbolGroupValue(child, ctx));
            } else {
                return std::vector<SymbolGroupValue>();
                break;
            }
        }
    }
    return rc;
}

// Return the node type of a QHash/QMap:
// "class QHash<K,V>" -> [struct] "QtCored4!QHashNode<K,V>";
static inline std::string qHashNodeType(const SymbolGroupValue &v,
                                        const char *nodeType)
{
    std::string qHashType = v.type();
    const std::string::size_type pos = qHashType.find('<');
    if (pos != std::string::npos)
        qHashType.insert(pos, nodeType);
    // A map node must be qualified with the current module and
    // the Qt namespace (particularly QMapNode, QHashNodes work also for
    // the unqualified case).
    const QtInfo &qtInfo = QtInfo::get(v.context());
    const std::string currentModule = v.node()->symbolGroup()->module();
    return QtInfo::prependModuleAndNameSpace(qHashType, currentModule, qtInfo.nameSpace);
}

enum { debugMap  = 0 };

// Return up to count nodes of type "QHashNode<K,V>" of a "class QHash<K,V>".
SymbolGroupValueVector qHashNodes(const SymbolGroupValue &v,
                                  VectorIndexType count)
{
    if (!count)
        return SymbolGroupValueVector();
    const SymbolGroupValue hashData = v["d"];
    // 'e' is used as a special value to indicate empty hash buckets in the array.
    const ULONG64 ePtr = v["e"].pointerValue();
    if (debugMap)
        DebugPrint() << v << " Count=" << count << ",ePtr=0x" << std::hex << ePtr;
    if (!hashData || !ePtr)
        return SymbolGroupValueVector();
    // Retrieve the array of buckets of 'd'
    const int numBuckets = hashData["numBuckets"].intValue();
    const ULONG64 bucketArray = hashData["buckets"].pointerValue();
    if (numBuckets <= 0 || !bucketArray)
        return SymbolGroupValueVector();
    void *bucketPointers = readPointerArray(bucketArray, numBuckets, v.context());
    if (!bucketPointers)
        return SymbolGroupValueVector();
    // Get list of buckets (starting elements of 'QHashData::Node')
    const std::string dummyNodeType = QtInfo::get(v.context()).prependQtCoreModule("QHashData::Node");
    const SymbolGroupValueVector buckets = SymbolGroupValue::pointerSize() == 8 ?
        hashBuckets(v.node()->symbolGroup(), dummyNodeType,
                    reinterpret_cast<const ULONG64 *>(bucketPointers), numBuckets,
                    ePtr, v.context()) :
        hashBuckets(v.node()->symbolGroup(), dummyNodeType,
                    reinterpret_cast<const ULONG32 *>(bucketPointers), numBuckets,
                    ULONG32(ePtr), v.context());
    delete [] bucketPointers ;
    // Generate the list 'QHashData::Node *' by iterating over the linked list of
    // nodes starting at each bucket. Using the 'QHashData::Node *' instead of
    // the 'QHashNode<K,T>' is much faster. Each list has a trailing, unused
    // dummy element.
    SymbolGroupValueVector dummyNodeList;
    dummyNodeList.reserve(count);
    bool notEnough = true;
    const SymbolGroupValueVector::const_iterator ncend = buckets.end();
    for (SymbolGroupValueVector::const_iterator it = buckets.begin(); notEnough && it != ncend; ++it) {
        for (SymbolGroupValue l = *it; notEnough && l ; ) {
            const SymbolGroupValue next = l["next"];
            if (next && next.pointerValue()) { // Stop at trailing dummy element
                dummyNodeList.push_back(l);
                if (dummyNodeList.size() >= count) // Stop at maximum count
                    notEnough = false;
                if (debugMap)
                    DebugPrint() << '#' << (dummyNodeList.size() - 1) << "l=" << l << ",next=" << next;
                l = next;
            } else {
                break;
            }
        }
    }
    // Finally convert them into real nodes 'QHashNode<K,V> (potentially expensive)
    const std::string nodeType = qHashNodeType(v, "Node");
    if (debugMap)
        DebugPrint() << "Converting into " << nodeType;
    SymbolGroupValueVector nodeList;
    nodeList.reserve(count);
    const SymbolGroupValueVector::const_iterator dcend = dummyNodeList.end();
    for (SymbolGroupValueVector::const_iterator it = dummyNodeList.begin(); it != dcend; ++it) {
        if (const SymbolGroupValue n = (*it).typeCast(nodeType.c_str())) {
            nodeList.push_back(n);
        }  else {
            return SymbolGroupValueVector();
        }
    }
    return nodeList;
}

// QSet<>: Contains a 'QHash<key, QHashDummyValue>' as member 'q_hash'.
// Just dump the keys as an array.
static inline AbstractSymbolGroupNodePtrVector
    qSetChildList(const SymbolGroupValue &v, int count)
{
    const SymbolGroupValue qHash = v["q_hash"];
    AbstractSymbolGroupNodePtrVector rc;
    if (!count || !qHash)
        return rc;
    const SymbolGroupValueVector nodes = qHashNodes(qHash, count);
    if (nodes.size() != VectorIndexType(count))
        return rc;
    rc.reserve(count);
    for (int i = 0; i < count; i++) {
        if (const SymbolGroupValue key = nodes.at(i)["key"]) {
            rc.push_back(ReferenceSymbolGroupNode::createArrayNode(i, key.node()));
        } else {
            return AbstractSymbolGroupNodePtrVector();
        }
    }
    return rc;
}

// QHash<>: Add with fake map nodes.
static inline AbstractSymbolGroupNodePtrVector
    qHashChildList(const SymbolGroupValue &v, int count)
{
    AbstractSymbolGroupNodePtrVector rc;
    if (!count)
        return rc;
    const SymbolGroupValueVector nodes = qHashNodes(v, count);
    if (nodes.size() != count)
        return rc;
    rc.reserve(count);
    for (int i = 0; i < count; i++) {
        const SymbolGroupValue &mapNode = nodes.at(i);
        const SymbolGroupValue key = mapNode["key"];
        const SymbolGroupValue value = mapNode["value"];
        if (!key || !value)
            return AbstractSymbolGroupNodePtrVector();
        rc.push_back(MapNodeSymbolGroupNode::create(i, mapNode.address(),
                                                    mapNode.type(), key.node(), value.node()));
    }
    return rc;
}

// QMap<>: Return the list of QMapData::Node
static inline SymbolGroupValueVector qMapNodes(const SymbolGroupValue &v, VectorIndexType count)
{
    const SymbolGroupValue e = v["e"];
    const ULONG64 ePtr = e.pointerValue();
    if (debugMap)
        DebugPrint() << v.type() << " E=0x" << std::hex << ePtr;
    if (!ePtr)
        return SymbolGroupValueVector();
    if (debugMap)
        DebugPrint() << v.type() << " E=0x" << std::hex << ePtr;
    SymbolGroupValueVector rc;
    rc.reserve(count);
    SymbolGroupValue n = e["forward"][unsigned(0)];
    for (VectorIndexType i = 0; i < count && n && n.pointerValue() != ePtr; i++) {
        rc.push_back(n);
        n = n["forward"][unsigned(0)];
    }
    return rc;
}

// QMap<>: Add with fake map nodes.
static inline AbstractSymbolGroupNodePtrVector
    qMapChildList(const SymbolGroupValue &v, VectorIndexType count)
{
    if (debugMap)
        DebugPrint() << v.type() << "," << count;

    if (!count)
        return AbstractSymbolGroupNodePtrVector();
    // Get node type: 'class namespace::QMap<K,T>'
    // ->'QtCored4!namespace::QMapNode<K,T>'
    // Note: Any types QMapNode<> will not be found without modules!
    const std::string mapNodeType = qHashNodeType(v, "Node");
    const std::string mapPayloadNodeType = qHashNodeType(v, "PayloadNode");
    // Calculate the offset needed (see QMap::concrete() used by the iterator).
    const unsigned mapNodeSize = SymbolGroupValue::sizeOf(mapPayloadNodeType.c_str());
    const unsigned payloadNodeSize = SymbolGroupValue::sizeOf(mapPayloadNodeType.c_str());
    const unsigned pointerSize = SymbolGroupValue::pointerSize();
    if (debugMap) {
        DebugPrint() << v.type() << "," << mapNodeType << ':'
                     << mapNodeSize << ',' << mapPayloadNodeType << ':' << payloadNodeSize
                     << ", pointerSize=" << pointerSize;
    }
    if (!payloadNodeSize || !mapNodeSize)
        return AbstractSymbolGroupNodePtrVector();
    const ULONG64 payLoad  = payloadNodeSize - pointerSize;
    // Get the value offset. Manually determine the alignment to be able
    // to retrieve key/value without having to deal with QMapNode<> (see below).
    // Subtract the 2 trailing pointers of the node.
    const std::vector<std::string> innerTypes = v.innerTypes();
    if (innerTypes.size() != 2u)
        return AbstractSymbolGroupNodePtrVector();
    const unsigned valueSize = SymbolGroupValue::sizeOf(innerTypes.at(1).c_str());
    const unsigned valueOffset = mapNodeSize - valueSize - pointerSize;
    if (debugMap)
        DebugPrint() << "Payload=" << payLoad << ",valueOffset=" << valueOffset << ','
                     << innerTypes.front() << ',' << innerTypes.back() << ':' << valueSize;
    if (!valueOffset || !valueSize)
        return AbstractSymbolGroupNodePtrVector();
    // Get the children.
    const SymbolGroupValueVector childNodes = qMapNodes(v, count);
    if (debugMap)
        DebugPrint() << "children: " << childNodes.size() << " of " << count;
    // Deep  expansion of the forward[0] sometimes fails. In that case,
    // take what we can get.
    if (childNodes.size() != count)
        count = childNodes.size();
    // The correct way of doing this would be to construct additional symbols
    // '*(QMapNode<K,V> *)(node_address)'. However, when doing this as of
    // 'CDB 6.12.0002.633' (21.12.2010) IDebugSymbolGroup::AddSymbol()
    // just fails, returning DEBUG_ANY_ID without actually doing something. So,
    // we circumvent the map nodes and directly create key and values at their addresses.
    AbstractSymbolGroupNodePtrVector rc;
    rc.reserve(count);
    std::string errorMessage;
    SymbolGroup *sg = v.node()->symbolGroup();

    for (VectorIndexType i = 0; i < count ; i++) {
        const ULONG64 nodePtr = childNodes.at(i).pointerValue();
        if (!nodePtr)
            return AbstractSymbolGroupNodePtrVector();
        const ULONG64 keyAddress = nodePtr - payLoad;
        const std::string keyExp = pointedToSymbolName(keyAddress, innerTypes.front());
        const std::string valueExp = pointedToSymbolName(keyAddress + valueOffset, innerTypes.at(1));
        if (debugMap) {
            DebugPrint() << '#' << i << '/' << count << ' ' << std::hex << ",node=0x" << nodePtr <<
                  ',' <<keyExp << ',' << valueExp;
        }
        // Create the nodes
        SymbolGroupNode *keyNode = sg->addSymbol(keyExp, std::string(), &errorMessage);
        SymbolGroupNode *valueNode = sg->addSymbol(valueExp, std::string(), &errorMessage);
        if (!keyNode || !valueNode)
            return AbstractSymbolGroupNodePtrVector();
        rc.push_back(MapNodeSymbolGroupNode::create(int(i), keyAddress,
                                                    mapNodeType, keyNode, valueNode));
    }
    return rc;
}

AbstractSymbolGroupNodePtrVector containerChildren(SymbolGroupNode *node, int type,
                                                   int size, const SymbolGroupValueContext &ctx)
{
    if (!size)
        return AbstractSymbolGroupNodePtrVector();
    if (size > 100)
        size = 100;
    switch (type) {
    case KT_QVector:
        return qVectorChildList(node, size, ctx);
    case KT_StdVector:
        return stdVectorChildList(node, size, ctx);
    case KT_QLinkedList:
        return qLinkedListChildList(node, size, ctx);
    case KT_QList:
        return qListChildList(SymbolGroupValue(node, ctx), size);
    case KT_QQueue:
        if (const SymbolGroupValue qList = SymbolGroupValue(node, ctx)[unsigned(0)])
            return qListChildList(qList, size);
        break;
    case KT_QStack:
        if (const SymbolGroupValue qVector = SymbolGroupValue(node, ctx)[unsigned(0)])
            return qVectorChildList(qVector.node(), size, ctx);
        break;
    case KT_QHash:
        return qHashChildList(SymbolGroupValue(node, ctx), size);
    case KT_QMultiHash:
        if (const SymbolGroupValue hash = SymbolGroupValue(node, ctx)[unsigned(0)])
            return qHashChildList(hash, size);
        break;
    case KT_QSet:
        return qSetChildList(SymbolGroupValue(node, ctx), size);
    case KT_QMap:
        return qMapChildList(SymbolGroupValue(node, ctx), size);
    case KT_QMultiMap:
        if (const SymbolGroupValue qmap = SymbolGroupValue(node, ctx)[unsigned(0)])
            return qMapChildList(qmap, size);
        break;
    case KT_QStringList:
        if (const SymbolGroupValue qList = SymbolGroupValue(node, ctx)[unsigned(0)])
            return qListChildList(qList, size);
        break;
    case KT_StdList:
        return stdListChildList(node, size , ctx);
    case KT_StdDeque:
        return stdDequeChildList(SymbolGroupValue(node, ctx), size);
    case KT_StdStack:
        if (const SymbolGroupValue deque = SymbolGroupValue(node, ctx)[unsigned(0)])
            return stdDequeChildList(deque, size);
        break;
    case KT_StdSet:
        return stdSetChildList(SymbolGroupValue(node, ctx), size);
    case KT_StdMap:
    case KT_StdMultiMap:
        return stdMapChildList(SymbolGroupValue(node, ctx), size);
    }
    return AbstractSymbolGroupNodePtrVector();
}
