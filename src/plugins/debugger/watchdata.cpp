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

#include "watchdata.h"

#include <QtCore/QTextStream>
#include <QtCore/QDebug>

////////////////////////////////////////////////////////////////////
//
// WatchData
//
////////////////////////////////////////////////////////////////////

namespace Debugger {
namespace Internal {

static QString htmlEscape(const QString &plain)
{
    QString rich;
    rich.reserve(int(plain.length() * 1.1));
    for (int i = 0; i < plain.length(); ++i) {
        if (plain.at(i) == QLatin1Char('<'))
            rich += QLatin1String("&lt;");
        else if (plain.at(i) == QLatin1Char('>'))
            rich += QLatin1String("&gt;");
        else if (plain.at(i) == QLatin1Char('&'))
            rich += QLatin1String("&amp;");
        else if (plain.at(i) == QLatin1Char('"'))
            rich += QLatin1String("&quot;");
        else
            rich += plain.at(i);
    }
    return rich;
}

bool isPointerType(const QByteArray &type)
{
    return type.endsWith('*') || type.endsWith("* const");
}

bool isCharPointerType(const QByteArray &type)
{
    return type == "char *" || type == "const char *" || type == "char const *";
}

bool isIntType(const QByteArray &type)
{
    if (type.isEmpty())
        return false;
    switch (type.at(0)) {
        case 'b':
            return type == "bool";
        case 'c':
            return type == "char";
        case 'i':
            return type == "int" || type == "int64";
        case 'l':
            return type == "long"
                || type.startsWith("long ");
        case 'p':
            return type == "ptrdiff_t";
        case 'q':
            return type == "qint16" || type == "quint16"
                || type == "qint32" || type == "quint32"
                || type == "qint64" || type == "quint64"
                || type == "qlonglong" || type == "qulonglong";
        case 's':
            return type == "short"
                || type == "signed"
                || type == "size_t"
                || type == "std::size_t"
                || type == "std::ptrdiff_t"
                || type.startsWith("signed ");
        case 'u':
            return type == "unsigned"
                || type.startsWith("unsigned ");
        default:
            return false;
    }
}

bool isFloatType(const QByteArray &type)
{
   return type == "float" || type == "double" || type == "qreal";
}

bool isIntOrFloatType(const QByteArray &type)
{
    return isIntType(type) || isFloatType(type);
}

WatchData::WatchData() :
    id(0),
    state(InitialState),
    editformat(0),
    address(0),
    referencingAddress(0),
    size(0),
    bitpos(0),
    bitsize(0),
    generation(-1),
    hasChildren(false),
    valueEnabled(true),
    valueEditable(true),
    error(false),
    changed(false),
    sortId(0),
    source(0)
{
}

bool WatchData::isEqual(const WatchData &other) const
{
    return iname == other.iname
      && exp == other.exp
      && name == other.name
      && value == other.value
      && editvalue == other.editvalue
      && valuetooltip == other.valuetooltip
      && type == other.type
      && displayedType == other.displayedType
      && variable == other.variable
      && address == other.address
      && size == other.size
      && hasChildren == other.hasChildren
      && valueEnabled == other.valueEnabled
      && valueEditable == other.valueEditable
      && error == other.error;
}

bool WatchData::isVTablePointer() const
{
    // First case: Cdb only. No user type can be named like this, this is safe.
    // Second case: Python dumper only.
    return type.startsWith("__fptr()")
        || (type.isEmpty() && name == QLatin1String("[vptr]"));
}

void WatchData::setError(const QString &msg)
{
    setAllUnneeded();
    value = msg;
    setHasChildren(false);
    valueEnabled = false;
    valueEditable = false;
    error = true;
}

void WatchData::setValue(const QString &value0)
{
    value = value0;
    if (value == QLatin1String("{...}")) {
        value.clear();
        hasChildren = true; // at least one...
    }
    // strip off quoted characters for chars.
    if (value.endsWith(QLatin1Char('\'')) && type.endsWith("char")) {
        const int blankPos = value.indexOf(QLatin1Char(' '));
        if (blankPos != -1)
            value.truncate(blankPos);
    }

    // avoid duplicated information
    if (value.startsWith(QLatin1Char('(')) && value.contains(QLatin1String(") 0x")))
        value.remove(0, value.lastIndexOf(QLatin1String(") 0x")) + 2);

    // doubles are sometimes displayed as "@0x6141378: 1.2".
    // I don't want that.
    if (/*isIntOrFloatType(type) && */ value.startsWith(QLatin1String("@0x"))
         && value.contains(QLatin1Char(':'))) {
        value.remove(0, value.indexOf(QLatin1Char(':')) + 2);
        setHasChildren(false);
    }

    // "numchild" is sometimes lying
    //MODEL_DEBUG("\n\n\nPOINTER: " << type << value);
    if (isPointerType(type))
        setHasChildren(value != QLatin1String("0x0") && value != QLatin1String("<null>")
            && !isCharPointerType(type));

    // pointer type information is available in the 'type'
    // column. No need to duplicate it here.
    if (value.startsWith(QLatin1Char('(') + QLatin1String(type) + QLatin1String(") 0x")))
        value = value.section(QLatin1Char(' '), -1, -1);

    setValueUnneeded();
}

void WatchData::setValueToolTip(const QString &tooltip)
{
    valuetooltip = tooltip;
}

enum GuessChildrenResult { HasChildren, HasNoChildren, HasPossiblyChildren };

static GuessChildrenResult guessChildren(const QByteArray &type)
{
    if (isIntOrFloatType(type))
        return HasNoChildren;
    if (isCharPointerType(type))
        return HasNoChildren;
    if (isPointerType(type))
        return HasChildren;
    if (type.endsWith("QString"))
        return HasNoChildren;
    return HasPossiblyChildren;
}

void WatchData::setType(const QByteArray &str, bool guessChildrenFromType)
{
    type = str.trimmed();
    bool changed = true;
    while (changed) {
        if (type.endsWith("const"))
            type.chop(5);
        else if (type.endsWith(' '))
            type.chop(1);
        else if (type.endsWith('&'))
            type.chop(1);
        else if (type.startsWith("const "))
            type = type.mid(6);
        else if (type.startsWith("volatile "))
            type = type.mid(9);
        else if (type.startsWith("class "))
            type = type.mid(6);
        else if (type.startsWith("struct "))
            type = type.mid(6);
        else if (type.startsWith(' '))
            type = type.mid(1);
        else
            changed = false;
    }
    setTypeUnneeded();
    if (guessChildrenFromType) {
        switch (guessChildren(type)) {
        case HasChildren:
            setHasChildren(true);
            break;
        case HasNoChildren:
            setHasChildren(false);
            break;
        case HasPossiblyChildren:
            setHasChildren(true); // FIXME: bold assumption
            break;
        }
    }
}

void WatchData::setAddress(const quint64 &a)
{
    address = a;
}

void WatchData::setHexAddress(const QByteArray &a)
{
    bool ok;
    const qint64 av = a.toULongLong(&ok, 0);
    if (ok) {
        address = av;
    } else {
        qWarning("WatchData::setHexAddress(): Failed to parse address value '%s' for '%s', '%s'",
                 a.constData(), iname.constData(), type.constData());
        address = 0;
    }
}

QString WatchData::toString() const
{
    const char *doubleQuoteComma = "\",";
    QString res;
    QTextStream str(&res);
    str << QLatin1Char('{');
    if (!iname.isEmpty())
        str << "iname=\"" << iname << doubleQuoteComma;
    str << "sortId=\"" << sortId << doubleQuoteComma;
    if (!name.isEmpty() && name != QLatin1String(iname))
        str << "name=\"" << name << doubleQuoteComma;
    if (error)
        str << "error,";
    if (address) {
        str.setIntegerBase(16);
        str << "addr=\"0x" << address << doubleQuoteComma;
        str.setIntegerBase(10);
    }
    if (referencingAddress) {
        str.setIntegerBase(16);
        str << "referencingaddr=\"0x" << referencingAddress << doubleQuoteComma;
        str.setIntegerBase(10);
    }
    if (!exp.isEmpty())
        str << "exp=\"" << exp << doubleQuoteComma;

    if (!variable.isEmpty())
        str << "variable=\"" << variable << doubleQuoteComma;

    if (isValueNeeded())
        str << "value=<needed>,";
    if (isValueKnown() && !value.isEmpty())
        str << "value=\"" << value << doubleQuoteComma;

    if (!editvalue.isEmpty())
        str << "editvalue=\"<...>\",";
    //    str << "editvalue=\"" << editvalue << doubleQuoteComma;

    if (!dumperFlags.isEmpty())
        str << "dumperFlags=\"" << dumperFlags << doubleQuoteComma;

    if (isTypeNeeded())
        str << "type=<needed>,";
    if (isTypeKnown() && !type.isEmpty())
        str << "type=\"" << type << doubleQuoteComma;

    if (isHasChildrenNeeded())
        str << "hasChildren=<needed>,";
    if (isHasChildrenKnown())
        str << "hasChildren=\"" << (hasChildren ? "true" : "false") << doubleQuoteComma;

    if (isChildrenNeeded())
        str << "children=<needed>,";
    str.flush();
    if (res.endsWith(QLatin1Char(',')))
        res.truncate(res.size() - 1);
    return res + QLatin1Char('}');
}

// Format a tooltip fow with aligned colon.
static void formatToolTipRow(QTextStream &str,
    const QString &category, const QString &value)
{
    str << "<tr><td>" << category << "</td><td> : </td><td>"
        << htmlEscape(value) << "</td></tr>";
}

QString WatchData::toToolTip() const
{
    if (!valuetooltip.isEmpty())
        return QString::number(valuetooltip.size());
    QString res;
    QTextStream str(&res);
    str << "<html><body><table>";
    formatToolTipRow(str, tr("Name"), name);
    formatToolTipRow(str, tr("Expression"), QLatin1String(exp));
    formatToolTipRow(str, tr("Internal Type"), QLatin1String(type));
    formatToolTipRow(str, tr("Displayed Type"), displayedType);
    QString val = value;
    if (value.size() > 1000) {
        val.truncate(1000);
        val += tr(" ... <cut off>");
    }
    formatToolTipRow(str, tr("Value"), val);
    formatToolTipRow(str, tr("Object Address"),
                     QString::fromAscii(hexAddress()));
    if (referencingAddress)
        formatToolTipRow(str, tr("Referencing Address"),
                         QString::fromAscii(hexReferencingAddress()));
    if (size)
        formatToolTipRow(str, tr("Size"), QString::number(size));
    formatToolTipRow(str, tr("Internal ID"), QLatin1String(iname));
    formatToolTipRow(str, tr("Generation"),
        QString::number(generation));
    str << "</table></body></html>";
    return res;
}

QString WatchData::msgNotInScope()
{
    //: Value of variable in Debugger Locals display for variables out
    //: of scope (stopped above initialization).
    static const QString rc =
        QCoreApplication::translate("Debugger::Internal::WatchData", "<not in scope>");
    return rc;
}

const QString &WatchData::shadowedNameFormat()
{
    //: Display of variables shadowed by variables of the same name
    //: in nested scopes: Variable %1 is the variable name, %2 is a
    //: simple count.
    static const QString format =
        QCoreApplication::translate("Debugger::Internal::WatchData", "%1 <shadowed %2>");
    return format;
}

QString WatchData::shadowedName(const QString &name, int seen)
{
    if (seen <= 0)
        return name;
    return shadowedNameFormat().arg(name).arg(seen);
}

quint64 WatchData::coreAddress() const
{
    return address;
}

QByteArray WatchData::hexAddress() const
{
    if (address)
        return QByteArray("0x") + QByteArray::number(address, 16);
    return QByteArray();
}

QByteArray WatchData::hexReferencingAddress() const
{
    if (referencingAddress)
        return QByteArray("0x") + QByteArray::number(referencingAddress, 16);
    return QByteArray();
}

bool WatchData::hasChanged(const WatchData &old) const
{
    return !value.isEmpty() && value != old.value && value != msgNotInScope();
}

} // namespace Internal
} // namespace Debugger

