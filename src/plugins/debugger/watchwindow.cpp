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

#include "watchwindow.h"

#include "breakhandler.h"
#include "registerhandler.h"
#include "debuggeractions.h"
#include "debuggerconstants.h"
#include "debuggerinternalconstants.h"
#include "debuggercore.h"
#include "debuggerdialogs.h"
#include "debuggerengine.h"
#include "debuggerstartparameters.h"
#include "watchdelegatewidgets.h"
#include "watchhandler.h"
#include "debuggertooltipmanager.h"
#include "memoryagent.h"
#include <utils/qtcassert.h>
#include <utils/savedaction.h>

#include <QtCore/QDebug>
#include <QtCore/QMetaObject>
#include <QtCore/QMetaProperty>
#include <QtCore/QVariant>

#include <QtGui/QApplication>
#include <QtGui/QPalette>
#include <QtGui/QClipboard>
#include <QtGui/QContextMenuEvent>
#include <QtGui/QHeaderView>
#include <QtGui/QItemDelegate>
#include <QtGui/QMenu>
#include <QtGui/QPainter>
#include <QtGui/QResizeEvent>
#include <QtGui/QInputDialog>
#include <QtGui/QMessageBox>

/////////////////////////////////////////////////////////////////////
//
// WatchDelegate
//
/////////////////////////////////////////////////////////////////////

namespace Debugger {
namespace Internal {

static DebuggerEngine *currentEngine()
{
    return debuggerCore()->currentEngine();
}

class WatchDelegate : public QItemDelegate
{
public:
    explicit WatchDelegate(WatchWindow *parent)
        : QItemDelegate(parent), m_watchWindow(parent)
    {}

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &,
        const QModelIndex &index) const
    {
        // Value column: Custom editor. Apply integer-specific settings.
        if (index.column() == 1) {
            const QVariant::Type type =
                static_cast<QVariant::Type>(index.data(LocalsEditTypeRole).toInt());
            switch (type) {
            case QVariant::Bool:
                return new BooleanComboBox(parent);
            default:
                break;
            }
            WatchLineEdit *edit = WatchLineEdit::create(type, parent);
            edit->setFrame(false);
            IntegerWatchLineEdit *intEdit
                = qobject_cast<IntegerWatchLineEdit *>(edit);
            if (intEdit)
                intEdit->setBase(index.data(LocalsIntegerBaseRole).toInt());
            return edit;
        }

        // Standard line edits for the rest.
        QLineEdit *lineEdit = new QLineEdit(parent);
        lineEdit->setFrame(false);
        return lineEdit;
    }

    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const
    {
        // Standard handling for anything but the watcher name column (change
        // expression), which removes/recreates a row, which cannot be done
        // in model->setData().
        if (index.column() != 0) {
            QItemDelegate::setModelData(editor, model, index);
            return;
        }
        const QMetaProperty userProperty = editor->metaObject()->userProperty();
        QTC_ASSERT(userProperty.isValid(), return);
        const QString value = editor->property(userProperty.name()).toString();
        const QString exp = index.data(LocalsExpressionRole).toString();
        if (exp == value)
            return;
        m_watchWindow->removeWatchExpression(exp);
        m_watchWindow->watchExpression(value);
    }

    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
        const QModelIndex &) const
    {
        editor->setGeometry(option.rect);
    }

private:
    WatchWindow *m_watchWindow;
};

// Watch model query helpers.
static inline quint64 addressOf(const QModelIndex &m)
{
    return m.data(LocalsAddressRole).toULongLong();
}

static inline quint64 pointerValueOf(const QModelIndex &m)
{
    return m.data(LocalsPointerValueRole).toULongLong();
}

static inline QString nameOf(const QModelIndex &m)
{
    return m.data().toString();
}

static inline QString typeOf(const QModelIndex &m)
{
    return m.data(LocalsTypeRole).toString();
}

static inline uint sizeOf(const QModelIndex &m)
{
    return m.data(LocalsSizeRole).toUInt();
}

// Create a map of value->name for register markup
typedef QMap<quint64, QString> RegisterMap;
typedef RegisterMap::const_iterator RegisterMapConstIt;

RegisterMap registerMap(const DebuggerEngine *engine)
{
    RegisterMap result;
    foreach (const Register &reg, engine->registerHandler()->registers()) {
        const QVariant v = reg.editValue();
        if (v.type() == QVariant::ULongLong)
            result.insert(v.toULongLong(), QString::fromAscii(reg.name));
    }
    return result;
}

// Helper functionality to indicate the area of a member variable in
// a vector representing the memory area by a unique color
// number and tooltip. Parts of it will be overwritten when recursing
// over the children.

typedef QPair<int, QString> ColorNumberToolTipPair;
typedef QVector<ColorNumberToolTipPair> ColorNumberToolTipVector;

static inline QString variableToolTip(const QString &name,
                                      const QString &type,
                                      quint64 offset)
{
    return offset ?
           //: HTML tooltip of a variable in the memory editor
           WatchWindow::tr("<i>%1</i> %2 at #%3").
               arg(type, name).arg(offset) :
           //: HTML tooltip of a variable in the memory editor
           WatchWindow::tr("<i>%1</i> %2").arg(type, name);
}

