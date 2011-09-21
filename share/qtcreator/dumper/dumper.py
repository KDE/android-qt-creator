import sys
import base64
import __builtin__
import os
import tempfile

# Fails on Windows.
try:
    import curses.ascii
    def printableChar(ucs):
        if curses.ascii.isprint(ucs):
            return ucs
        return '?'
except:
    def printableChar(ucs):
        if ucs >= 32 and ucs <= 126:
            return ucs
        return '?'

# Fails on SimulatorQt.
tempFileCounter = 0
try:
    # Test if 2.6 is used (Windows), trigger exception and default
    # to 2nd version.
    tempfile.NamedTemporaryFile(prefix="py_",delete=True)
    def createTempFile():
        file = tempfile.NamedTemporaryFile(prefix="py_",delete=False)
        file.close()
        return file.name, file

except:
    def createTempFile():
        global tempFileCounter
        tempFileCounter += 1
        fileName = "%s/py_tmp_%d_%d" \
            % (tempfile.gettempdir(), os.getpid(), tempFileCounter)
        return fileName, None

def removeTempFile(name, file):
    try:
        os.remove(name)
    except:
        pass

try:
    import binascii
except:
    pass

verbosity = 0
verbosity = 1

# Some "Enums"

# Encodings
Unencoded8Bit, \
Base64Encoded8BitWithQuotes, \
Base64Encoded16BitWithQuotes, \
Base64Encoded32BitWithQuotes, \
Base64Encoded16Bit, \
Base64Encoded8Bit, \
Hex2EncodedLatin1, \
Hex4EncodedLittleEndian, \
Hex8EncodedLittleEndian, \
Hex2EncodedUtf8, \
Hex8EncodedBigEndian, \
Hex4EncodedBigEndian, \
Hex4EncodedLittleEndianWithoutQuotes, \
Hex2EncodedLocal8Bit, \
JulianDate, \
MillisecondsSinceMidnight, \
JulianDateAndMillisecondsSinceMidnight \
    = range(17)

# Display modes
StopDisplay, \
DisplayImage1, \
DisplayString, \
DisplayImage, \
DisplayProcess \
    = range(5)


def hasInferiorThreadList():
    #return False
    try:
        a = gdb.inferiors()[0].threads()
        return True
    except:
        return False

def upcast(value):
    try:
        type = value.dynamic_type
        return value.cast(type)
    except:
        return value

typeCache = {}

class TypeInfo:
    def __init__(self, type):
        self.size = type.sizeof
        self.reported = False

typeInfoCache = {}

def lookupType(typestring):
    type = typeCache.get(typestring)
    #warn("LOOKUP 1: %s -> %s" % (typestring, type))
    if not type is None:
        return type

    ts = typestring
    while True:
        #WARN("ts: '%s'" % ts)
        if ts.startswith("class "):
            ts = ts[6:]
        elif ts.startswith("struct "):
            ts = ts[7:]
        elif ts.startswith("const "):
            ts = ts[6:]
        elif ts.startswith("volatile "):
            ts = ts[9:]
        elif ts.startswith("enum "):
            ts = ts[5:]
        elif ts.endswith(" const"):
            ts = ts[:-6]
        elif ts.endswith(" volatile"):
            ts = ts[:-9]
        elif ts.endswith("*const"):
            ts = ts[:-5]
        elif ts.endswith("*volatile"):
            ts = ts[:-8]
        else:
            break

    if ts.endswith('*'):
        type = lookupType(ts[0:-1])
        if not type is None:
            type = type.pointer()
            typeCache[typestring] = type
            return type

    try:
        #warn("LOOKING UP '%s'" % ts)
        type = gdb.lookup_type(ts)
    except RuntimeError, error:
        #warn("LOOKING UP '%s': %s" % (ts, error))
        # See http://sourceware.org/bugzilla/show_bug.cgi?id=11912
        exp = "(class '%s'*)0" % ts
        try:
            type = parseAndEvaluate(exp).type.target()
        except:
            # Can throw "RuntimeError: No type named class Foo."
            pass
    except:
        #warn("LOOKING UP '%s' FAILED" % ts)
        pass

    # This could still be None as gdb.lookup_type("char[3]") generates
    # "RuntimeError: No type named char[3]"
    return type

def cleanAddress(addr):
    if addr is None:
        return "<no address>"
    # We cannot use str(addr) as it yields rubbish for char pointers
    # that might trigger Unicode encoding errors.
    #return addr.cast(lookupType("void").pointer())
    # We do not use "hex(...)" as it (sometimes?) adds a "L" suffix.
    return "0x%x" % long(addr)

def extractTemplateArgument(type, position):
    level = 0
    skipSpace = False
    inner = ""
    type = str(type)
    for c in type[type.find('<') + 1 : -1]:
        if c == '<':
            inner += c
            level += 1
        elif c == '>':
            level -= 1
            inner += c
        elif c == ',':
            if level == 0:
                if position == 0:
                    return inner
                position -= 1
                inner = ""
            else:
                inner += c
                skipSpace = True
        else:
            if skipSpace and c == ' ':
                pass
            else:
                inner += c
                skipSpace = False
    return inner

def templateArgument(type, position):
    try:
        # This fails on stock 7.2 with
        # "RuntimeError: No type named myns::QObject.\n"
        return type.template_argument(position)
    except:
        # That's something like "myns::QList<...>"
        return lookupType(extractTemplateArgument(type.strip_typedefs(), position))


# Workaround for gdb < 7.1
def numericTemplateArgument(type, position):
    try:
        return int(type.template_argument(position))
    except RuntimeError, error:
        # ": No type named 30."
        msg = str(error)
        return int(msg[14:-1])


def showException(msg, exType, exValue, exTraceback):
    warn("**** CAUGHT EXCEPTION: %s ****" % msg)
    try:
        import traceback
        for line in traceback.format_exception(exType, exValue, exTraceback):
            warn("%s" % line)
    except:
        pass


class OutputSafer:
    def __init__(self, d, pre = "", post = ""):
        self.d = d
        self.pre = pre
        self.post = post

    def __enter__(self):
        self.d.put(self.pre)
        self.savedOutput = self.d.output
        self.d.output = []

    def __exit__(self, exType, exValue, exTraceBack):
        self.d.put(self.post)
        if self.d.passExceptions and not exType is None:
            showException("OUTPUTSAFER", exType, exValue, exTraceBack)
            self.d.output = self.savedOutput
        else:
            self.savedOutput.extend(self.d.output)
            self.d.output = self.savedOutput
        return False


