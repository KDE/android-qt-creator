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

#ifndef TIPCONTENTS_H
#define TIPCONTENTS_H

#include "texteditor/texteditor_global.h"

#include <QtCore/QString>
#include <QtGui/QColor>

namespace TextEditor {

class TEXTEDITOR_EXPORT TipContent
{
protected:
    TipContent();

public:
    virtual ~TipContent();

    virtual TipContent *clone() const = 0;
    virtual int typeId() const = 0;
    virtual bool isValid() const = 0;
    virtual bool isInteractive() const = 0;
    virtual int showTime() const = 0;
    virtual bool equals(const TipContent &tipContent) const = 0;
};

class TEXTEDITOR_EXPORT ColorContent : public TipContent
{
public:
    ColorContent(const QColor &color);
    virtual ~ColorContent();

    virtual TipContent *clone() const;
    virtual int typeId() const;
    virtual bool isValid() const;
    virtual bool isInteractive() const;
    virtual int showTime() const;
    virtual bool equals(const TipContent &tipContent) const;

    const QColor &color() const;

    static const int COLOR_CONTENT_ID = 0;

private:
    QColor m_color;
};

class TEXTEDITOR_EXPORT TextContent : public TipContent
{
public:
    TextContent(const QString &text);
    virtual ~TextContent();

    virtual TipContent *clone() const;
    virtual int typeId() const;
    virtual bool isValid() const;
    virtual bool isInteractive() const;
    virtual int showTime() const;
    virtual bool equals(const TipContent &tipContent) const;

    const QString &text() const;

    static const int TEXT_CONTENT_ID = 1;

private:
    QString m_text;
};

// A content for displaying any widget (with a layout).
class TEXTEDITOR_EXPORT WidgetContent : public TipContent
{
public:
    explicit WidgetContent(QWidget *w, bool interactive = false);

    virtual TipContent *clone() const;
    virtual int typeId() const;
    virtual bool isValid() const;
    virtual int showTime() const;
    virtual bool isInteractive() const;
    void setInteractive(bool i);

    virtual bool equals(const TipContent &tipContent) const;

    // Helper to 'pin' (show as real window) a tooltip shown
    // using WidgetContent
    static bool pinToolTip(QWidget *w);

    static const int WIDGET_CONTENT_ID = 42;

    QWidget *widget() const { return m_widget; }

private:
    QWidget *m_widget;
    bool m_interactive;
};

} // namespace TextEditor

#endif // TIPCONTENTS_H
