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

#include "pasteselectdialog.h"
#include "protocol.h"

#include <utils/qtcassert.h>

#include <QtCore/QDebug>
#include <QtGui/QPushButton>

namespace CodePaster {

PasteSelectDialog::PasteSelectDialog(const QList<Protocol*> &protocols,
                                     QWidget *parent) :
    QDialog(parent),
    m_protocols(protocols)
{
    m_ui.setupUi(this);
    foreach(const Protocol *protocol, protocols) {
        m_ui.protocolBox->addItem(protocol->name());
        connect(protocol, SIGNAL(listDone(QString,QStringList)),
                this, SLOT(listDone(QString,QStringList)));
    }
    connect(m_ui.protocolBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(protocolChanged(int)));

    m_refreshButton = m_ui.buttons->addButton(tr("Refresh"), QDialogButtonBox::ActionRole);
    connect(m_refreshButton, SIGNAL(clicked()), this, SLOT(list()));

    m_ui.listWidget->setSelectionMode(QAbstractItemView::SingleSelection);
#ifndef Q_WS_MACX
    m_ui.listWidget->setFrameStyle(QFrame::NoFrame);
#endif // Q_WS_MACX
    // Proportional formatting of columns for CodePaster
    QFont listFont = m_ui.listWidget->font();
    listFont.setFamily(QLatin1String("Courier"));
    listFont.setStyleHint(QFont::TypeWriter);
    m_ui.listWidget->setFont(listFont);
}

PasteSelectDialog::~PasteSelectDialog()
{
}

QString PasteSelectDialog::pasteId() const
{
    QString id = m_ui.pasteEdit->text();
    const int blankPos = id.indexOf(QLatin1Char(' '));
    if (blankPos != -1)
        id.truncate(blankPos);
    return id;
}

void PasteSelectDialog::setProtocol(const QString &p)
{
    const int index = m_ui.protocolBox->findText(p);
    if (index >= 0) {
        if (index != m_ui.protocolBox->currentIndex()) {
            m_ui.protocolBox->setCurrentIndex(index);
        } else {
            // Trigger a refresh
            protocolChanged(index);
        }
    }
}

QString PasteSelectDialog::protocol() const
{
    return m_ui.protocolBox->currentText();
}

int PasteSelectDialog::protocolIndex() const
{
    return m_ui.protocolBox->currentIndex();
}

void PasteSelectDialog::listDone(const QString &name, const QStringList &items)
{
    // Set if the protocol is still current
    if (name == protocol()) {
        m_ui.listWidget->clear();
        m_ui.listWidget->addItems(items);
    }
}

void PasteSelectDialog::list()
{
    const int index = protocolIndex();

    Protocol *protocol = m_protocols[index];
    QTC_ASSERT((protocol->capabilities() & Protocol::ListCapability), return);

    m_ui.listWidget->clear();
    if (Protocol::ensureConfiguration(protocol, this)) {
        m_ui.listWidget->addItem(new QListWidgetItem(tr("Waiting for items")));
        protocol->list();
    }
}

void PasteSelectDialog::protocolChanged(int i)
{
    const bool canList = m_protocols.at(i)->capabilities() & Protocol::ListCapability;
    m_refreshButton->setEnabled(canList);
    if (canList) {
        list();
    } else {
        m_ui.listWidget->clear();
        m_ui.listWidget->addItem(new QListWidgetItem(tr("This protocol does not support listing")));
    }
}
} // namespace CodePaster