static int memberVariableRecursion(const QAbstractItemModel *model,
                                   const QModelIndex &modelIndex,
                                   const QString &name,
                                   quint64 start, quint64 end,
                                   int *colorNumberIn,
                                   ColorNumberToolTipVector *cnmv)
{
    int childCount = 0;
    // Recurse over top level items if modelIndex is invalid.
    const bool isRoot = !modelIndex.isValid();
    const int rowCount = isRoot ? model->rowCount() : modelIndex.model()->rowCount(modelIndex);
    if (!rowCount)
        return childCount;
    const QString nameRoot = name.isEmpty() ? name : name +  QLatin1Char('.');
    for (int r = 0; r < rowCount; r++) {
        const QModelIndex childIndex = isRoot ? model->index(r, 0) : modelIndex.child(r, 0);
        const quint64 childAddress = addressOf(childIndex);
        const uint childSize = sizeOf(childIndex);
        if (childAddress && childAddress >= start
                && (childAddress + childSize) <= end) { // Non-static, within area?
            const QString childName = nameRoot + nameOf(childIndex);
            const quint64 childOffset = childAddress - start;
            const QString toolTip
                = variableToolTip(childName, typeOf(childIndex), childOffset);
            const ColorNumberToolTipPair colorNumberNamePair((*colorNumberIn)++, toolTip);
            const ColorNumberToolTipVector::iterator begin = cnmv->begin() + childOffset;
            qFill(begin, begin + childSize, colorNumberNamePair);
            childCount++;
            childCount += memberVariableRecursion(model, childIndex, childName, start, end, colorNumberIn, cnmv);
        }
    }
    return childCount;
}

/*!
    \fn variableMemoryMarkup()

    \brief Creates markup for a variable in the memory view.

    Marks the visible children with alternating colors in the parent, that is, for
    \code
    struct Foo {
    char c1
    char c2
    int x2;
    QPair<int, int> pair
    }
    \endcode
    create something like:
    \code
    0 memberColor1
    1 memberColor2
    2 base color (padding area of parent)
    3 base color
    4 member color1
    ...
    8 memberColor2 (pair.first)
    ...
    12 memberColor1 (pair.second)
    \endcode

    In addition, registers pointing into the area are shown as 1 byte-markers.

   Fixme: When dereferencing a pointer, the size of the pointee is not
   known, currently. So, we take an area of 1024 and fill the background
   with the default color so that just the members are shown
   (sizeIsEstimate=true). This could be fixed by passing the pointee size
   as well from the debugger, but would require expensive type manipulation.

   \note To recurse over the top level items of the model, pass an invalid model
   index.

    \sa Debugger::Internal::MemoryViewWidget
*/

typedef QList<MemoryMarkup> MemoryMarkupList;

static inline MemoryMarkupList
    variableMemoryMarkup(const QAbstractItemModel *model,
                         const QModelIndex &modelIndex,
                         const QString &rootName,
                         const QString &rootToolTip,
                         quint64 address, quint64 size,
                         const RegisterMap &registerMap,
                         bool sizeIsEstimate,
                         const QColor &defaultBackground)
{
    enum { debug = 0 };
    enum { registerColorNumber = 0x3453 };

    if (debug)
        qDebug() << address << ' ' << size << rootName << rootToolTip;
    // Starting out from base, create an array representing the area
    // filled with base color. Fill children with some unique color numbers,
    // leaving the padding areas of the parent colored with the base color.
    MemoryMarkupList result;
    int colorNumber = 0;
    ColorNumberToolTipVector ranges(size, ColorNumberToolTipPair(colorNumber, rootToolTip));
    const int childCount = memberVariableRecursion(model, modelIndex,
                                                   rootName, address, address + size,
                                                   &colorNumber, &ranges);
    if (sizeIsEstimate && !childCount)
        return result; // Fixme: Exact size not known, no point in filling if no children.
    // Punch in registers as 1-byte markers on top.
    const RegisterMapConstIt regcEnd = registerMap.constEnd();
    for (RegisterMapConstIt it = registerMap.constBegin(); it != regcEnd; ++it) {
        if (it.key() >= address) {
            const quint64 offset = it.key() - address;
            if (offset < size) {
                ranges[offset] =
                    ColorNumberToolTipPair(registerColorNumber,
                                           WatchWindow::tr("Register <i>%1</i>").arg(it.value()));
            } else {
                break; // Sorted.
            }
        }
    } // for registers.
    if (debug) {
        QDebug dbg = qDebug().nospace();
        dbg << rootToolTip << ' ' << address << ' ' << size << '\n';
        QString name;
        for (unsigned i = 0; i < size; ++i)
            if (name != ranges.at(i).second) {
                dbg << ",[" << i << ' ' << ranges.at(i).first << ' ' << ranges.at(i).second << ']';
                name = ranges.at(i).second;
            }
    }

    // Condense ranges of identical color into markup ranges. Assign member colors
    // interchangeably.
    const QColor baseColor = sizeIsEstimate ? defaultBackground : Qt::lightGray;
    const QColor registerColor = Qt::green;
    QColor memberColor1 = QColor(Qt::yellow).lighter();
    QColor memberColor2 = QColor(Qt::cyan).lighter();

    int lastColorNumber = 0;
    int childNumber = 0;
    for (unsigned i = 0; i < size; ++i) {
        const ColorNumberToolTipPair &range = ranges.at(i);
        if (result.isEmpty() || lastColorNumber != range.first) {
            lastColorNumber = range.first;
            // Base colors: Parent/register
            QColor color = range.first == registerColorNumber ? registerColor : baseColor;
            if (range.first && range.first != registerColorNumber) {
                if (childNumber++ & 1) { // Alternating member colors.
                    color = memberColor1;
                    memberColor1 = memberColor1.darker(120);
                } else {
                    color = memberColor2;
                    memberColor2 = memberColor2.darker(120);
                }
            } // color switch
            result.push_back(MemoryMarkup(address + i, 1, color, range.second));
        } else {
            result.back().length++;
        }
    }

    if (debug) {
        QDebug dbg = qDebug().nospace();
        dbg << rootName << ' ' << address << ' ' << size << '\n';
        QString name;
        for (unsigned i = 0; i < size; ++i)
            if (name != ranges.at(i).second) {
                dbg << ',' << i << ' ' << ranges.at(i).first << ' ' << ranges.at(i).second;
                name = ranges.at(i).second;
            }
        dbg << '\n';
        foreach (const MemoryMarkup &m, result)
            dbg << m.address <<  ' ' << m.length << ' '  << m.toolTip << '\n';
    }

    return result;
}

