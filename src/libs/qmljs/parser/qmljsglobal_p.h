/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef QMLJSGLOBAL_P_H
#define QMLJSGLOBAL_P_H

#include <QtCore/qglobal.h>

#ifdef QT_CREATOR
#  define QT_QML_BEGIN_NAMESPACE
#  define QT_QML_END_NAMESPACE

#  ifdef QMLJS_BUILD_DIR
#    define QML_PARSER_EXPORT Q_DECL_EXPORT
#  elif QML_BUILD_STATIC_LIB
#    define QML_PARSER_EXPORT
#  else
#    define QML_PARSER_EXPORT Q_DECL_IMPORT
#  endif // QMLJS_BUILD_DIR

#else // !QT_CREATOR
#  define QT_QML_BEGIN_NAMESPACE QT_BEGIN_NAMESPACE
#  define QT_QML_END_NAMESPACE QT_END_NAMESPACE
#  define QML_PARSER_EXPORT
#endif // QT_CREATOR

#endif // QMLJSGLOBAL_P_H
