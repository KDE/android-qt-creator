/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
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
**************************************************************************/

#ifndef IFINDFILTER_H
#define IFINDFILTER_H

#include "find_global.h"
#include "textfindconstants.h"

#include <QtGui/QTextDocument>
#include <QtGui/QKeySequence>

QT_BEGIN_NAMESPACE
class QWidget;
class QSettings;
QT_END_NAMESPACE

namespace Find {

class FIND_EXPORT IFindFilter : public QObject
{
    Q_OBJECT
public:

    virtual ~IFindFilter() {}

    virtual QString id() const = 0;
    virtual QString displayName() const = 0;
    ///
    virtual bool isEnabled() const = 0;
    virtual bool canCancel() const = 0;
    virtual void cancel() = 0;
    virtual QKeySequence defaultShortcut() const { return QKeySequence(); }
    virtual bool isReplaceSupported() const { return false; }
    virtual FindFlags supportedFindFlags() const;

    virtual void findAll(const QString &txt, Find::FindFlags findFlags) = 0;
    virtual void replaceAll(const QString &txt, Find::FindFlags findFlags)
    { Q_UNUSED(txt) Q_UNUSED(findFlags) }

    virtual QWidget *createConfigWidget() { return 0; }
    virtual void writeSettings(QSettings *settings) { Q_UNUSED(settings) }
    virtual void readSettings(QSettings *settings) { Q_UNUSED(settings) }

signals:
    void changed();
};

} // namespace Find

#endif // IFINDFILTER_H