// Convenience to create a memory view of a variable.
static void addVariableMemoryView(DebuggerEngine *engine,
                                  bool separateView,
                                  const QModelIndex &m, bool deferencePointer,
                                  const QPoint &p,
                                  QWidget *parent)
{
    const QColor background = parent->palette().color(QPalette::Normal, QPalette::Base);
    const quint64 address = deferencePointer ? pointerValueOf(m) : addressOf(m);
    // Fixme: Get the size of pointee (see variableMemoryMarkup())?
    const QString rootToolTip = variableToolTip(nameOf(m), typeOf(m), 0);
    const quint64 typeSize = sizeOf(m);
    const bool sizeIsEstimate = deferencePointer || !typeSize;
    const quint64 size    = sizeIsEstimate ? 1024 : typeSize;
    if (!address)
         return;
    const QList<MemoryMarkup> markup =
        variableMemoryMarkup(m.model(), m, nameOf(m), rootToolTip,
                             address, size,
                             registerMap(engine),
                             sizeIsEstimate, background);
    const unsigned flags = separateView ? (DebuggerEngine::MemoryView|DebuggerEngine::MemoryReadOnly) : 0;
    const QString title = deferencePointer ?
    WatchWindow::tr("Memory Referenced by Pointer '%1' (0x%2)").arg(nameOf(m)).arg(address, 0, 16) :
    WatchWindow::tr("Memory at Variable '%1' (0x%2)").arg(nameOf(m)).arg(address, 0, 16);
    engine->openMemoryView(address, flags, markup, p, title, parent);
}

// Add a memory view of the stack layout showing local variables
// and registers.
static inline void addStackLayoutMemoryView(DebuggerEngine *engine,
                                            bool separateView,
                                            const QAbstractItemModel *m, const QPoint &p,
                                            QWidget *parent)
{
    typedef QPair<quint64, QString> RegisterValueNamePair;

    // Determine suitable address range from locals
    quint64 start = Q_UINT64_C(0xFFFFFFFFFFFFFFFF);
    quint64 end = 0;
    const int rootItemCount = m->rowCount();
    // Note: Unsorted by default. Exclude 'Automatically dereferenced
    // pointer' items as they are outside the address range.
    for (int r = 0; r < rootItemCount; r++) {
        const QModelIndex idx = m->index(r, 0);
        if (idx.data(LocalsReferencingAddressRole).toULongLong() == 0) {
            const quint64 address = addressOf(idx);
            if (address) {
                if (address < start)
                    start = address;
                if (const uint size = sizeOf(idx))
                    if (address + size > end)
                        end = address + size;
            }
        }
    }
    // Anything found and everything in a sensible range (static data in-between)?
    if (end <= start || end - start > 100 * 1024) {
        QMessageBox::information(parent, WatchWindow::tr("Cannot Display Stack Layout"),
            WatchWindow::tr("Could not determine a suitable address range."));
        return;
    }
    // Take a look at the register values. Extend the range a bit if suitable
    // to show stack/stack frame pointers.
    const RegisterMap regMap = registerMap(engine);
    const RegisterMapConstIt regcEnd = regMap.constEnd();
    for (RegisterMapConstIt it = regMap.constBegin(); it != regcEnd; ++it) {
        const quint64 value = it.key();
        if (value < start && start - value < 512) {
            start = value;
        } else if (value > end && value - end < 512) {
            end = value + 1;
        }
    }
    // Indicate all variables.
    const QColor background = parent->palette().color(QPalette::Normal, QPalette::Base);
    const MemoryMarkupList markup =
        variableMemoryMarkup(m, QModelIndex(), QString(),
                             QString(), start, end - start,
                             regMap, true, background);
    const unsigned flags = separateView ? (DebuggerEngine::MemoryView|DebuggerEngine::MemoryReadOnly) : 0;
    const QString title =
        WatchWindow::tr("Memory Layout of Local Variables at 0x%1").arg(start, 0, 16);
    engine->openMemoryView(start, flags, markup, p, title, parent);
}

