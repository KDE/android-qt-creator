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

#include "registerwindow.h"
#include "memoryview.h"
#include "debuggeractions.h"
#include "debuggerdialogs.h"
#include "debuggerconstants.h"
#include "debuggercore.h"
#include "debuggerengine.h"
#include "registerhandler.h"
#include "watchdelegatewidgets.h"
#include "memoryagent.h"

#include <utils/qtcassert.h>
#include <utils/savedaction.h>

#include <QtCore/QDebug>

#include <QtGui/QHeaderView>
#include <QtGui/QItemDelegate>
#include <QtGui/QMenu>
#include <QtGui/QPainter>
#include <QtGui/QResizeEvent>


namespace Debugger {
namespace Internal {

static DebuggerEngine *currentEngine()
{
    return debuggerCore()->currentEngine();
}

static RegisterHandler *currentHandler()
{
    DebuggerEngine *engine = currentEngine();
    QTC_ASSERT(engine, return 0);
    return engine->registerHandler();
}

///////////////////////////////////////////////////////////////////////
//
// RegisterDelegate
//
///////////////////////////////////////////////////////////////////////

class RegisterDelegate : public QItemDelegate
{
public:
    RegisterDelegate(QObject *parent)
        : QItemDelegate(parent)
    {}

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &,
        const QModelIndex &index) const
    {
        Register reg = currentHandler()->registerAt(index.row());
        IntegerWatchLineEdit *lineEdit = new IntegerWatchLineEdit(parent);
        const int base = currentHandler()->numberBase();
        const bool big = reg.value.size() > 16;
        // Big integers are assumed to be hexadecimal.
        lineEdit->setBigInt(big);
        lineEdit->setBase(big ? 16 : base);
        lineEdit->setSigned(false);
        lineEdit->setAlignment(Qt::AlignRight);
        lineEdit->setFrame(false);
        return lineEdit;
    }

    void setEditorData(QWidget *editor, const QModelIndex &index) const
    {
        IntegerWatchLineEdit *lineEdit = qobject_cast<IntegerWatchLineEdit *>(editor);
        QTC_ASSERT(lineEdit, return);
        lineEdit->setModelData(index.data(Qt::EditRole));
    }

    void setModelData(QWidget *editor, QAbstractItemModel *,
        const QModelIndex &index) const
    {
        if (index.column() != 1)
            return;
        IntegerWatchLineEdit *lineEdit = qobject_cast<IntegerWatchLineEdit*>(editor);
        QTC_ASSERT(lineEdit, return);
        const int base = currentHandler()->numberBase();
        QString value = lineEdit->text();
        if (base == 16 && !value.startsWith("0x"))
            value = "0x" + value;
        currentEngine()->setRegisterValue(index.row(), value);
    }

    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
        const QModelIndex &) const
    {
        editor->setGeometry(option.rect);
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
        const QModelIndex &index) const
    {
        if (index.column() == 1) {
            bool paintRed = currentHandler()->registerAt(index.row()).changed;
            QPen oldPen = painter->pen();
            if (paintRed)
                painter->setPen(QColor(200, 0, 0));
            // FIXME: performance? this changes only on real font changes.
            QFontMetrics fm(option.font);
            int charWidth = fm.width(QLatin1Char('x'));
            for (int i = '1'; i <= '9'; ++i)
                charWidth = qMax(charWidth, fm.width(QLatin1Char(i)));
            for (int i = 'a'; i <= 'f'; ++i)
                charWidth = qMax(charWidth, fm.width(QLatin1Char(i)));
            QString str = index.data(Qt::DisplayRole).toString();
            int x = option.rect.x();
            for (int i = 0; i < str.size(); ++i) {
                QRect r = option.rect;
                r.setX(x);
                r.setWidth(charWidth);
                x += charWidth;
                painter->drawText(r, Qt::AlignHCenter, QString(str.at(i)));
            }
            if (paintRed)
                painter->setPen(oldPen);
        } else {
            QItemDelegate::paint(painter, option, index);
        }
    }
};


///////////////////////////////////////////////////////////////////////
//
// RegisterWindow
//
///////////////////////////////////////////////////////////////////////

RegisterWindow::RegisterWindow(QWidget *parent)
  : QTreeView(parent)
{
    QAction *act = debuggerCore()->action(UseAlternatingRowColors);
    setFrameStyle(QFrame::NoFrame);
    setWindowTitle(tr("Registers"));
    setAttribute(Qt::WA_MacShowFocusRect, false);
    setAlternatingRowColors(act->isChecked());
    setRootIsDecorated(false);
    setItemDelegate(new RegisterDelegate(this));

    connect(act, SIGNAL(toggled(bool)),
        SLOT(setAlternatingRowColorsHelper(bool)));
    connect(debuggerCore()->action(AlwaysAdjustRegistersColumnWidths),
        SIGNAL(toggled(bool)),
        SLOT(setAlwaysResizeColumnsToContents(bool)));
    setObjectName(QLatin1String("RegisterWindow"));
}

