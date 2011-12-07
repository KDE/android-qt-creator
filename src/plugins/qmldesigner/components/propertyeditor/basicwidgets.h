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

#ifndef BASICWIDGETS_H
#define BASICWIDGETS_H

#include <qdeclarative.h>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QLabel>
#include <QPushButton>
#include <QToolButton>
#include <QCheckBox>
#include <QRadioButton>
#include <QLineEdit>
#include <QTextEdit>
#include <QPlainTextEdit>
#include <QDoubleSpinBox>
#include <QSlider>
#include <QDateEdit>
#include <QTimeEdit>
#include <QProgressBar>
#include <QGroupBox>
#include <QDial>
#include <QLCDNumber>
#include <QFontComboBox>
#include <QScrollBar>
#include <QCalendarWidget>
#include <QTabWidget>
#include <QAction>
#include "filewidget.h"
#include "layoutwidget.h"

QML_DECLARE_TYPE(QWidget)

//display
QML_DECLARE_TYPE(QLabel)
QML_DECLARE_TYPE(QProgressBar)
QML_DECLARE_TYPE(QLCDNumber)

//input
QML_DECLARE_TYPE(QLineEdit)
QML_DECLARE_TYPE(QTextEdit)
QML_DECLARE_TYPE(QPlainTextEdit)
QML_DECLARE_TYPE(QSpinBox)
QML_DECLARE_TYPE(QDoubleSpinBox)
QML_DECLARE_TYPE(QSlider)
QML_DECLARE_TYPE(QDateTimeEdit)
QML_DECLARE_TYPE(QDateEdit)
QML_DECLARE_TYPE(QTimeEdit)
QML_DECLARE_TYPE(QFontComboBox)
QML_DECLARE_TYPE(QDial)
QML_DECLARE_TYPE(QScrollBar)
QML_DECLARE_TYPE(QCalendarWidget)
QML_DECLARE_TYPE(QComboBox)

//buttons
QML_DECLARE_TYPE(QPushButton)
QML_DECLARE_TYPE(QToolButton)
QML_DECLARE_TYPE(QCheckBox)
QML_DECLARE_TYPE(QRadioButton)

//containers
QML_DECLARE_TYPE(QGroupBox)
QML_DECLARE_TYPE(QFrame)
QML_DECLARE_TYPE(QScrollArea)
QML_DECLARE_TYPE(QTabWidget)
QML_DECLARE_TYPE(FileWidget)
QML_DECLARE_TYPE(LayoutWidget)


class Action : public QAction {
    Q_OBJECT
public:
    Action(QObject *parent = 0) : QAction(parent) {}
};

QML_DECLARE_TYPE(QMenu)
QML_DECLARE_TYPE(Action)

//QML_DECLARE_TYPE(QToolBox)

//itemviews
//QML_DECLARE_TYPE(QListView)
//QML_DECLARE_TYPE(QTreeView)
//QML_DECLARE_TYPE(QTableView)

//top-level windows?
class BasicWidgets {
public:
    static void registerDeclarativeTypes();
};

#endif // BASICWIDGETS_H