class NoAddress:
    def __init__(self, d):
        self.d = d

    def __enter__(self):
        self.savedPrintsAddress = self.d.currentPrintsAddress
        self.d.currentPrintsAddress = False

    def __exit__(self, exType, exValue, exTraceBack):
        self.d.currentPrintsAddress = self.savedPrintsAddress


class SubItem:
    def __init__(self, d, component):
        self.d = d
        self.iname = "%s.%s" % (d.currentIName, component)
        self.name = component

    def __enter__(self):
        self.d.put('{')
        #if not self.name is None:
        if isinstance(self.name, str):
            self.d.put('name="%s",' % self.name)
        self.savedIName = self.d.currentIName
        self.savedValue = self.d.currentValue
        self.savedValuePriority = self.d.currentValuePriority
        self.savedValueEncoding = self.d.currentValueEncoding
        self.savedType = self.d.currentType
        self.savedTypePriority = self.d.currentTypePriority
        self.d.currentIName = self.iname
        self.d.currentValuePriority = -100
        self.d.currentValueEncoding = None
        self.d.currentType = ""
        self.d.currentTypePriority = -100

    def __exit__(self, exType, exValue, exTraceBack):
        #warn(" CURRENT VALUE: %s %s %s" % (self.d.currentValue,
        #    self.d.currentValueEncoding, self.d.currentValuePriority))
        if self.d.passExceptions and not exType is None:
            showException("SUBITEM", exType, exValue, exTraceBack)
        try:
            #warn("TYPE VALUE: %s" % self.d.currentValue)
            typeName = stripClassTag(self.d.currentType)
            #warn("TYPE: '%s'  DEFAULT: '%s' % (typeName, self.d.currentChildType))

            if len(typeName) > 0 and typeName != self.d.currentChildType:
                self.d.put('type="%s",' % typeName) # str(type.unqualified()) ?
                if not typeName in typeInfoCache \
                        and typeName != " ": # FIXME: Move to lookupType
                    typeObj = lookupType(typeName)
                    if not typeObj is None:
                        typeInfoCache[typeName] = TypeInfo(typeObj)
            if  self.d.currentValue is None:
                self.d.put('value="<not accessible>",numchild="0",')
            else:
                if not self.d.currentValueEncoding is None:
                    self.d.put('valueencoded="%d",' % self.d.currentValueEncoding)
                self.d.put('value="%s",' % self.d.currentValue)
        except:
            pass
        self.d.put('},')
        self.d.currentIName = self.savedIName
        self.d.currentValue = self.savedValue
        self.d.currentValuePriority = self.savedValuePriority
        self.d.currentValueEncoding = self.savedValueEncoding
        self.d.currentType = self.savedType
        self.d.currentTypePriority = self.savedTypePriority
        return True

class TopLevelItem(SubItem):
    def __init__(self, d, iname):
        self.d = d
        self.iname = iname
        self.name = None

class UnnamedSubItem(SubItem):
    def __init__(self, d, component):
        self.d = d
        self.iname = "%s.%s" % (self.d.currentIName, component)
        self.name = None

class Children:
    def __init__(self, d, numChild = 1, childType = None, childNumChild = None,
            maxNumChild = None, addrBase = None, addrStep = None):
        self.d = d
        self.numChild = numChild
        self.childNumChild = childNumChild
        self.maxNumChild = maxNumChild
        self.addrBase = addrBase
        self.addrStep = addrStep
        self.printsAddress = True
        if childType is None:
            self.childType = None
        else:
            self.childType = stripClassTag(str(childType))
            self.d.put('childtype="%s",' % self.childType)
            if childNumChild is None:
                if isSimpleType(childType):
                    self.d.put('childnumchild="0",')
                    self.childNumChild = 0
                elif childType.code == PointerCode:
                    self.d.put('childnumchild="1",')
                    self.childNumChild = 1
            else:
                self.d.put('childnumchild="%s",' % childNumChild)
                self.childNumChild = childNumChild
        try:
            if not addrBase is None and not addrStep is None:
                self.d.put('addrbase="0x%x",' % long(addrBase))
                self.d.put('addrstep="0x%x",' % long(addrStep))
                self.printsAddress = False
        except:
            warn("ADDRBASE: %s" % addrBase)
        #warn("CHILDREN: %s %s %s" % (numChild, childType, childNumChild))

    def __enter__(self):
        self.savedChildType = self.d.currentChildType
        self.savedChildNumChild = self.d.currentChildNumChild
        self.savedNumChild = self.d.currentNumChild
        self.savedMaxNumChild = self.d.currentMaxNumChild
        self.savedPrintsAddress = self.d.currentPrintsAddress
        self.d.currentChildType = self.childType
        self.d.currentChildNumChild = self.childNumChild
        self.d.currentNumChild = self.numChild
        self.d.currentMaxNumChild = self.maxNumChild
        self.d.currentPrintsAddress = self.printsAddress
        self.d.put("children=[")

    def __exit__(self, exType, exValue, exTraceBack):
        if self.d.passExceptions and not exType is None:
            showException("CHILDREN", exType, exValue, exTraceBack)
        if not self.d.currentMaxNumChild is None:
            if self.d.currentMaxNumChild < self.d.currentNumChild:
                self.d.put('{name="<incomplete>",value="",type="",numchild="0"},')
        self.d.currentChildType = self.savedChildType
        self.d.currentChildNumChild = self.savedChildNumChild
        self.d.currentNumChild = self.savedNumChild
        self.d.currentMaxNumChild = self.savedMaxNumChild
        self.d.currentPrintsAddress = self.savedPrintsAddress
        self.d.put('],')
        return True


def value(expr):
    value = parseAndEvaluate(expr)
    try:
        return int(value)
    except:
        return str(value)

def isSimpleType(typeobj):
    code = typeobj.code
    return code == BoolCode \
        or code == CharCode \
        or code == IntCode \
        or code == FloatCode \
        or code == EnumCode

def warn(message):
    if True or verbosity > 0:
        print "XXX: %s\n" % message.encode("latin1")
    pass

def check(exp):
    if not exp:
        raise RuntimeError("Check failed")

def checkRef(ref):
    count = ref["_q_value"]
    check(count > 0)
    check(count < 1000000) # assume there aren't a million references to any object

#def couldBePointer(p, align):
#    type = lookupType("unsigned int")
#    ptr = gdb.Value(p).cast(type)
#    d = int(str(ptr))
#    warn("CHECKING : %s %d " % (p, ((d & 3) == 0 and (d > 1000 or d == 0))))
#    return (d & (align - 1)) and (d > 1000 or d == 0)


def checkAccess(p, align = 1):
    return p.dereference()

def checkContents(p, expected, align = 1):
    if int(p.dereference()) != expected:
        raise RuntimeError("Contents check failed")

