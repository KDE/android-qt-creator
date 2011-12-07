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

#include "outputpanemanager.h"
#include "outputpane.h"
#include "coreconstants.h"
#include "findplaceholder.h"

#include "coreconstants.h"
#include "icore.h"
#include "ioutputpane.h"
#include "mainwindow.h"
#include "modemanager.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/findplaceholder.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/id.h>

#include <extensionsystem/pluginmanager.h>

#include <utils/styledbar.h>
#include <utils/qtcassert.h>

#include <QtCore/QDebug>

#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QComboBox>
#include <QtGui/QFocusEvent>
#include <QtGui/QHBoxLayout>
#include <QtGui/QSplitter>
#include <QtGui/QPainter>
#include <QtGui/QToolButton>
#include <QtGui/QStackedWidget>
#include <QtGui/QMenu>

namespace Core {
namespace Internal {

////
// OutputPaneManager
////

static OutputPaneManager *m_instance = 0;

void OutputPaneManager::create()
{
   m_instance = new OutputPaneManager;
}

void OutputPaneManager::destroy()
{
    delete m_instance;
    m_instance = 0;
}

OutputPaneManager *OutputPaneManager::instance()
{
    return m_instance;
}

void OutputPaneManager::updateStatusButtons(bool visible)
{
    int idx = m_widgetComboBox->itemData(m_widgetComboBox->currentIndex()).toInt();
    if (m_buttons.value(idx))
        m_buttons.value(idx)->setChecked(visible);
    if (m_pageMap.value(idx))
        m_pageMap.value(idx)->visibilityChanged(visible);
    m_minMaxAction->setVisible(OutputPanePlaceHolder::getCurrent()
                               && OutputPanePlaceHolder::getCurrent()->canMaximizeOrMinimize());
}

OutputPaneManager::OutputPaneManager(QWidget *parent) :
    QWidget(parent),
    m_widgetComboBox(new QComboBox),
    m_closeButton(new QToolButton),
    m_minMaxAction(0),
    m_minMaxButton(new QToolButton),
    m_nextAction(0),
    m_prevAction(0),
    m_lastIndex(-1),
    m_outputWidgetPane(new QStackedWidget),
    m_opToolBarWidgets(new QStackedWidget),
    m_minimizeIcon(":/core/images/arrowdown.png"),
    m_maximizeIcon(":/core/images/arrowup.png"),
    m_maximised(false)
{
    setWindowTitle(tr("Output"));
    connect(m_widgetComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(changePage()));

    m_clearAction = new QAction(this);
    m_clearAction->setIcon(QIcon(QLatin1String(Constants::ICON_CLEAN_PANE)));
    m_clearAction->setText(tr("Clear"));
    connect(m_clearAction, SIGNAL(triggered()), this, SLOT(clearPage()));

    m_nextAction = new QAction(this);
    m_nextAction->setIcon(QIcon(QLatin1String(Constants::ICON_NEXT)));
    m_nextAction->setText(tr("Next Item"));
    connect(m_nextAction, SIGNAL(triggered()), this, SLOT(slotNext()));

    m_prevAction = new QAction(this);
    m_prevAction->setIcon(QIcon(QLatin1String(Constants::ICON_PREV)));
    m_prevAction->setText(tr("Previous Item"));
    connect(m_prevAction, SIGNAL(triggered()), this, SLOT(slotPrev()));

    m_minMaxAction = new QAction(this);
    m_minMaxAction->setIcon(m_maximizeIcon);
    m_minMaxAction->setText(tr("Maximize Output Pane"));

    m_closeButton->setIcon(QIcon(QLatin1String(Constants::ICON_CLOSE)));
    connect(m_closeButton, SIGNAL(clicked()), this, SLOT(slotHide()));

    QVBoxLayout *mainlayout = new QVBoxLayout;
    mainlayout->setSpacing(0);
    mainlayout->setMargin(0);
    m_toolBar = new Utils::StyledBar;
    QHBoxLayout *toolLayout = new QHBoxLayout(m_toolBar);
    toolLayout->setMargin(0);
    toolLayout->setSpacing(0);
    toolLayout->addWidget(m_widgetComboBox);
    m_clearButton = new QToolButton;
    toolLayout->addWidget(m_clearButton);
    m_prevToolButton = new QToolButton;
    toolLayout->addWidget(m_prevToolButton);
    m_nextToolButton = new QToolButton;
    toolLayout->addWidget(m_nextToolButton);
    toolLayout->addWidget(m_opToolBarWidgets);
    toolLayout->addWidget(m_minMaxButton);
    toolLayout->addWidget(m_closeButton);
    mainlayout->addWidget(m_toolBar);
    mainlayout->addWidget(m_outputWidgetPane, 10);
    mainlayout->addWidget(new Core::FindToolBarPlaceHolder(this));
    setLayout(mainlayout);

    m_buttonsWidget = new QWidget;
    m_buttonsWidget->setLayout(new QHBoxLayout);
    m_buttonsWidget->layout()->setContentsMargins(5,0,0,0);
    m_buttonsWidget->layout()->setSpacing(4);

}

OutputPaneManager::~OutputPaneManager()
{
}

QWidget *OutputPaneManager::buttonsWidget()
{
    return m_buttonsWidget;
}

// Return shortcut as Ctrl+<number>
static inline int paneShortCut(int number)
{
#ifdef Q_WS_MAC
    int modifier = Qt::CTRL;
#else
    int modifier = Qt::ALT;
#endif
    return modifier | (Qt::Key_0 + number);
}

void OutputPaneManager::init()
{
    ActionManager *am = Core::ICore::instance()->actionManager();
    ActionContainer *mwindow = am->actionContainer(Constants::M_WINDOW);
    const Context globalcontext(Core::Constants::C_GLOBAL);

    // Window->Output Panes
    ActionContainer *mpanes = am->createMenu(Constants::M_WINDOW_PANES);
    mwindow->addMenu(mpanes, Constants::G_WINDOW_PANES);
    mpanes->menu()->setTitle(tr("Output &Panes"));
    mpanes->appendGroup("Coreplugin.OutputPane.ActionsGroup");
    mpanes->appendGroup("Coreplugin.OutputPane.PanesGroup");

    Core::Command *cmd;

    cmd = am->registerAction(m_clearAction, "Coreplugin.OutputPane.clear", globalcontext);
    m_clearButton->setDefaultAction(cmd->action());
    mpanes->addAction(cmd, "Coreplugin.OutputPane.ActionsGroup");

    cmd = am->registerAction(m_prevAction, "Coreplugin.OutputPane.previtem", globalcontext);
    cmd->setDefaultKeySequence(QKeySequence("Shift+F6"));
    m_prevToolButton->setDefaultAction(cmd->action());
    mpanes->addAction(cmd, "Coreplugin.OutputPane.ActionsGroup");

    cmd = am->registerAction(m_nextAction, "Coreplugin.OutputPane.nextitem", globalcontext);
    m_nextToolButton->setDefaultAction(cmd->action());
    cmd->setDefaultKeySequence(QKeySequence("F6"));
    mpanes->addAction(cmd, "Coreplugin.OutputPane.ActionsGroup");

    cmd = am->registerAction(m_minMaxAction, "Coreplugin.OutputPane.minmax", globalcontext);
#ifdef Q_WS_MAC
    cmd->setDefaultKeySequence(QKeySequence("Ctrl+9"));
#else
    cmd->setDefaultKeySequence(QKeySequence("Alt+9"));
#endif
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setAttribute(Command::CA_UpdateIcon);
    mpanes->addAction(cmd, "Coreplugin.OutputPane.ActionsGroup");
    connect(m_minMaxAction, SIGNAL(triggered()), this, SLOT(slotMinMax()));
    m_minMaxButton->setDefaultAction(cmd->action());

    QAction *sep = new QAction(this);
    sep->setSeparator(true);
    cmd = am->registerAction(sep, "Coreplugin.OutputPane.Sep", globalcontext);
    mpanes->addAction(cmd, "Coreplugin.OutputPane.ActionsGroup");

    QList<IOutputPane*> panes = ExtensionSystem::PluginManager::instance()
        ->getObjects<IOutputPane>();
    QMultiMap<int, IOutputPane*> sorted;
    foreach (IOutputPane* outPane, panes)
        sorted.insertMulti(outPane->priorityInStatusBar(), outPane);

    QMultiMap<int, IOutputPane*>::const_iterator it, begin;
    begin = sorted.constBegin();
    it = sorted.constEnd();
    int shortcutNumber = 1;
    while (it != begin) {
        --it;
        IOutputPane* outPane = it.value();
        const int idx = m_outputWidgetPane->addWidget(outPane->outputWidget(this));

        m_pageMap.insert(idx, outPane);
        connect(outPane, SIGNAL(showPage(bool,bool)), this, SLOT(showPage(bool,bool)));
        connect(outPane, SIGNAL(hidePage()), this, SLOT(slotHide()));
        connect(outPane, SIGNAL(togglePage(bool)), this, SLOT(togglePage(bool)));
        connect(outPane, SIGNAL(navigateStateUpdate()), this, SLOT(updateNavigateState()));

        QWidget *toolButtonsContainer = new QWidget(m_opToolBarWidgets);
        QHBoxLayout *toolButtonsLayout = new QHBoxLayout;
        toolButtonsLayout->setMargin(0);
        toolButtonsLayout->setSpacing(0);
        foreach (QWidget *toolButton, outPane->toolBarWidgets())
            toolButtonsLayout->addWidget(toolButton);
        toolButtonsLayout->addStretch(5);
        toolButtonsContainer->setLayout(toolButtonsLayout);

        m_opToolBarWidgets->addWidget(toolButtonsContainer);

        QString actionId = QString("QtCreator.Pane.%1").arg(outPane->displayName().simplified());
        actionId.remove(QLatin1Char(' '));
        QAction *action = new QAction(outPane->displayName(), this);

        Command *cmd = am->registerAction(action, Id(actionId), Context(Constants::C_GLOBAL));

        mpanes->addAction(cmd, "Coreplugin.OutputPane.PanesGroup");
        m_actions.insert(cmd->action(), idx);

        if (outPane->priorityInStatusBar() != -1) {
            cmd->setDefaultKeySequence(QKeySequence(paneShortCut(shortcutNumber)));
            QToolButton *button = new OutputPaneToggleButton(shortcutNumber, outPane->displayName(),
                                                             cmd->action());
            ++shortcutNumber;
            m_buttonsWidget->layout()->addWidget(button);
            connect(button, SIGNAL(clicked()), this, SLOT(buttonTriggered()));
            m_buttons.insert(idx, button);
        }

        // Now add the entry to the combobox, since the first item we add sets the currentIndex, thus we need to be set up for that
        m_widgetComboBox->addItem(outPane->displayName(), idx);

        connect(cmd->action(), SIGNAL(triggered()), this, SLOT(shortcutTriggered()));
    }

    changePage();
}

void OutputPaneManager::shortcutTriggered()
{
    QAction *action = qobject_cast<QAction*>(sender());
    if (action && m_actions.contains(action)) {
        int idx = m_actions.value(action);
        Core::IOutputPane *outputPane = m_pageMap.value(idx);
        // Now check the special case, the output window is already visible,
        // we are already on that page
        // but the outputpane doesn't have focus
        // then just give it focus
        // else do the same as clicking on the button does
        if (OutputPanePlaceHolder::isCurrentVisible()
           && m_widgetComboBox->itemData(m_widgetComboBox->currentIndex()).toInt() == idx) {
            if (!outputPane->hasFocus() && outputPane->canFocus())
                outputPane->setFocus();
            else
                slotHide();
        } else {
            outputPane->popup(true);
        }
    }
}

bool OutputPaneManager::isMaximized()const
{
    return m_maximised;
}

void OutputPaneManager::slotMinMax()
{
    QTC_ASSERT(OutputPanePlaceHolder::getCurrent(), return);

    if (!OutputPanePlaceHolder::getCurrent()->isVisible()) // easier than disabling/enabling the action
        return;
    m_maximised = !m_maximised;
    OutputPanePlaceHolder::getCurrent()->maximizeOrMinimize(m_maximised);
    m_minMaxAction->setIcon(m_maximised ? m_minimizeIcon : m_maximizeIcon);
    m_minMaxAction->setText(m_maximised ? tr("Minimize Output Pane")
                                            : tr("Maximize Output Pane"));
}

void OutputPaneManager::buttonTriggered()
{
    QToolButton *button = qobject_cast<QToolButton *>(sender());
    QMap<int, QToolButton *>::const_iterator it, end;
    end = m_buttons.constEnd();
    for (it = m_buttons.begin(); it != end; ++it) {
        if (it.value() == button)
            break;
    }
    int idx = it.key();

    if (m_widgetComboBox->itemData(m_widgetComboBox->currentIndex()).toInt() == idx &&
        OutputPanePlaceHolder::isCurrentVisible()) {
        // we should toggle and the page is already visible and we are actually closeable
        slotHide();
    } else {
        showPage(idx, true);
    }
}

void OutputPaneManager::slotNext()
{
    int idx = m_widgetComboBox->itemData(m_widgetComboBox->currentIndex()).toInt();
    ensurePageVisible(idx);
    IOutputPane *out = m_pageMap.value(idx);
    if (out->canNext())
        out->goToNext();
}

void OutputPaneManager::slotPrev()
{
    int idx = m_widgetComboBox->itemData(m_widgetComboBox->currentIndex()).toInt();
    ensurePageVisible(idx);
    IOutputPane *out = m_pageMap.value(idx);
    if (out->canPrevious())
        out->goToPrev();
}

void OutputPaneManager::slotHide()
{
    if (OutputPanePlaceHolder::getCurrent()) {
        OutputPanePlaceHolder::getCurrent()->setVisible(false);
        int idx = m_widgetComboBox->itemData(m_widgetComboBox->currentIndex()).toInt();
        if (m_buttons.value(idx))
            m_buttons.value(idx)->setChecked(false);
        if (m_pageMap.value(idx))
            m_pageMap.value(idx)->visibilityChanged(false);
        if (IEditor *editor = Core::EditorManager::instance()->currentEditor()) {
            QWidget *w = editor->widget()->focusWidget();
            if (!w)
                w = editor->widget();
            w->setFocus();
        }
    }
}

int OutputPaneManager::findIndexForPage(IOutputPane *out)
{
    if (!out)
        return -1;

    int stackIndex = -1;
    QMap<int, IOutputPane*>::const_iterator it = m_pageMap.constBegin();
    while (it != m_pageMap.constEnd()) {
        if (it.value() == out) {
            stackIndex = it.key();
            break;
        }
        ++it;
    }
    if (stackIndex > -1)
        return m_widgetComboBox->findData(stackIndex);
    else
        return -1;
}

void OutputPaneManager::ensurePageVisible(int idx)
{
    if (m_widgetComboBox->itemData(m_widgetComboBox->currentIndex()).toInt() != idx) {
        m_widgetComboBox->setCurrentIndex(m_widgetComboBox->findData(idx));
    } else {
        changePage();
    }
}

void OutputPaneManager::updateNavigateState()
{
    IOutputPane* pane = qobject_cast<IOutputPane*>(sender());
    int idx = findIndexForPage(pane);
    if (m_widgetComboBox->itemData(m_widgetComboBox->currentIndex()).toInt() == idx) {
        m_prevAction->setEnabled(pane->canNavigate() && pane->canPrevious());
        m_nextAction->setEnabled(pane->canNavigate() && pane->canNext());
    }
}

// Slot connected to showPage signal of each page
void OutputPaneManager::showPage(bool focus, bool ensureSizeHint)
{
    int idx = findIndexForPage(qobject_cast<IOutputPane*>(sender()));
    showPage(idx, focus);
    if (ensureSizeHint && OutputPanePlaceHolder::getCurrent())
        OutputPanePlaceHolder::getCurrent()->ensureSizeHintAsMinimum();
}

void OutputPaneManager::showPage(int idx, bool focus)
{
    IOutputPane *out = m_pageMap.value(idx);
    if (idx > -1) {
        if (!OutputPanePlaceHolder::getCurrent()) {
            // In this mode we don't have a placeholder
            // switch to the output mode and switch the page
            ModeManager::instance()->activateMode(Constants::MODE_EDIT);
        }
        if (OutputPanePlaceHolder::getCurrent()) {
            // make the page visible
            OutputPanePlaceHolder::getCurrent()->setVisible(true);
            ensurePageVisible(idx);
            if (focus && out->canFocus())
                out->setFocus();
        }
    }
}

void OutputPaneManager::togglePage(bool focus)
{
    int idx = findIndexForPage(qobject_cast<IOutputPane*>(sender()));
    if (OutputPanePlaceHolder::isCurrentVisible()
       && m_widgetComboBox->itemData(m_widgetComboBox->currentIndex()).toInt() == idx) {
         slotHide();
    } else {
         showPage(idx, focus);
    }
}

void OutputPaneManager::focusInEvent(QFocusEvent *e)
{
    if (m_outputWidgetPane->currentWidget())
        m_outputWidgetPane->currentWidget()->setFocus(e->reason());
}

void OutputPaneManager::changePage()
{
    if (m_outputWidgetPane->count() <= 0)
        return;

    if (!m_pageMap.contains(m_lastIndex)) {
        int idx = m_outputWidgetPane->currentIndex();
        m_pageMap.value(idx)->visibilityChanged(true);
        if (m_buttons.value(idx)) {
            m_buttons.value(idx)->setChecked(OutputPanePlaceHolder::isCurrentVisible());
        }
        m_lastIndex = idx;
        return;
    }

    int idx = m_widgetComboBox->itemData(m_widgetComboBox->currentIndex()).toInt();
    if (m_lastIndex != idx) {
        m_outputWidgetPane->setCurrentIndex(idx);
        m_opToolBarWidgets->setCurrentIndex(idx);
        m_pageMap.value(idx)->visibilityChanged(true);
        m_pageMap.value(m_lastIndex)->visibilityChanged(false);

        bool canNavigate = m_pageMap.value(idx)->canNavigate();
        m_prevAction->setEnabled(canNavigate && m_pageMap.value(idx)->canPrevious());
        m_nextAction->setEnabled(canNavigate && m_pageMap.value(idx)->canNext());
    }

    if (m_buttons.value(m_lastIndex))
        m_buttons.value(m_lastIndex)->setChecked(false);

    if (m_buttons.value(idx))
        m_buttons.value(idx)->setChecked(OutputPanePlaceHolder::isCurrentVisible());

    m_lastIndex = idx;
}

void OutputPaneManager::clearPage()
{
    if (m_pageMap.contains(m_outputWidgetPane->currentIndex()))
        m_pageMap.value(m_outputWidgetPane->currentIndex())->clearContents();
}


OutputPaneToggleButton::OutputPaneToggleButton(int number, const QString &text,
                                               QAction *action, QWidget *parent)
    : QToolButton(parent)
    , m_number(QString::number(number))
    , m_text(text)
    , m_action(action)
{
    setFocusPolicy(Qt::NoFocus);
    setCheckable(true);
    QFont fnt = QApplication::font();
    setFont(fnt);
    setStyleSheet(
            "QToolButton { border-image: url(:/core/images/panel_button.png) 2 2 2 19;"
                         " border-width: 2px 2px 2px 19px; padding-left: -17; padding-right: 4 } "
            "QToolButton:checked { border-image: url(:/core/images/panel_button_checked.png) 2 2 2 19 } "
            "QToolButton::menu-indicator { width:0; height:0 }"
#ifndef Q_WS_MAC // Mac UIs usually don't hover
            "QToolButton:checked:hover { border-image: url(:/core/images/panel_button_checked_hover.png) 2 2 2 19 } "
            "QToolButton:pressed:hover { border-image: url(:/core/images/panel_button_pressed.png) 2 2 2 19 } "
            "QToolButton:hover { border-image: url(:/core/images/panel_button_hover.png) 2 2 2 19 } "
#endif
            );
    if (m_action)
        connect(m_action, SIGNAL(changed()), this, SLOT(updateToolTip()));
}

void OutputPaneToggleButton::updateToolTip()
{
    Q_ASSERT(m_action);
    setToolTip(m_action->toolTip());
}

QSize OutputPaneToggleButton::sizeHint() const
{
    ensurePolished();

    QSize s = fontMetrics().size(Qt::TextSingleLine, m_text);

    // Expand to account for border image set by stylesheet above
    s.rwidth() += 19 + 5 + 2;
    s.rheight() += 2 + 2;

    return s.expandedTo(QApplication::globalStrut());
}

void OutputPaneToggleButton::paintEvent(QPaintEvent *event)
{
    // For drawing the style sheet stuff
    QToolButton::paintEvent(event);

    const QFontMetrics fm = fontMetrics();
    const int baseLine = (height() - fm.height() + 1) / 2 + fm.ascent();
    const int numberWidth = fm.width(m_number);

    QPainter p(this);
    p.setFont(font());
    p.setPen(Qt::white);
    p.drawText((20 - numberWidth) / 2, baseLine, m_number);
    if (!isChecked())
        p.setPen(Qt::black);
    int leftPart = 22;
    p.drawText(leftPart, baseLine, fm.elidedText(m_text, Qt::ElideRight, width() - leftPart - 1));
}

} // namespace Internal
} // namespace Core


