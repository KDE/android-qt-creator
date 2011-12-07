/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nicolas Arnaud-Cormos.
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

#include "macrooptionswidget.h"
#include "ui_macrooptionswidget.h"
#include "macrosconstants.h"
#include "macromanager.h"
#include "macro.h"
#include "macrosconstants.h"

#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/id.h>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>

#include <QtGui/QShortcut>
#include <QtGui/QButtonGroup>
#include <QtGui/QTreeWidget>
#include <QtGui/QTreeWidgetItem>
#include <QtGui/QCheckBox>
#include <QtGui/QGroupBox>
#include <QtGui/QHeaderView>
#include <QtGui/QRegExpValidator>
#include <QtGui/QLineEdit>

namespace {
    int NAME_ROLE = Qt::UserRole;
    int WRITE_ROLE = Qt::UserRole+1;
}

using namespace Macros;
using namespace Macros::Internal;


MacroOptionsWidget::MacroOptionsWidget(QWidget *parent) :
    QWidget(parent),
    m_ui(new Ui::MacroOptionsWidget),
    m_changingCurrent(false)
{
    m_ui->setupUi(this);

    connect(m_ui->treeWidget, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
            this, SLOT(changeCurrentItem(QTreeWidgetItem*)));
    connect(m_ui->removeButton, SIGNAL(clicked()),
            this, SLOT(remove()));
    connect(m_ui->description, SIGNAL(textChanged(QString)),
            this, SLOT(changeDescription(QString)));

    m_ui->treeWidget->header()->setSortIndicator(0, Qt::AscendingOrder);

    initialize();
}

MacroOptionsWidget::~MacroOptionsWidget()
{
    delete m_ui;
}

void MacroOptionsWidget::initialize()
{
    m_macroToRemove.clear();
    m_macroToChange.clear();
    m_ui->treeWidget->clear();

    // Create the treeview
    createTable();
}

void MacroOptionsWidget::createTable()
{
    QDir dir(MacroManager::instance()->macrosDirectory());
    Core::ICore *core = Core::ICore::instance();
    Core::ActionManager *am = core->actionManager();

    QMapIterator<QString, Macro *> it(MacroManager::instance()->macros());
    while (it.hasNext()) {
        it.next();
        QFileInfo fileInfo(it.value()->fileName());
        if (fileInfo.absoluteDir() == dir.absolutePath()) {
            QTreeWidgetItem *macroItem = new QTreeWidgetItem(m_ui->treeWidget);
            macroItem->setText(0, it.value()->displayName());
            macroItem->setText(1, it.value()->description());
            macroItem->setData(0, NAME_ROLE, it.value()->displayName());
            macroItem->setData(0, WRITE_ROLE, it.value()->isWritable());

            Core::Command *command = am->command(Core::Id(Constants::PREFIX_MACRO+it.value()->displayName()));
            if (command && command->shortcut())
                macroItem->setText(2, command->shortcut()->key().toString());
        }
    }
}

void MacroOptionsWidget::changeCurrentItem(QTreeWidgetItem *current)
{
    m_changingCurrent = true;
    if (!current) {
        m_ui->removeButton->setEnabled(false);
        m_ui->description->clear();
        m_ui->macroGroup->setEnabled(false);
    } else {
        m_ui->removeButton->setEnabled(true);
        m_ui->description->setText(current->text(1));
        m_ui->description->setEnabled(current->data(0, WRITE_ROLE).toBool());
        m_ui->macroGroup->setEnabled(true);
    }
    m_changingCurrent = false;
}

void MacroOptionsWidget::remove()
{
    QTreeWidgetItem *current = m_ui->treeWidget->currentItem();
    m_macroToRemove.append(current->data(0, NAME_ROLE).toString());
    delete current;
}

void MacroOptionsWidget::apply()
{
    // Remove macro
    foreach (const QString &name, m_macroToRemove) {
        MacroManager::instance()->deleteMacro(name);
        m_macroToChange.remove(name);
    }

    // Change macro
    QMapIterator<QString, QString> it(m_macroToChange);
    while (it.hasNext()) {
        it.next();
        MacroManager::instance()->changeMacro(it.key(), it.value());
    }

    // Reinitialize the page
    initialize();
}

void MacroOptionsWidget::changeDescription(const QString &description)
{
    QTreeWidgetItem *current = m_ui->treeWidget->currentItem();
    if (m_changingCurrent || !current)
        return;

    QString macroName = current->data(0, NAME_ROLE).toString();
    m_macroToChange[macroName] = description;
    current->setText(1, description);
    QFont font = current->font(1);
    font.setItalic(true);
    current->setFont(1, font);
}