def checkPointer(p, align = 1):
    if not isNull(p):
        p.dereference()

def isAccessible(p):
    try:
        long(p)
        return True
    except:
        return False

def isNull(p):
    # The following can cause evaluation to abort with "UnicodeEncodeError"
    # for invalid char *, as their "contents" is being examined
    #s = str(p)
    #return s == "0x0" or s.startswith("0x0 ")
    #try:
    #    # Can fail with: "RuntimeError: Cannot access memory at address 0x5"
    #    return p.cast(lookupType("void").pointer()) == 0
    #except:
    #    return False
    try:
        # Can fail with: "RuntimeError: Cannot access memory at address 0x5"
        return long(p) == 0
    except:
        return False

movableTypes = set([
    "QBrush", "QBitArray", "QByteArray", "QCustomTypeInfo", "QChar", "QDate",
    "QDateTime", "QFileInfo", "QFixed", "QFixedPoint", "QFixedSize",
    "QHashDummyValue", "QIcon", "QImage", "QLine", "QLineF", "QLatin1Char",
    "QLocale", "QMatrix", "QModelIndex", "QPoint", "QPointF", "QPen",
    "QPersistentModelIndex", "QResourceRoot", "QRect", "QRectF", "QRegExp",
    "QSize", "QSizeF", "QString", "QTime", "QTextBlock", "QUrl", "QVariant",
    "QXmlStreamAttribute", "QXmlStreamNamespaceDeclaration",
    "QXmlStreamNotationDeclaration", "QXmlStreamEntityDeclaration"
])

def stripClassTag(typeName):
    if typeName.startswith("class "):
        return typeName[6:]
    if typeName.startswith("struct "):
        return typeName[7:]
    if typeName.startswith("const "):
        return typeName[6:]
    if typeName.startswith("volatile "):
        return typeName[9:]
    return typeName

def checkPointerRange(p, n):
    for i in xrange(n):
        checkPointer(p)
        ++p

def call2(value, func, args):
    # args is a tuple.
    arg = ""
    for i in range(len(args)):
        if i:
            arg += ','
        a = args[i]
        if (':' in a) and not ("'" in a):
            arg = "'%s'" % a
        else:
            arg += a

    #warn("CALL: %s -> %s(%s)" % (value, func, arg))
    type = stripClassTag(str(value.type))
    if type.find(":") >= 0:
        type = "'" + type + "'"
    # 'class' is needed, see http://sourceware.org/bugzilla/show_bug.cgi?id=11912
    exp = "((class %s*)%s)->%s(%s)" % (type, value.address, func, arg)
    #warn("CALL: %s" % exp)
    result = None
    try:
        result = parseAndEvaluate(exp)
    except:
        pass
    #warn("  -> %s" % result)
    return result

def call(value, func, *args):
    return call2(value, func, args)

def makeValue(type, init):
    type = stripClassTag(type)
    if type.find(":") >= 0:
        type = "'" + type + "'"
    # Avoid malloc symbol clash with QVector.
    gdb.execute("set $d = (%s*)calloc(sizeof(%s), 1)" % (type, type))
    gdb.execute("set *$d = {%s}" % init)
    value = parseAndEvaluate("$d").dereference()
    #warn("  TYPE: %s" % value.type)
    #warn("  ADDR: %s" % value.address)
    #warn("  VALUE: %s" % value)
    return value

def makeExpression(value):
    type = stripClassTag(str(value.type))
    if type.find(":") >= 0:
        type = "'" + type + "'"
    #warn("  TYPE: %s" % type)
    #exp = "(*(%s*)(&%s))" % (type, value.address)
    exp = "(*(%s*)(%s))" % (type, value.address)
    #warn("  EXP: %s" % exp)
    return exp

qqNs = None

def qtNamespace():
    # FIXME: This only works when call from inside a Qt function frame.
    global qqNs
    if not qqNs is None:
        return qqNs
    try:
        str = catchCliOutput("ptype QString::Null")[0]
        # The result looks like:
        # "type = const struct myns::QString::Null {"
        # "    <no data fields>"
        # "}"
        pos1 = str.find("struct") + 7
        pos2 = str.find("QString::Null")
        if pos1 > -1 and pos2 > -1:
            qqNs = str[pos1:pos2]
            return qqNs
        return ""
    except:
        return ""

# --  Determine major Qt version by calling qVersion (cached)

qqMajorVersion = None

def qtMajorVersion():
    global qqMajorVersion
    if not qqMajorVersion is None:
        return qqMajorVersion
    try:
        # -- Result is returned as character, need to subtract '0'
        qqMajorVersion = int(parseAndEvaluate(qtNamespace() + "qVersion()[0]")) - 48
        return qqMajorVersion
    except:
        return 0

def findFirstZero(p, maximum):
    for i in xrange(maximum):
        if p.dereference() == 0:
            return i
        p = p + 1
    return maximum + 1

def extractCharArray(p, maxsize):
    t = lookupType("unsigned char").pointer()
    p = p.cast(t)
    limit = findFirstZero(p, maxsize)
    s = ""
    for i in xrange(limit):
        s += "%c" % int(p.dereference())
        p += 1
    if i > maxsize:
        s += "..."
    return s

def extractByteArray(value):
    d_ptr = value['d'].dereference()
    data = d_ptr['data']
    size = d_ptr['size']
    alloc = d_ptr['alloc']
    check(0 <= size and size <= alloc and alloc <= 100*1000*1000)
    checkRef(d_ptr["ref"])
    if size > 0:
        checkAccess(data, 4)
        checkAccess(data + size) == 0
    return extractCharArray(data, min(100, size))

def encodeCharArray(p, maxsize, limit = -1):
    t = lookupType("unsigned char").pointer()
    p = p.cast(t)
    if limit == -1:
        limit = findFirstZero(p, maxsize)
    s = ""
    try:
        # gdb.Inferior is new in gdb 7.2
        inferior = gdb.inferiors()[0]
        s = binascii.hexlify(inferior.read_memory(p, limit))
    except:
        for i in xrange(limit):
            s += "%02x" % int(p.dereference())
            p += 1
    if limit > maxsize:
        s += "2e2e2e"
    return s

def encodeChar2Array(p, maxsize):
    t = lookupType("unsigned short").pointer()
    p = p.cast(t)
    limit = findFirstZero(p, maxsize)
    s = ""
    for i in xrange(limit):
        s += "%04x" % int(p.dereference())
        p += 1
    if i == maxsize:
        s += "2e002e002e00"
    return s

