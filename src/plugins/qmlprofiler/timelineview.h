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

#ifndef TIMELINEVIEW_H
#define TIMELINEVIEW_H

#include <QtDeclarative/QDeclarativeItem>
#include <QtScript/QScriptValue>
#include <qmljsdebugclient/qmlprofilereventlist.h>

namespace QmlProfiler {
namespace Internal {

class TimelineView : public QDeclarativeItem
{
    Q_OBJECT
    Q_PROPERTY(qint64 startTime READ startTime WRITE setStartTime NOTIFY startTimeChanged)
    Q_PROPERTY(qint64 endTime READ endTime WRITE setEndTime NOTIFY endTimeChanged)
    Q_PROPERTY(QObject* eventList READ eventList WRITE setEventList NOTIFY eventListChanged)
    Q_PROPERTY(bool selectionLocked READ selectionLocked WRITE setSelectionLocked NOTIFY selectionLockedChanged)
    Q_PROPERTY(int selectedItem READ selectedItem WRITE setSelectedItem NOTIFY selectedItemChanged)
    Q_PROPERTY(int startDragArea READ startDragArea WRITE setStartDragArea NOTIFY startDragAreaChanged)
    Q_PROPERTY(int endDragArea READ endDragArea WRITE setEndDragArea NOTIFY endDragAreaChanged)

public:
    explicit TimelineView(QDeclarativeItem *parent = 0);

    qint64 startTime() const
    {
        return m_startTime;
    }

    qint64 endTime() const
    {
        return m_endTime;
    }

    bool selectionLocked() const
    {
        return m_selectionLocked;
    }

    int selectedItem() const
    {
        return m_selectedItem;
    }

    int startDragArea() const
    {
        return m_startDragArea;
    }

    int endDragArea() const
    {
        return m_endDragArea;
    }

    QmlJsDebugClient::QmlProfilerEventList *eventList() const { return m_eventList; }
    void setEventList(QObject *eventList)
    {
        m_eventList = qobject_cast<QmlJsDebugClient::QmlProfilerEventList *>(eventList);
        emit eventListChanged(m_eventList);
    }

    Q_INVOKABLE qint64 getDuration(int index) const;
    Q_INVOKABLE QString getFilename(int index) const;
    Q_INVOKABLE int getLine(int index) const;
    Q_INVOKABLE QString getDetails(int index) const;

    Q_INVOKABLE void setRowExpanded(int rowIndex, bool expanded);

    Q_INVOKABLE void selectNext();
    Q_INVOKABLE void selectPrev();
    Q_INVOKABLE int nextItemFromId(int eventId) const;
    Q_INVOKABLE int prevItemFromId(int eventId) const;
    Q_INVOKABLE void selectNextFromId(int eventId);
    Q_INVOKABLE void selectPrevFromId(int eventId);

signals:
    void startTimeChanged(qint64 arg);
    void endTimeChanged(qint64 arg);
    void eventListChanged(QmlJsDebugClient::QmlProfilerEventList *list);
    void selectionLockedChanged(bool locked);
    void selectedItemChanged(int itemIndex);
    void startDragAreaChanged(int startDragArea);
    void endDragAreaChanged(int endDragArea);
    void itemPressed(int pressedItem);

public slots:
    void clearData();
    void requestPaint();


    void setStartTime(qint64 arg)
    {
        if (m_startTime != arg) {
            m_startTime = arg;
            emit startTimeChanged(arg);
        }
    }

    void setEndTime(qint64 arg)
    {
        if (m_endTime != arg) {
            m_endTime = arg;
            emit endTimeChanged(arg);
        }
    }

    void setSelectionLocked(bool locked)
    {
        if (m_selectionLocked != locked) {
            m_selectionLocked = locked;
            update();
            emit selectionLockedChanged(locked);
        }
    }

    void setSelectedItem(int itemIndex)
    {
        if (m_selectedItem != itemIndex) {
            m_selectedItem = itemIndex;
            update();
            emit selectedItemChanged(itemIndex);
        }
    }

    void setStartDragArea(int startDragArea)
    {
        if (m_startDragArea != startDragArea) {
            m_startDragArea = startDragArea;
            emit startDragAreaChanged(startDragArea);
        }
    }

    void setEndDragArea(int endDragArea)
    {
        if (m_endDragArea != endDragArea) {
            m_endDragArea = endDragArea;
            emit endDragAreaChanged(endDragArea);
        }
    }

protected:
    virtual void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *);
    virtual void componentComplete();
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    virtual void hoverMoveEvent(QGraphicsSceneHoverEvent *event);

private:
    QColor colorForItem(int itemIndex);
    void drawItemsToPainter(QPainter *p, int fromIndex, int toIndex);
    void drawSelectionBoxes(QPainter *p);

    void manageClicked();
    void manageHovered(int x, int y);

private:
    qint64 m_startTime;
    qint64 m_endTime;
    qreal m_spacing;
    qint64 m_lastStartTime;
    qint64 m_lastEndTime;

    QmlJsDebugClient::QmlProfilerEventList *m_eventList;

    QList<int> m_rowLastX;
    QList<int> m_rowStarts;
    QList<int> m_rowWidths;
    QList<bool> m_rowsExpanded;

    struct {
        qint64 startTime;
        qint64 endTime;
        int row;
        int eventIndex;
    } m_currentSelection;

    int m_selectedItem;
    bool m_selectionLocked;
    int m_startDragArea;
    int m_endDragArea;
};

} // namespace Internal
} // namespace QmlProfiler

QML_DECLARE_TYPE(QmlProfiler::Internal::TimelineView)

#endif // TIMELINEVIEW_H
