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

#ifndef PROGRESSPIE_H
#define PROGRESSPIE_H

#include <QtCore/QString>
#include <QtGui/QWidget>

namespace Core {
namespace Internal {

class ProgressBar : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(float cancelButtonFader READ cancelButtonFader WRITE setCancelButtonFader)

public:
    explicit ProgressBar(QWidget *parent = 0);
    ~ProgressBar();

    QString title() const;
    void setTitle(const QString &title);
    // TODO rename setError
    void setError(bool on);
    bool hasError() const;
    QSize sizeHint() const;
    void paintEvent(QPaintEvent *);
    void mouseMoveEvent(QMouseEvent *);
    int minimum() const { return m_minimum; }
    int maximum() const { return m_maximum; }
    int value() const { return m_value; }
    bool finished() const { return m_finished; }
    void reset();
    void setRange(int minimum, int maximum);
    void setValue(int value);
    void setFinished(bool b);
    float cancelButtonFader() { return m_cancelButtonFader; }
    void setCancelButtonFader(float value) { update(); m_cancelButtonFader= value;}
    bool event(QEvent *);

signals:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent *event);

private:
    QImage bar;
    QString m_text;
    QString m_title;
    bool m_error;
    int m_progressHeight;
    int m_minimum;
    int m_maximum;
    int m_value;
    float m_cancelButtonFader;
    bool m_finished;
};

} // namespace Internal
} // namespace Core

#endif // PROGRESSPIE_H