def encodeChar4Array(p, maxsize):
    t = lookupType("unsigned int").pointer()
    p = p.cast(t)
    limit = findFirstZero(p, maxsize)
    s = ""
    for i in xrange(limit):
        s += "%08x" % int(p.dereference())
        p += 1
    if i > maxsize:
        s += "2e0000002e0000002e000000"
    return s

def qByteArrayData(value):
    if qtMajorVersion() < 5:
        d_ptr = value['d'].dereference()
        checkRef(d_ptr["ref"])
        data = d_ptr['data']
        size = d_ptr['size']
        alloc = d_ptr['alloc']
        return data, size, alloc
    else: # Qt5: Implement the QByteArrayData::data() accessor.
        qByteArrayData = value['d'].dereference()
        size = qByteArrayData['size']
        alloc = qByteArrayData['alloc']
        charPointerType = lookupType('char *')
        data = qByteArrayData['d'].cast(charPointerType) \
             + qByteArrayData['offset'] + charPointerType.sizeof
        return data, size, alloc

def encodeByteArray(value):
    data, size, alloc = qByteArrayData(value)
    check(0 <= size and size <= alloc and alloc <= 100*1000*1000)
    if size > 0:
        checkAccess(data, 4)
        checkAccess(data + size) == 0
    return encodeCharArray(data, 100, size)

def qQStringData(value):
    if qtMajorVersion() < 5:
        d_ptr = value['d'].dereference()
        checkRef(d_ptr['ref'])
        return d_ptr['data'], int(d_ptr['size']), int(d_ptr['alloc'])
    else: # Qt5: Implement the QStringArrayData::data() accessor.
        qStringData = value['d'].dereference()
        ushortPointerType = lookupType('ushort *')
        data = qStringData['d'].cast(ushortPointerType) \
            + ushortPointerType.sizeof / 2 + qStringData['offset']
        return data, int(qStringData['size']), int(qStringData['alloc'])

def encodeString(value):
    data, size, alloc = qQStringData(value)

    check(0 <= size and size <= alloc and alloc <= 100*1000*1000)
    if size > 0:
        checkAccess(data, 4)
        checkAccess(data + size) == 0
    s = ""
    limit = min(size, 1000)
    try:
        # gdb.Inferior is new in gdb 7.2
        inferior = gdb.inferiors()[0]
        s = binascii.hexlify(inferior.read_memory(data, 2 * limit))
    except:
        p = data
        for i in xrange(limit):
            val = int(p.dereference())
            s += "%02x" % (val % 256)
            s += "%02x" % (val / 256)
            p += 1
    if limit < size:
        s += "2e002e002e00"
    return s

def stripTypedefs(type):
    type = type.unqualified()
    while type.code == TypedefCode:
        type = type.strip_typedefs().unqualified()
    return type

def extractFields(type):
    # Insufficient, see http://sourceware.org/bugzilla/show_bug.cgi?id=10953:
    #fields = type.fields()
    # Insufficient, see http://sourceware.org/bugzilla/show_bug.cgi?id=11777:
    #fields = defsype).fields()
    # This seems to work.
    #warn("TYPE 0: %s" % type)
    type = stripTypedefs(type)
    fields = type.fields()
    if len(fields):
        return fields
    #warn("TYPE 1: %s" % type)
    # This fails for arrays. See comment in lookupType.
    type0 = lookupType(str(type))
    if not type0 is None:
        type = type0
    if type.code == FunctionCode:
        return []
    #warn("TYPE 2: %s" % type)
    fields = type.fields()
    #warn("FIELDS: %s" % fields)
    return fields

#######################################################################
#
# LocalItem
#
#######################################################################

# Contains iname, name, and value.
class LocalItem:
    pass

#######################################################################
#
# SetupCommand
#
#######################################################################

# This is a cache mapping from 'type name' to 'display alternatives'.
qqFormats = {}

# This is a cache of all known dumpers.
qqDumpers = {}

# This is a cache of all dumpers that support writing.
qqEditable = {}

# This is a cache of the namespace of the currently used Qt version.
# FIXME: This is not available on 'bbsetup' time, only at 'bb' time.

# This is a cache of typenames->bool saying whether we are QObject
# derived.
qqQObjectCache = {}

def bbsetup(args):
    module = sys.modules[__name__]
    for key, value in module.__dict__.items():
        if key.startswith("qdump__"):
            name = key[7:]
            qqDumpers[name] = value
            qqFormats[name] = qqFormats.get(name, "")
        elif key.startswith("qform__"):
            name = key[7:]
            formats = ""
            try:
                formats = value()
            except:
                pass
            qqFormats[name] = formats
        elif key.startswith("qedit__"):
            name = key[7:]
            try:
                qqEditable[name] = value
            except:
                pass
    result = "dumpers=["
    #qqNs = qtNamespace() # This is too early
    for key, value in qqFormats.items():
        if qqEditable.has_key(key):
            result += '{type="%s",formats="%s",editable="true"},' % (key, value)
        else:
            result += '{type="%s",formats="%s"},' % (key, value)
    result += ']'
    #result += ',namespace="%s"' % qqNs
    result += ',hasInferiorThreadList="%s"' % int(hasInferiorThreadList())
    return result

registerCommand("bbsetup", bbsetup)


#######################################################################
#
# Edit Command
#
#######################################################################

def bbedit(args):
    (type, expr, value) = args.split(",")
    type = base64.b16decode(type, True)
    ns = qtNamespace()
    if type.startswith(ns):
        type = type[len(ns):]
    type = type.replace("::", "__")
    pos = type.find('<')
    if pos != -1:
        type = type[0:pos]
    expr = base64.b16decode(expr, True)
    value = base64.b16decode(value, True)
    #warn("EDIT: %s %s %s %s: " % (pos, type, expr, value))
    if qqEditable.has_key(type):
        qqEditable[type](expr, value)

registerCommand("bbedit", bbedit)


#######################################################################
#
# Frame Command
#
#######################################################################

def bb(args):
    output = 'data=[' + "".join(Dumper(args).output) + '],typeinfo=['
    for typeName, typeInfo in typeInfoCache.iteritems():
        if not typeInfo.reported:
            output += '{name="' + base64.b64encode(typeName)
            output += '",size="' + str(typeInfo.size) + '"},'
            typeInfo.reported = True
    output += ']';
    return output

registerCommand("bb", bb)

def p1(args):
    import cProfile
    cProfile.run('bb("%s")' % args, "/tmp/bbprof")
    import pstats
    pstats.Stats('/tmp/bbprof').sort_stats('time').print_stats()
    return ""

registerCommand("p1", p1)

def p2(args):
    import timeit
    return timeit.repeat('bb("%s")' % args,
        'from __main__ import bb', number=10)

registerCommand("p2", p2)


