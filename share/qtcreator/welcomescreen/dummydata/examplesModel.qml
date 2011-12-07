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

import QtQuick 1.0

ListModel {
    ListElement {
        name: "Main Windows"
        descriptionData: "All the standard features of application main windows are provided by Qt. Main windows can have pull down menus, tool bars, and dock windows. These separate forms of user input are unified in an integrated action system that also supports keyboard shortcuts and accelerator keys in menu items."
        imageSource: "images/mockup/mainwindow-examples.png"
    }

    ListElement {
        name: "Layouts"
        description: "t uses a layout-based approach to widget management. Widgets are arranged in the optimal positions in windows based on simple layout rules, leading to a consistent look and feel. Custom layouts can be used to provide more control over the positions and sizes of child widgets."
        imageSource: "images/mockup/layout-examples.png"
    }

    ListElement {
        name: "Item Views"
        description: "tem views are widgets that typically display data sets. Qt 4's model/view framework lets you handle large data sets by separating the underlying data from the way it is represented to the user, and provides support for customized rendering through the use of delegates."
        imageSource: "images/mockup/itemview-examples.png"
    }

    ListElement {
        name: "Drag and Drop"
        description: "Qt supports native drag and drop on all platforms via an extensible MIME-based system that enables applications to send data to each other in the most appropriate formats. Drag and drop can also be implemented for internal use by applications."
        imageSource: "images/mockup/draganddrop-examples.png"
    }
    ListElement {
        name: "Threading and Concurrent Programming"
        description: "Qt 4 makes it easier than ever to write multithreaded applications. More classes have been made usable from non-GUI threads, and the signals and slots mechanism can now be used to communicate between threads. The QtConcurrent namespace includes a collection of classes and functions for straightforward concurrent programming."
        imageSource: "images/mockup/thread-examples.png"
    }

    ListElement {
        name: "OpenGL and OpenVG Examples"
        description: "Qt provides support for integration with OpenGL implementations on all platforms, giving developers the opportunity to display hardware accelerated 3D graphics alongside a more conventional user interface. Qt provides support for integration with OpenVG implementations on platforms with suitable drivers."
        imageSource: "images/mockup/opengl-examples.png"
    }

    ListElement {
        name: "Network"
        description: "Qt is provided with an extensive set of network classes to support both client-based and server side network programming."
        imageSource: "images/mockup/network-examples.png"
    }

    ListElement {
        name: "Qt Designer"
        description: "Qt Designer is a capable graphical user interface designer that lets you create and configure forms without writing code. GUIs created with Qt Designer can be compiled into an application or created at run-time."
        imageSource: "images/mockup/designer-examples.png"
    }
    ListElement {
        name: "Qt Script"
        description: "Qt is provided with a powerful embedded scripting environment through the QtScript classes."
        imageSource: "images/mockup/qtscript-examples.png"
    }

    ListElement {
        name: "Desktop"
        description: "Qt provides features to enable applications to integrate with the user's preferred desktop environment. Features such as system tray icons, access to the desktop widget, and support for desktop services can be used to improve the appearance of applications and take advantage of underlying desktop facilities."
        imageSource: "images/mockup/desktop-examples.png"
    }

    ListElement {
        name: "Caption"
        description: "Description"
        imageSource: "image/mockup/penguin.png"
    }

    ListElement {
        name: "Caption"
        description: "Description"
        imageSource: "images/mockup/penguin.png"
    }
}
