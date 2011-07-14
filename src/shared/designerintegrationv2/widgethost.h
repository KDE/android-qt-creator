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

#ifndef WIDGETHOST_H
#define WIDGETHOST_H

#include <QtGui/QScrollArea>

QT_FORWARD_DECLARE_CLASS(QDesignerFormWindowInterface)

namespace SharedTools {

namespace Internal {
    class FormResizer;
}

/* A scroll area that embeds a Designer form window */

class WidgetHost : public QScrollArea
{
    Q_OBJECT
public:
    explicit WidgetHost(QWidget *parent = 0, QDesignerFormWindowInterface *formWindow = 0);
    virtual ~WidgetHost();
    // Show handles if active and main container is selected.
    void updateFormWindowSelectionHandles(bool active);

    inline QDesignerFormWindowInterface *formWindow() const { return m_formWindow; }

    QWidget *integrationContainer() const;

protected:
    void setFormWindow(QDesignerFormWindowInterface *fw);

signals:
    void formWindowSizeChanged(int, int);

private slots:
    void fwSizeWasChanged(const QRect &, const QRect &);

private:
    QSize formWindowSize() const;

    QDesignerFormWindowInterface *m_formWindow;
    Internal::FormResizer *m_formResizer;
    QSize m_oldFakeWidgetSize;
};

} // namespace SharedTools

#endif // WIDGETHOST_H