#######################################################################
#
# The Dumper Class
#
#######################################################################


class Dumper:
    def __init__(self, args):
        self.output = []
        self.currentIName = ""
        self.currentPrintsAddress = True
        self.currentChildType = ""
        self.currentChildNumChild = -1
        self.currentMaxNumChild = -1
        self.currentNumChild = -1
        self.currentValue = None
        self.currentValuePriority = -100
        self.currentValueEncoding = None
        self.currentType = None
        self.currentTypePriority = -100
        self.typeformats = {}
        self.formats = {}
        self.expandedINames = ""

        options = []
        varList = []
        watchers = ""

        resultVarName = ""
        for arg in args.split(' '):
            pos = arg.find(":") + 1
            if arg.startswith("options:"):
                options = arg[pos:].split(",")
            elif arg.startswith("vars:"):
                if len(arg[pos:]) > 0:
                    varList = arg[pos:].split(",")
            elif arg.startswith("resultvarname:"):
                resultVarName = arg[pos:]
            elif arg.startswith("expanded:"):
                self.expandedINames = set(arg[pos:].split(","))
            elif arg.startswith("typeformats:"):
                for f in arg[pos:].split(","):
                    pos = f.find("=")
                    if pos != -1:
                        type = base64.b16decode(f[0:pos], True)
                        self.typeformats[type] = int(f[pos+1:])
            elif arg.startswith("formats:"):
                for f in arg[pos:].split(","):
                    pos = f.find("=")
                    if pos != -1:
                        self.formats[f[0:pos]] = int(f[pos+1:])
            elif arg.startswith("watchers:"):
                watchers = base64.b16decode(arg[pos:], True)

        self.useFancy = "fancy" in options
        self.passExceptions = "pe" in options
        self.autoDerefPointers = "autoderef" in options
        self.partialUpdate = "partial" in options
        self.tooltipOnly = "tooltiponly" in options
        self.noLocals = "nolocals" in options
        self.ns = qtNamespace()

        #warn("NAMESPACE: '%s'" % self.ns)
        #warn("VARIABLES: %s" % varList)
        #warn("EXPANDED INAMES: %s" % self.expandedINames)
        #warn("WATCHERS: %s" % watchers)
        #warn("PARTIAL: %s" % self.partialUpdate)
        #warn("NO LOCALS: %s" % self.noLocals)
        module = sys.modules[__name__]

        #
        # Locals
        #
        locals = []
        fullUpdateNeeded = True
        if self.partialUpdate and len(varList) == 1 and not self.tooltipOnly:
            #warn("PARTIAL: %s" % varList)
            parts = varList[0].split('.')
            #warn("PARTIAL PARTS: %s" % parts)
            name = parts[1]
            #warn("PARTIAL VAR: %s" % name)
            #fullUpdateNeeded = False
            try:
                frame = gdb.selected_frame()
                item = LocalItem()
                item.iname = "local." + name
                item.name = name
                item.value = frame.read_var(name)
                locals = [item]
                #warn("PARTIAL LOCALS: %s" % locals)
                fullUpdateNeeded = False
            except:
                pass
            varList = []

        if fullUpdateNeeded and not self.tooltipOnly and not self.noLocals:
            locals = listOfLocals(varList)

        # Take care of the return value of the last function call.
        if len(resultVarName) > 0:
            try:
                item = LocalItem()
                item.name = resultVarName
                item.iname = "return." + resultVarName
                item.value = parseAndEvaluate(resultVarName)
                locals.append(item)
            except:
                # Don't bother. It's only supplementary information anyway.
                pass

        for item in locals:
            value = upcast(item.value)
            with OutputSafer(self, "", ""):
                self.anonNumber = -1

                type = value.type.unqualified()
                typeName = str(type)

                # Special handling for char** argv.
                if type.code == PointerCode \
                        and item.iname == "local.argv" \
                        and typeName == "char **":
                    n = 0
                    p = value
                    # p is 0 for "optimized out" cases. Or contains rubbish.
                    try:
                        if not isNull(p):
                            while not isNull(p.dereference()) and n <= 100:
                                p += 1
                                n += 1
                    except:
                        pass

                    with TopLevelItem(self, item.iname):
                        self.put('iname="local.argv",name="argv",')
                        self.putItemCount(n, 100)
                        self.putType(typeName)
                        self.putNumChild(n)
                        if self.currentIName in self.expandedINames:
                            p = value
                            with Children(self, n):
                                for i in xrange(n):
                                    self.putSubItem(i, p.dereference())
                                    p += 1
                    continue

                else:
                    # A "normal" local variable or parameter.
                    with TopLevelItem(self, item.iname):
                        self.put('iname="%s",' % item.iname)
                        self.put('name="%s",' % item.name)
                        self.putItem(value)

        #
        # Watchers
        #
        with OutputSafer(self, ",", ""):
            if len(watchers) > 0:
                for watcher in watchers.split("##"):
                    (exp, iname) = watcher.split("#")
                    self.handleWatch(exp, iname)

        #print('data=[' + locals + sep + watchers + ']\n')

    def checkForQObjectBase(self, type):
        name = str(type)
        if name in qqQObjectCache:
            return qqQObjectCache[name]
        if name == self.ns + "QObject":
            qqQObjectCache[name] = True
            return True
        fields = type.strip_typedefs().fields()
        #fields = extractFields(type)
        if len(fields) == 0:
            qqQObjectCache[name] = False
            return False
        base = fields[0].type.strip_typedefs()
        if base.code != StructCode:
            return False
        # Prevent infinite recursion in Qt 3.3.8
        if str(base) == name:
            return False
        result = self.checkForQObjectBase(base)
        qqQObjectCache[name] = result
        return result


    def handleWatch(self, exp, iname):
        exp = str(exp)
        escapedExp = base64.b64encode(exp);
        #warn("HANDLING WATCH %s, INAME: '%s'" % (exp, iname))
        if exp.startswith("[") and exp.endswith("]"):
            #warn("EVAL: EXP: %s" % exp)
            with TopLevelItem(self, iname):
                self.put('iname="%s",' % iname)
                self.put('wname="%s",' % escapedExp)
                try:
                    list = eval(exp)
                    self.putValue("")
                    self.putNoType()
                    self.putNumChild(len(list))
                    # This is a list of expressions to evaluate
                    with Children(self, len(list)):
                        itemNumber = 0
                        for item in list:
                            self.handleWatch(item, "%s.%d" % (iname, itemNumber))
                            itemNumber += 1
                except RuntimeError, error:
                    warn("EVAL: ERROR CAUGHT %s" % error)
                    self.putValue("<syntax error>")
                    self.putNoType()
                    self.putNumChild(0)
                    self.put("children=[],")
            return

        with TopLevelItem(self, iname):
            self.put('iname="%s",' % iname)
            self.put('wname="%s",' % escapedExp)
            if len(exp) == 0: # The <Edit> case
                self.putValue(" ")
                self.putNoType()
                self.putNumChild(0)
            else:
                try:
                    value = parseAndEvaluate(exp)
                    self.putItem(value)
                except RuntimeError:
                    self.currentType = " "
                    self.currentValue = "<no such value>"
                    self.currentChildNumChild = -1
                    self.currentNumChild = 0
                    self.putNumChild(0)


    def put(self, value):
        self.output.append(value)

    def putField(self, name, value):
        self.put('%s="%s",' % (name, value))

    def childRange(self):
        if self.currentMaxNumChild is None:
            return xrange(0, self.currentNumChild)
        return xrange(min(self.currentMaxNumChild, self.currentNumChild))

    # Convenience function.
    def putItemCount(self, count, maximum = 1000000000):
        # This needs to override the default value, so don't use 'put' directly.
        if count > maximum:
            self.putValue('<>%s items>' % maximum)
        else:
            self.putValue('<%s items>' % count)

    def putType(self, type, priority = 0):
        # Higher priority values override lower ones.
        if priority >= self.currentTypePriority:
            self.currentType = str(type)
            self.currentTypePriority = priority

    def putNoType(self):
        # FIXME: replace with something that does not need special handling
        # in SubItem.__exit__().
        self.putBetterType(" ")

    def putInaccessible(self):
        #self.putBetterType(" ")
        self.putNumChild(0)
        self.currentValue = None

    def putBetterType(self, type, priority = 0):
        self.currentType = str(type)
        self.currentTypePriority = self.currentTypePriority + 1

    def putAddress(self, addr):
        if self.currentPrintsAddress:
            try:
                # addr can be "None", long(None) fails.
                self.put('addr="0x%x",' % long(addr))
            except:
                pass

    def putNumChild(self, numchild):
        #warn("NUM CHILD: '%s' '%s'" % (numchild, self.currentChildNumChild))
        if numchild != self.currentChildNumChild:
            self.put('numchild="%s",' % numchild)

    def putValue(self, value, encoding = None, priority = 0):
        # Higher priority values override lower ones.
        if priority >= self.currentValuePriority:
            self.currentValue = value
            self.currentValuePriority = priority
            self.currentValueEncoding = encoding

    def putPointerValue(self, value):
        # Use a lower priority
        if value is None:
            self.putValue(" ", None, -1)
        else:
            self.putValue("0x%x" % value.dereference().cast(
                lookupType("unsigned long")), None, -1)

    def putStringValue(self, value, priority = 0):
        if not value is None:
            str = encodeString(value)
            self.putValue(str, Hex4EncodedLittleEndian, priority)

    def putDisplay(self, format, value = None, cmd = None):
        self.put('editformat="%s",' % format)
        if cmd is None:
            if not value is None:
                self.put('editvalue="%s",' % value)
        else:
            self.put('editvalue="%s|%s",' % (cmd, value))

    def putByteArrayValue(self, value):
        str = encodeByteArray(value)
        self.putValue(str, Hex2EncodedLatin1)

    def putName(self, name):
        self.put('name="%s",' % name)

    def isExpanded(self):
        #warn("IS EXPANDED: %s in %s" % (item.iname, self.expandedINames))
        #if item.iname is None:
        #    raise "Illegal iname 'None'"
        #if item.iname.startswith("None"):
        #    raise "Illegal iname '%s'" % item.iname
        #warn("   --> %s" % (item.iname in self.expandedINames))
        return self.currentIName in self.expandedINames

    def isExpandedSubItem(self, component):
        iname = "%s.%s" % (self.currentIName, component)
        #warn("IS EXPANDED: %s in %s" % (iname, self.expandedINames))
        return iname in self.expandedINames

    def stripNamespaceFromType(self, typeName):
        type = stripClassTag(typeName)
        if len(self.ns) > 0 and type.startswith(self.ns):
            type = type[len(self.ns):]
        pos = type.find("<")
        # FIXME: make it recognize  foo<A>::bar<B>::iterator?
        while pos != -1:
            pos1 = type.rfind(">", pos)
            type = type[0:pos] + type[pos1+1:]
            pos = type.find("<")
        return type

    def isMovableType(self, type):
        if type.code == PointerCode:
            return True
        if isSimpleType(type):
            return True
        return self.stripNamespaceFromType(str(type)) in movableTypes

    def putIntItem(self, name, value):
        with SubItem(self, name):
            self.putValue(value)
            self.putAddress(value.address)
            self.putType("int")
            self.putNumChild(0)

    def putBoolItem(self, name, value):
        with SubItem(self, name):
            self.putValue(value)
            self.putType("bool")
            self.putNumChild(0)

    def currentItemFormat(self):
        format = self.formats.get(self.currentIName)
        if format is None:
            format = self.typeformats.get(stripClassTag(str(self.currentType)))
        return format

    def putSubItem(self, component, value):
        with SubItem(self, component):
            self.putItem(value)

    def putNamedSubItem(self, component, value, name):
        with SubItem(self, component):
            self.putName(name)
            self.putItem(value)

    def putCallItem(self, name, value, func, *args):
        result = call2(value, func, args)
        with SubItem(self, name):
            self.putItem(result)

    def putItem(self, value):
        if value is None:
            # Happens for non-available watchers in gdb versions that
            # need to use gdb.execute instead of gdb.parse_and_eval
            self.putValue("<not available>")
            self.putType("<unknown>")
            self.putNumChild(0)
            return

        type = value.type.unqualified()
        typeName = str(type)

        try:
            if value.is_optimized_out:
                self.putValue("<optimized out>")
                self.putType(typeName)
                self.putNumChild(0)
                return
        except:
            pass


        # FIXME: Gui shows references stripped?
        #warn(" ")
        #warn("REAL INAME: %s " % self.currentIName)
        #warn("REAL TYPE: %s " % value.type)
        #warn("REAL CODE: %s " % value.type.code)
        #warn("REAL VALUE: %s " % value)

        if type.code == ReferenceCode:
            #try:
                # This throws "RuntimeError: Attempt to dereference a
                # generic pointer." with MinGW's gcc 4.5 when it "identifies"
                # a "QWidget &" as "void &".
                self.putItem(value.cast(type.target()))
                return
            #except RuntimeError:
            #    pass

        if type.code == IntCode or type.code == CharCode:
            self.putAddress(value.address)
            self.putType(typeName)
            self.putValue(int(value))
            self.putNumChild(0)
            return

        if type.code == FloatCode or type.code == BoolCode:
            self.putAddress(value.address)
            self.putType(typeName)
            self.putValue(value)
            self.putNumChild(0)
            return

        if type.code == EnumCode:
            self.putType(typeName)
            self.putValue("%s (%d)" % (value, value))
            self.putNumChild(0)
            return

        if type.code == TypedefCode:
            self.putItem(value.cast(type.strip_typedefs()))
            self.putBetterType(typeName)
            return

        format = self.formats.get(self.currentIName)
        if format is None:
            format = self.typeformats.get(stripClassTag(typeName))

        if type.code == ArrayCode:
            targettype = type.target()
            self.putAddress(value.address)
            self.putType(typeName)
            self.putNumChild(1)
            format = self.currentItemFormat()
            if format == 0:
                # Explicitly requested Latin1 formatting.
                self.putValue(encodeCharArray(value, 100), Hex2EncodedLatin1)
            elif format == 1:
                # Explicitly requested UTF-8 formatting.
                self.putValue(encodeCharArray(value, 100), Hex2EncodedUtf8)
            elif format == 2:
                # Explicitly requested Local 8-bit formatting.
                self.putValue(encodeCharArray(value, 100), Hex2EncodedLocal8Bit)
            else:
                self.putValue("@0x%x" % long(value.cast(targettype.pointer())))
            if self.currentIName in self.expandedINames:
                i = 0
                with Children(self, childType=targettype,
                        addrBase=value.cast(targettype.pointer()),
                        addrStep=targettype.sizeof):
                    self.putFields(value)
                    i = i + 1
            return

        if type.code == PointerCode:
            #warn("POINTER: %s" % value)

            if not isAccessible(value):
                self.currentValue = None
                self.putNumChild(0)
                return

            if isNull(value):
                #warn("NULL POINTER")
                self.putAddress(value.address)
                self.putType(typeName)
                self.putValue("0x0")
                self.putNumChild(0)
                return

            innerType = type.target()
            innerTypeName = str(innerType.unqualified())

            if innerType.code == VoidCode:
                #warn("VOID POINTER: %s" % format)
                self.putType(typeName)
                self.putValue(str(value))
                self.putNumChild(0)
                self.putAddress(value.address)
                return

            if format == 0:
                # Explicitly requested bald pointer.
                self.putAddress(value.address)
                self.putType(typeName)
                self.putPointerValue(value.address)
                self.putNumChild(1)
                if self.currentIName in self.expandedINames:
                    with Children(self):
                        with SubItem(self, '*'):
                            self.putItem(value.dereference())
                            #self.putAddress(value)
                return

            if format == 1:
                # Explicityly requested Latin1 formatting.
                self.putAddress(value.address)
                self.putType(typeName)
                self.putValue(encodeCharArray(value, 100), Hex2EncodedLatin1)
                self.putNumChild(0)
                return

            if format == 2:
                # Explicityly requested UTF-8 formatting.
                self.putAddress(value.address)
                self.putType(typeName)
                self.putValue(encodeCharArray(value, 100), Hex2EncodedUtf8)
                self.putNumChild(0)
                return

            if format == 3:
                # Explicityly requested local 8 bit formatting.
                self.putAddress(value.address)
                self.putType(typeName)
                self.putValue(encodeCharArray(value, 100), Hex2EncodedLocal8Bit)
                self.putNumChild(0)
                return

            if format == 4:
                # Explitly requested UTF-16 formatting.
                self.putAddress(value.address)
                self.putType(typeName)
                self.putValue(encodeChar2Array(value, 100), Hex4EncodedBigEndian)
                self.putNumChild(0)
                return

            if format == 5:
                # Explitly requested UCS-4 formatting.
                self.putAddress(value.address)
                self.putType(typeName)
                self.putValue(encodeChar4Array(value, 100), Hex8EncodedBigEndian)
                self.putNumChild(0)
                return

            if (typeName.replace("(anonymous namespace)", "").find("(") != -1):
                # A function pointer with format None.
                self.putValue(str(value))
                self.putAddress(value.address)
                self.putType(typeName)
                self.putNumChild(0)
                return

            #warn("AUTODEREF: %s" % self.autoDerefPointers)
            #warn("INAME: %s" % self.currentIName)
            if self.autoDerefPointers or self.currentIName.endswith('.this'):
                ## Generic pointer type with format None
                #warn("GENERIC AUTODEREF POINTER: %s AT %s TO %s"
                #    % (type, value.address, innerTypeName))
                # Never dereference char types.
                if innerTypeName != "char" \
                        and innerTypeName != "signed char" \
                        and innerTypeName != "unsigned char"  \
                        and innerTypeName != "wchar_t":
                    self.putType(innerType)
                    savedCurrentChildType = self.currentChildType
                    self.currentChildType = stripClassTag(innerTypeName)
                    self.putItem(value.dereference())
                    self.currentChildType = savedCurrentChildType
                    self.putPointerValue(value.address)
                    self.put('origaddr="%s",' % value)
                    return

            # Fall back to plain pointer printing.
            #warn("GENERIC PLAIN POINTER: %s" % value.type)
            self.putType(typeName)
            self.putAddress(value.address)
            self.putNumChild(1)
            if self.currentIName in self.expandedINames:
                with Children(self):
                    with SubItem(self, "*"):
                        self.put('name="*",')
                        self.putItem(value.dereference())
            self.putPointerValue(value.address)
            return

        if typeName.startswith("<anon"):
            # Anonymous union. We need a dummy name to distinguish
            # multiple anonymous unions in the struct.
            self.putType(type)
            self.putValue("{...}")
            self.anonNumber += 1
            with Children(self, 1):
                self.listAnonymous(value, "#%d" % self.anonNumber, type)
            return

        if type.code != StructCode and type.code != UnionCode:
            warn("WRONG ASSUMPTION HERE: %s " % type.code)
            check(False)

        if self.useFancy and (format is None or format >= 1):
            self.putAddress(value.address)
            self.putType(typeName)

            if typeName in qqDumpers:
                qqDumpers[typeName](self, value)
                return

            nsStrippedType = self.stripNamespaceFromType(typeName)\
                .replace("::", "__")
            #warn(" STRIPPED: %s" % nsStrippedType)
            #warn(" DUMPERS: %s" % (nsStrippedType in qqDumpers))
            if nsStrippedType in qqDumpers:
                qqDumpers[nsStrippedType](self, value)
                return

            # Is this derived from QObject?
            if self.checkForQObjectBase(type):
                qdump__QObject(self, value)
                return

        #warn("GENERIC STRUCT: %s" % type)
        #warn("INAME: %s " % self.currentIName)
        #warn("INAMES: %s " % self.expandedINames)
        #warn("EXPANDED: %s " % (self.currentIName in self.expandedINames))
        fields = extractFields(type)
        #fields = type.fields()

        self.putType(typeName)
        self.putAddress(value.address)
        self.putValue("{...}")

        if False:
            numfields = 0
            for field in fields:
                bitpos = getattr(field, "bitpos", None)
                if not bitpos is None:
                    ++numfields
        else:
            numfields = len(fields)
        self.putNumChild(numfields)

        if self.currentIName in self.expandedINames:
            innerType = None
            if len(fields) == 1 and fields[0].name is None:
                innerType = type.target()
            with Children(self, 1, childType=innerType):
                self.putFields(value)

    def putPlainChildren(self, value):
        self.putValue(" ", None, -99)
        self.putNumChild(1)
        self.putAddress(value.address)
        if self.currentIName in self.expandedINames:
            with Children(self):
               self.putFields(value)

    def putFields(self, value, dumpBase = True):
            type = stripTypedefs(value.type)
            # Insufficient, see http://sourceware.org/bugzilla/show_bug.cgi?id=10953:
            #fields = type.fields()
            fields = extractFields(type)
            #warn("TYPE: %s" % type)
            #warn("FIELDS: %s" % fields)
            baseNumber = 0
            for field in fields:
                #warn("FIELD: %s" % field)
                #warn("  BITSIZE: %s" % field.bitsize)
                #warn("  ARTIFICIAL: %s" % field.artificial)
                bitpos = getattr(field, "bitpos", None)
                if bitpos is None: # FIXME: Is check correct?
                    continue  # A static class member(?).

                if field.name is None:
                    innerType = type.target()
                    p = value.cast(innerType.pointer())
                    for i in xrange(type.sizeof / innerType.sizeof):
                        with SubItem(self, i):
                            self.putItem(p.dereference())
                        p = p + 1
                    continue

                # Ignore vtable pointers for virtual inheritance.
                if field.name.startswith("_vptr."):
                    with SubItem(self, "[vptr]"):
                        # int (**)(void)
                        n = 20
                        self.putType(" ")
                        self.putValue(value[field.name])
                        self.putNumChild(n)
                        if self.isExpanded():
                            with Children(self):
                                p = value[field.name]
                                for i in xrange(n):
                                    if long(p.dereference()) != 0:
                                        with SubItem(self, i):
                                            self.putItem(p.dereference())
                                            self.putType(" ")
                                            p = p + 1
                    continue

                #warn("FIELD NAME: %s" % field.name)
                #warn("FIELD TYPE: %s" % field.type)
                if field.is_base_class:
                    # Field is base type. We cannot use field.name as part
                    # of the iname as it might contain spaces and other
                    # strange characters.
                    if dumpBase:
                        baseNumber += 1
                        with UnnamedSubItem(self, "@%d" % baseNumber):
                            self.put('iname="%s",' % self.currentIName)
                            self.put('name="%s",' % field.name)
                            self.putItem(value.cast(field.type))
                elif len(field.name) == 0:
                    # Anonymous union. We need a dummy name to distinguish
                    # multiple anonymous unions in the struct.
                    self.anonNumber += 1
                    self.listAnonymous(value, "#%d" % self.anonNumber,
                        field.type)
                else:
                    # Named field.
                    with SubItem(self, field.name):
                        #bitsize = getattr(field, "bitsize", None)
                        #if not bitsize is None:
                        #    self.put("bitsize=\"%s\",bitpos=\"%s\","
                        #            % (bitsize, bitpos))
                        self.putItem(upcast(value[field.name]))


    def listAnonymous(self, value, name, type):
        for field in type.fields():
            #warn("FIELD NAME: %s" % field.name)
            if len(field.name) > 0:
                with SubItem(self, field.name):
                    #self.putAddress(value.address)
                    self.putItem(value[field.name])
            else:
                # Further nested.
                self.anonNumber += 1
                name = "#%d" % self.anonNumber
                #iname = "%s.%s" % (selitem.iname, name)
                #child = SameItem(item.value, iname)
                with SubItem(self, name):
                    self.put('name="%s",' % name)
                    self.putValue(" ")
                    fieldTypeName = str(field.type)
                    if fieldTypeName.endswith("<anonymous union>"):
                        self.putType("<anonymous union>")
                    elif fieldTypeName.endswith("<anonymous struct>"):
                        self.putType("<anonymous struct>")
                    else:
                        self.putType(fieldTypeName)
                    with Children(self, 1):
                        self.listAnonymous(value, name, field.type)

