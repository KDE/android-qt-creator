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

#include "tips.h"
#include "tipcontents.h"
#include "reuse.h"

#include <utils/qtcassert.h>

#include <QtCore/QRect>
#include <QtGui/QColor>
#include <QtGui/QPainter>
#include <QtGui/QPen>
#include <QtGui/QPixmap>
#include <QtGui/QStyle>
#include <QtGui/QFontMetrics>
#include <QtGui/QTextDocument>
#include <QtGui/QStylePainter>
#include <QtGui/QStyleOptionFrame>
#include <QtGui/QResizeEvent>
#include <QtGui/QPaintEvent>
#include <QtGui/QVBoxLayout>

namespace TextEditor {
    namespace Internal {

namespace {
    // @todo: Reuse...
    QPixmap tilePixMap(int size)
    {
        const int checkerbordSize= size;
        QPixmap tilePixmap(checkerbordSize * 2, checkerbordSize * 2);
        tilePixmap.fill(Qt::white);
        QPainter tilePainter(&tilePixmap);
        QColor color(220, 220, 220);
        tilePainter.fillRect(0, 0, checkerbordSize, checkerbordSize, color);
        tilePainter.fillRect(checkerbordSize, checkerbordSize, checkerbordSize, checkerbordSize, color);
        return tilePixmap;
    }
}

QTipLabel::QTipLabel(QWidget *parent) :
    QLabel(parent, Qt::ToolTip | Qt::BypassGraphicsProxyWidget),
    m_tipContent(0)
{}

QTipLabel::~QTipLabel()
{
    if (m_tipContent)
        delete m_tipContent;
}

bool QTipLabel::isInteractive() const
{
    return m_tipContent && m_tipContent->isInteractive();
}

void QTipLabel::setContent(const TipContent &content)
{
    if (m_tipContent)
        delete m_tipContent;
    m_tipContent = content.clone();
}

const TipContent &QTipLabel::content() const
{ return *m_tipContent; }

ColorTip::ColorTip(QWidget *parent) : QTipLabel(parent)
{
    resize(QSize(40, 40));
    m_tilePixMap = tilePixMap(10);
}

ColorTip::~ColorTip()
{}

void ColorTip::configure(const QPoint &pos, QWidget *w)
{
    Q_UNUSED(pos)
    Q_UNUSED(w)

    update();
}

bool ColorTip::canHandleContentReplacement(const TipContent &content) const
{
    if (content.typeId() == ColorContent::COLOR_CONTENT_ID)
        return true;
    return false;
}

void ColorTip::paintEvent(QPaintEvent *event)
{
    QTipLabel::paintEvent(event);

    const QColor &color = static_cast<const ColorContent &>(content()).color();

    QPen pen;
    pen.setWidth(1);
    if (color.value() > 100)
        pen.setColor(color.darker());
    else
        pen.setColor(color.lighter());

    QPainter painter(this);
    painter.setPen(pen);
    painter.setBrush(color);
    QRect r(1, 1, rect().width() - 2, rect().height() - 2);
    painter.drawTiledPixmap(r, m_tilePixMap);
    painter.drawRect(r);
}

TextTip::TextTip(QWidget *parent) : QTipLabel(parent)
{
    setForegroundRole(QPalette::ToolTipText);
    setBackgroundRole(QPalette::ToolTipBase);
    ensurePolished();
    setMargin(1 + style()->pixelMetric(QStyle::PM_ToolTipLabelFrameWidth, 0, this));
    setFrameStyle(QFrame::NoFrame);
    setAlignment(Qt::AlignLeft);
    setIndent(1);
    setWindowOpacity(style()->styleHint(QStyle::SH_ToolTipLabel_Opacity, 0, this) / 255.0);
}

TextTip::~TextTip()
{}

void TextTip::configure(const QPoint &pos, QWidget *w)
{
    const QString &text = static_cast<const TextContent &>(content()).text();
    setText(text);

    // Make it look good with the default ToolTip font on Mac, which has a small descent.
    QFontMetrics fm(font());
    int extraHeight = 0;
    if (fm.descent() == 2 && fm.ascent() >= 11)
        ++extraHeight;

    // Try to find a nice width without unnecessary wrapping.
    setWordWrap(false);
    int tipWidth = sizeHint().width();
    const int screenWidth = screenGeometry(pos, w).width();
    const int maxDesiredWidth = int(screenWidth * .5);
    if (tipWidth > maxDesiredWidth) {
        setWordWrap(true);
        tipWidth = sizeHint().width();
        // If the width is still too large (maybe due to some extremely long word which prevents
        // wrapping), the tip is truncated according to the screen.
        if (tipWidth > screenWidth)
            tipWidth = screenWidth - 10;
    }

    resize(tipWidth, heightForWidth(tipWidth) + extraHeight);
}

bool TextTip::canHandleContentReplacement(const TipContent &content) const
{
    if (content.typeId() == TextContent::TEXT_CONTENT_ID)
        return true;
    return false;
}

void TextTip::paintEvent(QPaintEvent *event)
{
    QStylePainter p(this);
    QStyleOptionFrame opt;
    opt.init(this);
    p.drawPrimitive(QStyle::PE_PanelTipLabel, opt);
    p.end();

    QLabel::paintEvent(event);
}

void TextTip::resizeEvent(QResizeEvent *event)
{
    QStyleHintReturnMask frameMask;
    QStyleOption option;
    option.init(this);
    if (style()->styleHint(QStyle::SH_ToolTip_Mask, &option, this, &frameMask))
        setMask(frameMask.region);

    QLabel::resizeEvent(event);
}

WidgetTip::WidgetTip(QWidget *parent) :
    QTipLabel(parent), m_layout(new QVBoxLayout)
{
    m_layout->setContentsMargins(0, 0, 0, 0);
    setLayout(m_layout);
}

QWidget *WidgetTip::takeWidget(Qt::WindowFlags wf)
{
    // Remove widget from layout
    if (!m_layout->count())
        return 0;
    QLayoutItem *item = m_layout->takeAt(0);
    QWidget *widget = item->widget();
    delete item;
    if (!widget)
        return 0;
    widget->setParent(0, wf);
    return widget;
}

void WidgetTip::configure(const QPoint &pos, QWidget *)
{
    const WidgetContent &anyContent = static_cast<const WidgetContent &>(content());
    QWidget *widget = anyContent.widget();

    QTC_ASSERT(widget && m_layout->count() == 0, return; )

    move(pos);
    m_layout->addWidget(widget);
    m_layout->setSizeConstraint(QLayout::SetFixedSize);
    adjustSize();
}

void WidgetTip::pinToolTipWidget()
{
    QTC_ASSERT(m_layout->count(), return; )

    // Pin the content widget: Rip the widget out of the layout
    // and re-show as a tooltip, with delete on close.
    const QPoint screenPos = mapToGlobal(QPoint(0, 0));
    QWidget *widget = takeWidget(Qt::ToolTip);
    QTC_ASSERT(widget, return; )

    widget->move(screenPos);
    widget->show();
    widget->setAttribute(Qt::WA_DeleteOnClose);
}

bool WidgetTip::canHandleContentReplacement(const TipContent & ) const
{
    // Always create a new widget.
    return false;
}

// need to include it here to force it to be inside the namespaces
#include "moc_tips.cpp"

} // namespace Internal
} // namespace TextEditor