/////////////////////////////////////////////////////////////////////
//
// WatchWindow
//
/////////////////////////////////////////////////////////////////////

WatchWindow::WatchWindow(Type type, QWidget *parent)
  : QTreeView(parent),
    m_type(type)
{
    setObjectName(QLatin1String("WatchWindow"));
    m_grabbing = false;

    setFrameStyle(QFrame::NoFrame);
    setAttribute(Qt::WA_MacShowFocusRect, false);
    setWindowTitle(tr("Locals and Expressions"));
    setIndentation(indentation() * 9/10);
    setUniformRowHeights(true);
    setItemDelegate(new WatchDelegate(this));
    setDragEnabled(true);
    setAcceptDrops(true);
    setDropIndicatorShown(true);

    QAction *useColors = debuggerCore()->action(UseAlternatingRowColors);
    setAlternatingRowColors(useColors->isChecked());

    QAction *adjustColumns = debuggerCore()->action(AlwaysAdjustLocalsColumnWidths);

    connect(useColors, SIGNAL(toggled(bool)),
        SLOT(setAlternatingRowColorsHelper(bool)));
    connect(adjustColumns, SIGNAL(triggered(bool)),
        SLOT(setAlwaysResizeColumnsToContents(bool)));
    connect(this, SIGNAL(expanded(QModelIndex)),
        SLOT(expandNode(QModelIndex)));
    connect(this, SIGNAL(collapsed(QModelIndex)),
        SLOT(collapseNode(QModelIndex)));
}

void WatchWindow::expandNode(const QModelIndex &idx)
{
    setModelData(LocalsExpandedRole, true, idx);
}

void WatchWindow::collapseNode(const QModelIndex &idx)
{
    setModelData(LocalsExpandedRole, false, idx);
}

void WatchWindow::keyPressEvent(QKeyEvent *ev)
{
    if (ev->key() == Qt::Key_Delete && m_type == WatchersType) {
        QModelIndex idx = currentIndex();
        QModelIndex idx1 = idx.sibling(idx.row(), 0);
        QString exp = idx1.data(LocalsRawExpressionRole).toString();
        removeWatchExpression(exp);
    } else if (ev->key() == Qt::Key_Return
            && ev->modifiers() == Qt::ControlModifier
            && m_type == LocalsType) {
        QModelIndex idx = currentIndex();
        QModelIndex idx1 = idx.sibling(idx.row(), 0);
        QString exp = model()->data(idx1).toString();
        watchExpression(exp);
    }
    QTreeView::keyPressEvent(ev);
}

void WatchWindow::dragEnterEvent(QDragEnterEvent *ev)
{
    //QTreeView::dragEnterEvent(ev);
    if (ev->mimeData()->hasFormat("text/plain")) {
        ev->setDropAction(Qt::CopyAction);
        ev->accept();
    }
}

void WatchWindow::dragMoveEvent(QDragMoveEvent *ev)
{
    //QTreeView::dragMoveEvent(ev);
    if (ev->mimeData()->hasFormat("text/plain")) {
        ev->setDropAction(Qt::CopyAction);
        ev->accept();
    }
}

void WatchWindow::dropEvent(QDropEvent *ev)
{
    if (ev->mimeData()->hasFormat("text/plain")) {
        watchExpression(ev->mimeData()->text());
        //ev->acceptProposedAction();
        ev->setDropAction(Qt::CopyAction);
        ev->accept();
    }
    //QTreeView::dropEvent(ev);
}

void WatchWindow::mouseDoubleClickEvent(QMouseEvent *ev)
{
    const QModelIndex idx = indexAt(ev->pos());
    if (!idx.isValid()) {
        // The "<Edit>" case.
        watchExpression(QString());
        return;
    }
    QTreeView::mouseDoubleClickEvent(ev);
}

// Text for add watch action with truncated expression
static inline QString addWatchActionText(QString exp)
{
    if (exp.isEmpty())
        return WatchWindow::tr("Evaluate Expression");
    if (exp.size() > 30) {
        exp.truncate(30);
        exp.append(QLatin1String("..."));
    }
    return WatchWindow::tr("Evaluate Expression \"%1\"").arg(exp);
}

// Text for add watch action with truncated expression
static inline QString removeWatchActionText(QString exp)
{
    if (exp.isEmpty())
        return WatchWindow::tr("Remove Evaluated Expression");
    if (exp.size() > 30) {
        exp.truncate(30);
        exp.append(QLatin1String("..."));
    }
    return WatchWindow::tr("Remove Evaluated Expression \"%1\"").arg(exp);
}

static inline void copyToClipboard(const QString &clipboardText)
{
    QClipboard *clipboard = QApplication::clipboard();
#ifdef Q_WS_X11
    clipboard->setText(clipboardText, QClipboard::Selection);
#endif
    clipboard->setText(clipboardText, QClipboard::Clipboard);
}