#######################################################################
#
# ThreadNames Command
#
#######################################################################

def threadnames(arg):
    ns = qtNamespace()
    out = '['
    oldthread = gdb.selected_thread()
    try:
        for thread in gdb.inferiors()[0].threads():
            maximalStackDepth = int(arg)
            thread.switch()
            e = gdb.selected_frame ()
            while True:
                maximalStackDepth -= 1
                if maximalStackDepth < 0:
                    break
                e = e.older()
                if e == None or e.name() == None:
                    break
                if e.name() == ns + "QThreadPrivate::start":
                    try:
                        thrptr = e.read_var("thr").dereference()
                        obtype = lookupType(ns + "QObjectPrivate").pointer()
                        d_ptr = thrptr["d_ptr"]["d"].cast(obtype).dereference()
                        objectName = d_ptr["objectName"]
                        out += '{valueencoded="';
                        out += str(Hex4EncodedLittleEndianWithoutQuotes)+'",id="'
                        out += str(thread.num) + '",value="'
                        out += encodeString(objectName)
                        out += '"},'
                    except:
                        pass
    except:
        pass
    oldthread.switch()
    return out + ']'

registerCommand("threadnames", threadnames)



#######################################################################
#
# Mixed C++/Qml debugging
#
#######################################################################

def qmlb(args):
    # executeCommand(command, to_string=True).split("\n")
    warm("RUNNING: break -f QScript::FunctionWrapper::proxyCall")
    output = catchCliOutput("rbreak -f QScript::FunctionWrapper::proxyCall")
    warn("OUTPUT: %s " % output)
    bp = output[0]
    warn("BP: %s " % bp)
    # BP: ['Breakpoint 3 at 0xf166e7: file .../qscriptfunction.cpp, line 75.\\n'] \n"
    pos = bp.find(' ') + 1
    warn("POS: %s " % pos)
    nr = bp[bp.find(' ') + 1 : bp.find(' at ')]
    warn("NR: %s " % nr)
    return bp

registerCommand("qmlb", qmlb)