void RegisterWindow::contextMenuEvent(QContextMenuEvent *ev)
{
    QMenu menu;

    DebuggerEngine *engine = currentEngine();
    QTC_ASSERT(engine, return);
    RegisterHandler *handler = currentHandler();
    const unsigned engineCapabilities = engine->debuggerCapabilities();
    const bool actionsEnabled = engine->debuggerActionsEnabled();
    const int state = engine->state();

    QAction *actReload = menu.addAction(tr("Reload Register Listing"));
    actReload->setEnabled((engineCapabilities & RegisterCapability)
        && (state == InferiorStopOk || state == InferiorUnrunnable));

    menu.addSeparator();

    const QModelIndex idx = indexAt(ev->pos());
    if (!idx.isValid())
        return;
    const Register &aRegister = handler->registers().at(idx.row());
    const QVariant addressV = aRegister.editValue();
    const quint64 address = addressV.type() == QVariant::ULongLong
        ? addressV.toULongLong() : 0;
    QAction *actViewMemory = menu.addAction(QString());
    QAction *actEditMemory = menu.addAction(QString());

    QAction *actShowDisassemblerAt = menu.addAction(QString());
    QAction *actShowDisassembler = menu.addAction(tr("Open Disassembler..."));
    actShowDisassembler->setEnabled(engineCapabilities & DisassemblerCapability);

    if (address) {
        const bool canShow = actionsEnabled && (engineCapabilities & ShowMemoryCapability);
        actEditMemory->setText(tr("Open Memory Editor at 0x%1").arg(address, 0, 16));
        actEditMemory->setEnabled(canShow);
        actViewMemory->setText(tr("Open Memory View at Value of Register %1 0x%2")
            .arg(QString::fromAscii(aRegister.name)).arg(address, 0, 16));
        actShowDisassemblerAt->setText(tr("Open Disassembler at 0x%1")
            .arg(address, 0, 16));
        actShowDisassemblerAt->setEnabled(engineCapabilities & DisassemblerCapability);
    } else {
        actEditMemory->setText(tr("Open Memory Editor"));
        actViewMemory->setText(tr("Open Memory View at Value of Register"));
        actEditMemory->setEnabled(false);
        actViewMemory->setEnabled(false);
        actShowDisassemblerAt->setText(tr("Open Disassembler"));
        actShowDisassemblerAt->setEnabled(false);
    }
    menu.addSeparator();

    const int base = handler->numberBase();
    QAction *act16 = menu.addAction(tr("Hexadecimal"));
    act16->setCheckable(true);
    act16->setChecked(base == 16);
    QAction *act10 = menu.addAction(tr("Decimal"));
    act10->setCheckable(true);
    act10->setChecked(base == 10);
    QAction *act8 = menu.addAction(tr("Octal"));
    act8->setCheckable(true);
    act8->setChecked(base == 8);
    QAction *act2 = menu.addAction(tr("Binary"));
    act2->setCheckable(true);
    act2->setChecked(base == 2);
    menu.addSeparator();

    QAction *actAdjust = menu.addAction(tr("Adjust Column Widths to Contents"));
    menu.addAction(debuggerCore()->action(AlwaysAdjustRegistersColumnWidths));
    menu.addSeparator();

    menu.addAction(debuggerCore()->action(SettingsDialog));

    const QPoint position = ev->globalPos();
    QAction *act = menu.exec(position);

    if (act == actAdjust)
        resizeColumnsToContents();
    else if (act == actReload)
        engine->reloadRegisters();
    else if (act == actEditMemory) {
        const QString registerName = QString::fromAscii(aRegister.name, address);
        engine->openMemoryView(address, 0,
            RegisterMemoryView::registerMarkup(address, registerName),
            QPoint(), RegisterMemoryView::title(registerName), 0);
    } else if (act == actViewMemory) {
        engine->openMemoryView(idx.row(),
            DebuggerEngine::MemoryTrackRegister|DebuggerEngine::MemoryView,
            QList<MemoryMarkup>(), position, QString(), this);
    } else if (act == actShowDisassembler) {
        AddressDialog dialog;
        if (address)
            dialog.setAddress(address);
        if (dialog.exec() == QDialog::Accepted)
            currentEngine()->openDisassemblerView(Location(dialog.address()));
    } else if (act == actShowDisassemblerAt) {
        engine->openDisassemblerView(Location(address));
    } else if (act == act16)
        handler->setNumberBase(16);
    else if (act == act10)
        handler->setNumberBase(10);
    else if (act == act8)
        handler->setNumberBase(8);
    else if (act == act2)
        handler->setNumberBase(2);
}

void RegisterWindow::resizeColumnsToContents()
{
    resizeColumnToContents(0);
    resizeColumnToContents(1);
}

void RegisterWindow::setAlwaysResizeColumnsToContents(bool on)
{
    QHeaderView::ResizeMode mode = on
        ? QHeaderView::ResizeToContents : QHeaderView::Interactive;
    header()->setResizeMode(0, mode);
    header()->setResizeMode(1, mode);
}

void RegisterWindow::setModel(QAbstractItemModel *model)
{
    QTreeView::setModel(model);
    if (header()) {
        bool adjust = debuggerCore()->boolSetting(AlwaysAdjustRegistersColumnWidths);
        setAlwaysResizeColumnsToContents(adjust);
    }
}

void RegisterWindow::reloadRegisters()
{
    // FIXME: Only trigger when becoming visible?
    currentEngine()->reloadRegisters();
}

} // namespace Internal
} // namespace Debugger
