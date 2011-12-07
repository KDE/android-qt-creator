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

#ifndef DOUBLETABWIDGET_H
#define DOUBLETABWIDGET_H

#include <QtCore/QVector>
#include <QtGui/QWidget>
#include <QtGui/QPixmap>

namespace ProjectExplorer {
namespace Internal {

namespace Ui {
    class DoubleTabWidget;
}

class DoubleTabWidget : public QWidget
{
    Q_OBJECT
public:
    DoubleTabWidget(QWidget *parent = 0);
    ~DoubleTabWidget();

    void setTitle(const QString &title);
    QString title() const { return m_title; }

    void addTab(const QString &name, const QString &fullName, const QStringList &subTabs);
    void insertTab(int index, const QString &name, const QString &fullName, const QStringList &subTabs);
    void removeTab(int index);
    int tabCount() const;

    int currentIndex() const;
    void setCurrentIndex(int index);

    int currentSubIndex() const;

signals:
    void currentIndexChanged(int index, int subIndex);

protected:
    void paintEvent(QPaintEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void changeEvent(QEvent *e);
    bool event(QEvent *event);
    QSize minimumSizeHint() const;

private:
    struct Tab {
        QString name;
        QString fullName;
        bool nameIsUnique;
        QStringList subTabs;
        int currentSubTab;
        QString displayName() const {
            return nameIsUnique ? name : fullName;
        }
    };
    void updateNameIsUniqueAdd(Tab *tab);
    void updateNameIsUniqueRemove(const Tab &tab);

    enum HitArea { HITNOTHING, HITOVERFLOW, HITTAB, HITSUBTAB };
    QPair<DoubleTabWidget::HitArea, int> convertPosToTab(QPoint pos);

    const QPixmap m_left;
    const QPixmap m_mid;
    const QPixmap m_right;

    Ui::DoubleTabWidget *ui;


    QString m_title;
    QList<Tab> m_tabs;
    int m_currentIndex;
    QVector<int> m_currentTabIndices;
    int m_lastVisibleIndex;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // DOUBLETABWIDGET_H
