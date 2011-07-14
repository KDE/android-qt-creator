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

#ifndef METATYPEDECLARATIONS_H
#define METATYPEDECLARATIONS_H

#include <coreplugin/messagemanager.h>
#include <coreplugin/filemanager.h>
#include <coreplugin/ifile.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>

#include <QtCore/QList>
#include <QtCore/QMetaType>

QT_BEGIN_NAMESPACE
class QMainWindow;
class QStatusBar;
class QSettings;
QT_END_NAMESPACE

Q_DECLARE_METATYPE(Core::MessageManager*)
Q_DECLARE_METATYPE(Core::FileManager*)
Q_DECLARE_METATYPE(Core::IFile*)
Q_DECLARE_METATYPE(QList<Core::IFile*>)
Q_DECLARE_METATYPE(QList<Core::IEditor*>)
Q_DECLARE_METATYPE(Core::EditorManager*)
Q_DECLARE_METATYPE(Core::ICore*)

Q_DECLARE_METATYPE(QMainWindow*)
Q_DECLARE_METATYPE(QStatusBar*)
Q_DECLARE_METATYPE(QSettings*)

#endif // METATYPEDECLARATIONS_H
