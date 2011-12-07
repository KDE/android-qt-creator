/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Author: Frank Osterfeld, KDAB (frank.osterfeld@kdab.com)
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

#ifndef LIBVALGRIND_PROTOCOL_SUPPRESSION_H
#define LIBVALGRIND_PROTOCOL_SUPPRESSION_H

#include <QtCore/QSharedDataPointer>

QT_BEGIN_NAMESPACE
class QString;
template <typename T> class QVector;
QT_END_NAMESPACE

namespace Valgrind {
namespace XmlProtocol {

class SuppressionFrame
{
public:
    SuppressionFrame();
    SuppressionFrame(const SuppressionFrame &other);
    ~SuppressionFrame();
    SuppressionFrame &operator=(const SuppressionFrame &other);
    void swap(SuppressionFrame &other);
    bool operator==(const SuppressionFrame &other) const;
    bool operator!=(const SuppressionFrame &other) const
    {
        return !operator==(other);
    }

    QString object() const;
    void setObject(const QString &object);

    QString function() const;
    void setFunction(const QString &function);

    QString toString() const;

private:
    class Private;
    QSharedDataPointer<Private> d;
};

typedef QVector<SuppressionFrame> SuppressionFrames;

class Suppression
{
public:
    Suppression();
    Suppression(const Suppression &other);
    ~Suppression();
    Suppression &operator=(const Suppression &other);

    void swap(Suppression &other);
    bool operator==(const Suppression &other) const;

    bool isNull() const;

    QString name() const;
    void setName(const QString &name);

    QString kind() const;
    void setKind(const QString &kind);

    QString auxKind() const;
    void setAuxKind(const QString &kind);

    QString rawText() const;
    void setRawText(const QString &text);

    SuppressionFrames frames() const;
    void setFrames(const SuppressionFrames &frames);

    QString toString() const;

private:
    class Private;
    QSharedDataPointer<Private> d;
};

} // namespace XmlProtocol
} // namespace Valgrind

#endif // LIBVALGRIND_PROTOCOL_SUPPRESSION_H
