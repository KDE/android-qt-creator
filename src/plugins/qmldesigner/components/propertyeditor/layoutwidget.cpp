/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#include "layoutwidget.h"
#include <QGridLayout>
#include <QFile>
#include <QDebug>

QT_BEGIN_NAMESPACE


LayoutWidget::LayoutWidget(QWidget *parent) : QFrame(parent)
{
    QSizePolicy policy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QGridLayout *grid = new QGridLayout(this);
    setLayout(grid);
    grid->setContentsMargins(0,0,0,0);
    grid->setSpacing(4);

    m_topButton = new QPushButton(this);
    m_topButton->setCheckable(true);
    m_topButton->setSizePolicy(policy);

    grid->addWidget(m_topButton, 0, 2, 2, 1, Qt::AlignHCenter);

    m_bottomButton = new QPushButton(this);
    m_bottomButton->setCheckable(true);

    m_bottomButton->setSizePolicy(policy);

    grid->addWidget(m_bottomButton, 3, 2, 2, 1, Qt::AlignHCenter);

    m_leftButton = new QPushButton(this);
    m_leftButton->setCheckable(true);
    m_leftButton->setSizePolicy(policy);


    grid->addWidget(m_leftButton, 2, 0, 1, 2, Qt::AlignVCenter);

    m_rightButton = new QPushButton(this);
    m_rightButton->setCheckable(true);
    m_rightButton->setSizePolicy(policy);

    grid->addWidget(m_rightButton, 2, 3, 1, 2, Qt::AlignVCenter);


    m_middleButton = new QPushButton(this);

   grid->addWidget(m_middleButton, 2, 2, 1, 1, Qt::AlignCenter);

   connect(m_topButton, SIGNAL(toggled(bool)), this, SLOT(setTopAnchor(bool)));
   connect(m_bottomButton, SIGNAL(toggled(bool)), this, SLOT(setBottomAnchor(bool)));
   connect(m_leftButton, SIGNAL(toggled(bool)), this, SLOT(setLeftAnchor(bool)));
   connect(m_rightButton, SIGNAL(toggled(bool)), this, SLOT(setRightAnchor(bool)));

   connect(m_middleButton, SIGNAL(pressed()), this, SIGNAL(fill()));
}

LayoutWidget::~LayoutWidget()
{
}

 void LayoutWidget::setIcon(QPushButton *button, QUrl url)
 {
        if (url.scheme() == QLatin1String("file")) {
            QFile file(url.toLocalFile());
            file.open(QIODevice::ReadOnly);
            if (file.isOpen()) {
                QPixmap pixmap(url.toLocalFile());
                button->setIcon(pixmap);
            } else {
                qWarning() << QLatin1String("setIconFromFile: ") << url << QLatin1String(" not found!");
            }
        }
 }

QT_END_NAMESPACE