void WatchWindow::contextMenuEvent(QContextMenuEvent *ev)
{
    DebuggerEngine *engine = currentEngine();
    WatchHandler *handler = engine->watchHandler();

    const QModelIndex idx = indexAt(ev->pos());
    const QModelIndex mi0 = idx.sibling(idx.row(), 0);
    const QModelIndex mi1 = idx.sibling(idx.row(), 1);
    const QModelIndex mi2 = idx.sibling(idx.row(), 2);
    const quint64 address = addressOf(mi0);
    const uint size = sizeOf(mi0);
    const quint64 pointerValue = pointerValueOf(mi0);
    const QString exp = mi0.data(LocalsExpressionRole).toString();
    const QString name = mi0.data(LocalsNameRole).toString();
    const QString type = mi2.data().toString();

    // Offer to open address pointed to or variable address.
    const bool createPointerActions = pointerValue && pointerValue != address;

    const QStringList alternativeFormats =
        mi0.data(LocalsTypeFormatListRole).toStringList();
    const int typeFormat =
        mi0.data(LocalsTypeFormatRole).toInt();
    const int individualFormat =
        mi0.data(LocalsIndividualFormatRole).toInt();
    const int effectiveIndividualFormat =
        individualFormat == -1 ? typeFormat : individualFormat;
    const int unprintableBase = handler->unprintableBase();

    QMenu formatMenu;
    QList<QAction *> typeFormatActions;
    QList<QAction *> individualFormatActions;
    QAction *clearTypeFormatAction = 0;
    QAction *clearIndividualFormatAction = 0;
    QAction *showUnprintableUnicode = 0;
    QAction *showUnprintableOctal = 0;
    QAction *showUnprintableHexadecimal = 0;
    formatMenu.setTitle(tr("Change Display Format..."));
    showUnprintableUnicode =
        formatMenu.addAction(tr("Treat All Characters as Printable"));
    showUnprintableUnicode->setCheckable(true);
    showUnprintableUnicode->setChecked(unprintableBase == 0);
    showUnprintableOctal =
        formatMenu.addAction(tr("Show Unprintable Characters as Octal"));
    showUnprintableOctal->setCheckable(true);
    showUnprintableOctal->setChecked(unprintableBase == 8);
    showUnprintableHexadecimal =
        formatMenu.addAction(tr("Show Unprintable Characters as Hexadecimal"));
    showUnprintableHexadecimal->setCheckable(true);
    showUnprintableHexadecimal->setChecked(unprintableBase == 16);
    if (idx.isValid() /*&& !alternativeFormats.isEmpty() */) {
        const QString spacer = QLatin1String("     ");
        formatMenu.addSeparator();
        QAction *dummy = formatMenu.addAction(
            tr("Change Display for Object Named \"%1\":").arg(mi0.data().toString()));
        dummy->setEnabled(false);
        clearIndividualFormatAction
            = formatMenu.addAction(spacer + tr("Use Display Format Based on Type"));
        //clearIndividualFormatAction->setEnabled(individualFormat != -1);
        clearIndividualFormatAction->setCheckable(true);
        clearIndividualFormatAction->setChecked(effectiveIndividualFormat == -1);
        for (int i = 0; i != alternativeFormats.size(); ++i) {
            const QString format = spacer + alternativeFormats.at(i);
            QAction *act = new QAction(format, &formatMenu);
            act->setCheckable(true);
            if (i == effectiveIndividualFormat)
                act->setChecked(true);
            formatMenu.addAction(act);
            individualFormatActions.append(act);
        }
        formatMenu.addSeparator();
        dummy = formatMenu.addAction(
            tr("Change Display for Type \"%1\":").arg(type));
        dummy->setEnabled(false);
        clearTypeFormatAction = formatMenu.addAction(spacer + tr("Automatic"));
        //clearTypeFormatAction->setEnabled(typeFormat != -1);
        //clearTypeFormatAction->setEnabled(individualFormat != -1);
        clearTypeFormatAction->setCheckable(true);
        clearTypeFormatAction->setChecked(typeFormat == -1);
        for (int i = 0; i != alternativeFormats.size(); ++i) {
            const QString format = spacer + alternativeFormats.at(i);
            QAction *act = new QAction(format, &formatMenu);
            act->setCheckable(true);
            //act->setEnabled(individualFormat != -1);
            if (i == typeFormat)
                act->setChecked(true);
            formatMenu.addAction(act);
            typeFormatActions.append(act);
        }
    } else {
        QAction *dummy = formatMenu.addAction(
            tr("Change Display for Type or Item..."));
        dummy->setEnabled(false);
    }

    const bool actionsEnabled = engine->debuggerActionsEnabled();
    const unsigned engineCapabilities = engine->debuggerCapabilities();
    const bool canHandleWatches = engineCapabilities & AddWatcherCapability;
    const DebuggerState state = engine->state();
    const bool canInsertWatches = state == InferiorStopOk
        || (state == InferiorRunOk && engine->acceptsWatchesWhileRunning());

    QMenu breakpointMenu;
    breakpointMenu.setTitle(tr("Add Data Breakpoint..."));
    QAction *actSetWatchpointAtVariableAddress = 0;
    QAction *actSetWatchpointAtPointerValue = 0;
    const bool canSetWatchpoint = engineCapabilities & WatchpointByAddressCapability;
    if (canSetWatchpoint && address) {
        actSetWatchpointAtVariableAddress =
            new QAction(tr("Add Data Breakpoint at Object's Address (0x%1)")
                .arg(address, 0, 16), &breakpointMenu);
        actSetWatchpointAtVariableAddress->
            setChecked(mi0.data(LocalsIsWatchpointAtAddressRole).toBool());
        if (createPointerActions) {
            actSetWatchpointAtPointerValue =
                new QAction(tr("Add Data Breakpoint at Referenced Address (0x%1)")
                    .arg(pointerValue, 0, 16), &breakpointMenu);
            actSetWatchpointAtPointerValue->setCheckable(true);
            actSetWatchpointAtPointerValue->
                setChecked(mi0.data(LocalsIsWatchpointAtPointerValueRole).toBool());
        }
    } else {
        actSetWatchpointAtVariableAddress =
            new QAction(tr("Add Data Breakpoint"), &breakpointMenu);
        actSetWatchpointAtVariableAddress->setEnabled(false);
    }
    actSetWatchpointAtVariableAddress->setToolTip(
        tr("Setting a data breakpoint on an address will cause the program "
           "to stop when the data at the address is modified."));

    QAction *actSetWatchpointAtExpression =
            new QAction(tr("Add Data Breakpoint at Expression \"%1\"").arg(name),
                &breakpointMenu);
    actSetWatchpointAtExpression->setToolTip(
        tr("Setting a data breakpoint on an expression will cause the program "
           "to stop when the data at the address given by the expression "
           "is modified."));

    breakpointMenu.addAction(actSetWatchpointAtVariableAddress);
    if (actSetWatchpointAtPointerValue)
        breakpointMenu.addAction(actSetWatchpointAtPointerValue);
    breakpointMenu.addAction(actSetWatchpointAtExpression);

    QMenu menu;
    QAction *actInsertNewWatchItem = menu.addAction(tr("Insert New Evaluated Expression"));
    actInsertNewWatchItem->setEnabled(canHandleWatches && canInsertWatches);
    QAction *actSelectWidgetToWatch = menu.addAction(tr("Select Widget to Watch"));
    actSelectWidgetToWatch->setEnabled(canHandleWatches && (engine->canWatchWidgets()));

    menu.addSeparator();

    QAction *actWatchExpression = new QAction(addWatchActionText(exp), &menu);
    actWatchExpression->setEnabled(canHandleWatches && !exp.isEmpty());

    // Can remove watch if engine can handle it or session engine.
    QAction *actRemoveWatchExpression = new QAction(removeWatchActionText(exp), &menu);
    actRemoveWatchExpression->setEnabled(
        (canHandleWatches || state == DebuggerNotReady) && !exp.isEmpty());
    QAction *actRemoveWatches = new QAction(tr("Remove All Watch Items"), &menu);
    actRemoveWatches->setEnabled(!WatchHandler::watcherNames().isEmpty());

    if (m_type == LocalsType)
        menu.addAction(actWatchExpression);
    else {
        menu.addAction(actRemoveWatchExpression);
        menu.addAction(actRemoveWatches);
    }

    QMenu memoryMenu;
    memoryMenu.setTitle(tr("Open Memory Editor..."));
    QAction *actOpenMemoryEditAtVariableAddress = new QAction(&memoryMenu);
    QAction *actOpenMemoryEditAtPointerValue = new QAction(&memoryMenu);
    QAction *actOpenMemoryEditor = new QAction(&memoryMenu);
    QAction *actOpenMemoryEditorStackLayout = new QAction(&memoryMenu);
    QAction *actOpenMemoryViewAtVariableAddress = new QAction(&memoryMenu);
    QAction *actOpenMemoryViewAtPointerValue = new QAction(&memoryMenu);
    if (engineCapabilities & ShowMemoryCapability) {
        actOpenMemoryEditor->setText(tr("Open Memory Editor..."));
        if (address) {
            actOpenMemoryEditAtVariableAddress->setText(
                tr("Open Memory Editor at Object's Address (0x%1)")
                    .arg(address, 0, 16));
            actOpenMemoryViewAtVariableAddress->setText(
                    tr("Open Memory View at Object's Address (0x%1)")
                        .arg(address, 0, 16));
        } else {
            actOpenMemoryEditAtVariableAddress->setText(
                tr("Open Memory Editor at Object's Address"));
            actOpenMemoryEditAtVariableAddress->setEnabled(false);
            actOpenMemoryViewAtVariableAddress->setText(
                    tr("Open Memory View at Object's Address"));
            actOpenMemoryViewAtVariableAddress->setEnabled(false);
        }
        if (createPointerActions) {
            actOpenMemoryEditAtPointerValue->setText(
                tr("Open Memory Editor at Referenced Address (0x%1)")
                    .arg(pointerValue, 0, 16));
            actOpenMemoryViewAtPointerValue->setText(
                tr("Open Memory View at Referenced Address (0x%1)")
                    .arg(pointerValue, 0, 16));
        } else {
            actOpenMemoryEditAtPointerValue->setText(
                tr("Open Memory Editor at Referenced Address"));
            actOpenMemoryEditAtPointerValue->setEnabled(false);
            actOpenMemoryViewAtPointerValue->setText(
                tr("Open Memory View at Referenced Address"));
            actOpenMemoryViewAtPointerValue->setEnabled(false);
        }
        actOpenMemoryEditorStackLayout->setText(tr("Open Memory Editor Showing Stack Layout"));
        actOpenMemoryEditorStackLayout->setEnabled(m_type == LocalsType);
        memoryMenu.addAction(actOpenMemoryViewAtVariableAddress);
        memoryMenu.addAction(actOpenMemoryViewAtPointerValue);
        memoryMenu.addAction(actOpenMemoryEditAtVariableAddress);
        memoryMenu.addAction(actOpenMemoryEditAtPointerValue);
        memoryMenu.addAction(actOpenMemoryEditorStackLayout);
        memoryMenu.addAction(actOpenMemoryEditor);
    } else {
        memoryMenu.setEnabled(false);
    }

    QAction *actCopy = new QAction(tr("Copy Contents to Clipboard"), &menu);
    QAction *actCopyValue = new QAction(tr("Copy Value to Clipboard"), &menu);
    actCopyValue->setEnabled(idx.isValid());


    menu.addAction(actInsertNewWatchItem);
    menu.addAction(actSelectWidgetToWatch);
    menu.addMenu(&formatMenu);
    menu.addMenu(&memoryMenu);
    menu.addMenu(&breakpointMenu);
    menu.addAction(actCopy);
    menu.addAction(actCopyValue);
    menu.addSeparator();

    menu.addAction(debuggerCore()->action(UseDebuggingHelpers));
    menu.addAction(debuggerCore()->action(UseToolTipsInLocalsView));
    menu.addAction(debuggerCore()->action(AutoDerefPointers));
    menu.addAction(debuggerCore()->action(ShowStdNamespace));
    menu.addAction(debuggerCore()->action(ShowQtNamespace));
    menu.addAction(debuggerCore()->action(SortStructMembers));

    QAction *actAdjustColumnWidths =
        menu.addAction(tr("Adjust Column Widths to Contents"));
    menu.addAction(debuggerCore()->action(AlwaysAdjustLocalsColumnWidths));
    menu.addSeparator();

    QAction *actClearCodeModelSnapshot
        = new QAction(tr("Refresh Code Model Snapshot"), &menu);
    actClearCodeModelSnapshot->setEnabled(actionsEnabled
        && debuggerCore()->action(UseCodeModel)->isChecked());
    menu.addAction(actClearCodeModelSnapshot);
    QAction *actShowInEditor
        = new QAction(tr("Show View Contents in Editor"), &menu);
    actShowInEditor->setEnabled(actionsEnabled);
    menu.addAction(actShowInEditor);
    menu.addAction(debuggerCore()->action(SettingsDialog));

    QAction *actCloseEditorToolTips = new QAction(tr("Close Editor Tooltips"), &menu);
    actCloseEditorToolTips->setEnabled(DebuggerToolTipManager::instance()->hasToolTips());
    menu.addAction(actCloseEditorToolTips);

    QAction *act = menu.exec(ev->globalPos());
    if (act == 0)
        return;

    if (act == actAdjustColumnWidths) {
        resizeColumnsToContents();
    } else if (act == actInsertNewWatchItem) {
        bool ok;
        QString newExp = QInputDialog::getText(this, tr("Enter watch expression"),
                                   tr("Expression:"), QLineEdit::Normal,
                                   QString(), &ok);
        if (ok && !newExp.isEmpty()) {
            watchExpression(newExp);
        }
    } else if (act == actOpenMemoryEditAtVariableAddress) {
        addVariableMemoryView(currentEngine(), false, mi0, false, ev->globalPos(), this);
    } else if (act == actOpenMemoryEditAtPointerValue) {
        addVariableMemoryView(currentEngine(), false, mi0, true, ev->globalPos(), this);
    } else if (act == actOpenMemoryEditor) {
        AddressDialog dialog;
        if (address)
            dialog.setAddress(address);
        if (dialog.exec() == QDialog::Accepted)
            currentEngine()->openMemoryView(dialog.address(), false, MemoryMarkupList(), QPoint());
    } else if (act == actOpenMemoryViewAtVariableAddress) {
        addVariableMemoryView(currentEngine(), true, mi0, false, ev->globalPos(), this);
    } else if (act == actOpenMemoryViewAtPointerValue) {
        addVariableMemoryView(currentEngine(), true, mi0, true, ev->globalPos(), this);
    } else if (act == actOpenMemoryEditorStackLayout) {
        addStackLayoutMemoryView(currentEngine(), false, mi0.model(), ev->globalPos(), this);
    } else if (act == actSetWatchpointAtVariableAddress) {
        setWatchpointAtAddress(address, size);
    } else if (act == actSetWatchpointAtPointerValue) {
        setWatchpointAtAddress(pointerValue, sizeof(void *)); // FIXME: an approximation..
    } else if (act == actSetWatchpointAtExpression) {
        setWatchpointAtExpression(name);
    } else if (act == actSelectWidgetToWatch) {
        grabMouse(Qt::CrossCursor);
        m_grabbing = true;
    } else if (act == actWatchExpression) {
        watchExpression(exp);
    } else if (act == actRemoveWatchExpression) {
        removeWatchExpression(exp);
    } else if (act == actCopy) {
        copyToClipboard(DebuggerToolTipWidget::treeModelClipboardContents(model()));
    } else if (act == actCopyValue) {
        copyToClipboard(mi1.data().toString());
    } else if (act == actRemoveWatches) {
        currentEngine()->watchHandler()->clearWatches();
    } else if (act == actClearCodeModelSnapshot) {
        debuggerCore()->clearCppCodeModelSnapshot();
    } else if (act == clearTypeFormatAction) {
        setModelData(LocalsTypeFormatRole, -1, mi1);
    } else if (act == clearIndividualFormatAction) {
        setModelData(LocalsIndividualFormatRole, -1, mi1);
    } else if (act == actShowInEditor) {
        QString contents = handler->editorContents();
        debuggerCore()->openTextEditor(tr("Locals & Watchers"), contents);
    } else if (act == showUnprintableUnicode) {
        handler->setUnprintableBase(0);
    } else if (act == showUnprintableOctal) {
        handler->setUnprintableBase(8);
    } else if (act == showUnprintableHexadecimal) {
        handler->setUnprintableBase(16);
    } else if (act == actCloseEditorToolTips) {
        DebuggerToolTipManager::instance()->closeAllToolTips();
    } else {
        for (int i = 0; i != typeFormatActions.size(); ++i) {
            if (act == typeFormatActions.at(i))
                setModelData(LocalsTypeFormatRole, i, mi1);
        }
        for (int i = 0; i != individualFormatActions.size(); ++i) {
            if (act == individualFormatActions.at(i))
                setModelData(LocalsIndividualFormatRole, i, mi1);
        }
    }
}

