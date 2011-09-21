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

#include "pluginerroroverview.h"
#include "ui_pluginerroroverview.h"
#include "pluginspec.h"
#include "pluginmanager.h"

Q_DECLARE_METATYPE(ExtensionSystem::PluginSpec*)

namespace ExtensionSystem {
namespace Internal {

class PluginErrorOverviewPrivate : public QObject
{
    Q_OBJECT
public:
    PluginErrorOverviewPrivate(PluginManager *manager, QDialog *dialog);
    ~PluginErrorOverviewPrivate();

private slots:
    void showDetails(QListWidgetItem *item);

private:
    Ui::PluginErrorOverview *m_ui;
    PluginManager *m_manager;
};

} // Internal
} // ExtensionSystem

using namespace ExtensionSystem;
using namespace ExtensionSystem::Internal;

PluginErrorOverview::PluginErrorOverview(PluginManager *manager, QWidget *parent) :
    QDialog(parent),
    d(new PluginErrorOverviewPrivate(manager, this))
{
}

PluginErrorOverview::~PluginErrorOverview()
{
    delete d;
}

PluginErrorOverviewPrivate::PluginErrorOverviewPrivate(PluginManager *manager, QDialog *dialog)
    : m_ui(new Ui::PluginErrorOverview),
      m_manager(manager)
{
    m_ui->setupUi(dialog);
    m_ui->buttonBox->addButton(tr("Continue"), QDialogButtonBox::AcceptRole);

    foreach (PluginSpec *spec, m_manager->plugins()) {
        // only show errors on startup if plugin is enabled.
        if (spec->hasError() && spec->isEnabled() && !spec->isDisabledIndirectly()) {
            QListWidgetItem *item = new QListWidgetItem(spec->name());
            item->setData(Qt::UserRole, qVariantFromValue(spec));
            m_ui->pluginList->addItem(item);
        }
    }

    connect(m_ui->pluginList, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
            this, SLOT(showDetails(QListWidgetItem*)));

    if (m_ui->pluginList->count() > 0)
        m_ui->pluginList->setCurrentRow(0);
}

PluginErrorOverviewPrivate::~PluginErrorOverviewPrivate()
{
    delete m_ui;
}

void PluginErrorOverviewPrivate::showDetails(QListWidgetItem *item)
{
    if (item) {
        PluginSpec *spec = item->data(Qt::UserRole).value<PluginSpec *>();
        m_ui->pluginError->setText(spec->errorString());
    } else {
        m_ui->pluginError->setText(QString());
    }
}

#include "pluginerroroverview.moc"
