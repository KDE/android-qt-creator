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

#include "ifindsupport.h"

#include <QtCore/QTimer>
#include <QtCore/QPropertyAnimation>
#include <QtGui/QWidget>
#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>

namespace Find {
namespace Internal {

class WrapIndicator : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity USER true)

public:
    WrapIndicator(QWidget *parent = 0)
        : QWidget(parent),
          m_opacity(1.0)
    {
        if (parent)
            setGeometry(QRect(parent->rect().center() - QPoint(25, 25),
                              parent->rect().center() + QPoint(25, 25)));
    }

    qreal opacity() const { return m_opacity; }
    void setOpacity(qreal value) { m_opacity = value; update(); }

    void run()
    {
        show();
        QTimer::singleShot(300, this, SLOT(runInternal()));
    }

protected:
    void paintEvent(QPaintEvent *)
    {
        static QPixmap foreground(QLatin1String(":/find/images/wrapindicator.png"));
        QPainter p(this);
        p.setOpacity(m_opacity);
        p.drawPixmap(rect(), foreground);
    }

private slots:
    void runInternal()
    {
        QPropertyAnimation *anim = new QPropertyAnimation(this, "opacity", this);
        anim->setDuration(200);
        anim->setEndValue(0.);
        connect(anim, SIGNAL(finished()), this, SLOT(deleteLater()));
        anim->start(QAbstractAnimation::DeleteWhenStopped);
    }

private:
    qreal m_opacity;
};

} // Internal
} // Find

using namespace Find;

void IFindSupport::showWrapIndicator(QWidget *parent)
{
    (new Internal::WrapIndicator(parent))->run();
}

#include "ifindsupport.moc"