void WatchWindow::resizeColumnsToContents()
{
    resizeColumnToContents(0);
    resizeColumnToContents(1);
}

void WatchWindow::setAlwaysResizeColumnsToContents(bool on)
{
    if (!header())
        return;
    QHeaderView::ResizeMode mode = on
        ? QHeaderView::ResizeToContents : QHeaderView::Interactive;
    header()->setResizeMode(0, mode);
    header()->setResizeMode(1, mode);
}

bool WatchWindow::event(QEvent *ev)
{
    if (m_grabbing && ev->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mev = static_cast<QMouseEvent *>(ev);
        m_grabbing = false;
        releaseMouse();
        currentEngine()->watchPoint(mapToGlobal(mev->pos()));
    }
    return QTreeView::event(ev);
}

void WatchWindow::editItem(const QModelIndex &idx)
{
    Q_UNUSED(idx) // FIXME
}

void WatchWindow::setModel(QAbstractItemModel *model)
{
    QTreeView::setModel(model);

    setRootIsDecorated(true);
    if (header()) {
        setAlwaysResizeColumnsToContents(
            debuggerCore()->boolSetting(AlwaysAdjustLocalsColumnWidths));
        header()->setDefaultAlignment(Qt::AlignLeft);
        if (m_type != LocalsType)
            header()->hide();
    }

    connect(model, SIGNAL(layoutChanged()), SLOT(resetHelper()));
    connect(model, SIGNAL(enableUpdates(bool)), SLOT(setUpdatesEnabled(bool)));
    // Potentially left in disabled state in case engine crashes when expanding.
    setUpdatesEnabled(true);
}

