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

#include "basevalidatinglineedit.h"

#include <QtCore/QDebug>

enum { debug = 0 };

/*!
    \namespace Utils
    General utility library namespace
*/

/*! \class Utils::BaseValidatingLineEdit

    \brief Base class for line edits that perform validation.

    Performs validation in a virtual  validate() function to be implemented in
    derived classes.
    When invalid, the text color will turn red and a tooltip will
    contain the error message. This approach is less intrusive than a
    QValidator which will prevent the user from entering certain characters.

    The widget has a concept of an "initialText" which can be something like
    "<Enter name here>". This results in state 'DisplayingInitialText', which
    is not valid, but is not marked red.
*/


namespace Utils {

struct BaseValidatingLineEditPrivate {
    explicit BaseValidatingLineEditPrivate(const QWidget *w);

    const QColor m_okTextColor;
    QColor m_errorTextColor;

    BaseValidatingLineEdit::State m_state;
    QString m_errorMessage;
    QString m_initialText;
    bool m_firstChange;
};

BaseValidatingLineEditPrivate::BaseValidatingLineEditPrivate(const QWidget *w) :
    m_okTextColor(BaseValidatingLineEdit::textColor(w)),
    m_errorTextColor(Qt::red),
    m_state(BaseValidatingLineEdit::Invalid),
    m_firstChange(true)
{
}

BaseValidatingLineEdit::BaseValidatingLineEdit(QWidget *parent) :
    QLineEdit(parent),
    m_bd(new BaseValidatingLineEditPrivate(this))
{
    // Note that textChanged() is also triggered automagically by
    // QLineEdit::setText(), no need to trigger manually.
    connect(this, SIGNAL(textChanged(QString)), this, SLOT(slotChanged(QString)));
}

BaseValidatingLineEdit::~BaseValidatingLineEdit()
{
    delete m_bd;
}

QString BaseValidatingLineEdit::initialText() const
{
    return m_bd->m_initialText;
}

void BaseValidatingLineEdit::setInitialText(const QString &t)
{
    if (m_bd->m_initialText != t) {
        m_bd->m_initialText = t;
        m_bd->m_firstChange = true;
        setText(t);
    }
}

QColor BaseValidatingLineEdit::errorColor() const
{
    return m_bd->m_errorTextColor;
}

void BaseValidatingLineEdit::setErrorColor(const  QColor &c)
{
     m_bd->m_errorTextColor = c;
}

QColor BaseValidatingLineEdit::textColor(const QWidget *w)
{
    return w->palette().color(QPalette::Active, QPalette::Text);
}

void BaseValidatingLineEdit::setTextColor(QWidget *w, const QColor &c)
{
    QPalette palette = w->palette();
    palette.setColor(QPalette::Active, QPalette::Text, c);
    w->setPalette(palette);
}

BaseValidatingLineEdit::State BaseValidatingLineEdit::state() const
{
    return m_bd->m_state;
}

bool BaseValidatingLineEdit::isValid() const
{
    return m_bd->m_state == Valid;
}

QString BaseValidatingLineEdit::errorMessage() const
{
    return m_bd->m_errorMessage;
}

void BaseValidatingLineEdit::slotChanged(const QString &t)
{
    m_bd->m_errorMessage.clear();
    // Are we displaying the initial text?
    const bool isDisplayingInitialText = !m_bd->m_initialText.isEmpty() && t == m_bd->m_initialText;
    const State newState = isDisplayingInitialText ?
                               DisplayingInitialText :
                               (validate(t, &m_bd->m_errorMessage) ? Valid : Invalid);
    setToolTip(m_bd->m_errorMessage);
    if (debug)
        qDebug() << Q_FUNC_INFO << t << "State" <<  m_bd->m_state << "->" << newState << m_bd->m_errorMessage;
    // Changed..figure out if valid changed. DisplayingInitialText is not valid,
    // but should not show error color. Also trigger on the first change.
    if (newState != m_bd->m_state || m_bd->m_firstChange) {
        const bool validHasChanged = (m_bd->m_state == Valid) != (newState == Valid);
        m_bd->m_state = newState;
        m_bd->m_firstChange = false;
        setTextColor(this, newState == Invalid ? m_bd->m_errorTextColor : m_bd->m_okTextColor);
        if (validHasChanged) {
            emit validChanged(newState == Valid);
            emit validChanged();
        }
    }
}

void BaseValidatingLineEdit::slotReturnPressed()
{
    if (isValid())
        emit validReturnPressed();
}

void BaseValidatingLineEdit::triggerChanged()
{
    slotChanged(text());
}

} // namespace Utils