void WatchWindow::setUpdatesEnabled(bool enable)
{
    //qDebug() << "ENABLING UPDATES: " << enable;
    QTreeView::setUpdatesEnabled(enable);
}

void WatchWindow::resetHelper()
{
    bool old = updatesEnabled();
    setUpdatesEnabled(false);
    resetHelper(model()->index(0, 0));
    setUpdatesEnabled(old);
}

void WatchWindow::resetHelper(const QModelIndex &idx)
{
    if (idx.data(LocalsExpandedRole).toBool()) {
        //qDebug() << "EXPANDING " << model()->data(idx, INameRole);
        if (!isExpanded(idx)) {
            expand(idx);
            for (int i = 0, n = model()->rowCount(idx); i != n; ++i) {
                QModelIndex idx1 = model()->index(i, 0, idx);
                resetHelper(idx1);
            }
        }
    } else {
        //qDebug() << "COLLAPSING " << model()->data(idx, INameRole);
        if (isExpanded(idx))
            collapse(idx);
    }
}

void WatchWindow::watchExpression(const QString &exp)
{
    currentEngine()->watchHandler()->watchExpression(exp);
}

void WatchWindow::removeWatchExpression(const QString &exp)
{
    currentEngine()->watchHandler()->removeWatchExpression(exp);
}

void WatchWindow::setModelData
    (int role, const QVariant &value, const QModelIndex &index)
{
    QTC_ASSERT(model(), return);
    model()->setData(index, value, role);
}

void WatchWindow::setWatchpointAtAddress(quint64 address, unsigned size)
{
    BreakpointParameters data(WatchpointAtAddress);
    data.address = address;
    data.size = size;
    BreakpointModelId id = breakHandler()->findWatchpoint(data);
    if (id) {
        qDebug() << "WATCHPOINT EXISTS";
        //   removeBreakpoint(index);
        return;
    }
    breakHandler()->appendBreakpoint(data);
}

void WatchWindow::setWatchpointAtExpression(const QString &exp)
{
    BreakpointParameters data(WatchpointAtExpression);
    data.expression = exp;
    BreakpointModelId id = breakHandler()->findWatchpoint(data);
    if (id) {
        qDebug() << "WATCHPOINT EXISTS";
        //   removeBreakpoint(index);
        return;
    }
    breakHandler()->appendBreakpoint(data);
}

} // namespace Internal
} // namespace Debugger

