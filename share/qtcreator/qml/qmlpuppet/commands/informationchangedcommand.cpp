/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "informationchangedcommand.h"

#include <QMetaType>

#include "propertyvaluecontainer.h"

namespace QmlDesigner {

InformationChangedCommand::InformationChangedCommand()
{
}

InformationChangedCommand::InformationChangedCommand(const QVector<InformationContainer> &informationVector)
    : m_informationVector(informationVector)
{
}

QVector<InformationContainer> InformationChangedCommand::informations() const
{
    return m_informationVector;
}

QDataStream &operator<<(QDataStream &out, const InformationChangedCommand &command)
{
    out << command.informations();

    return out;
}

QDataStream &operator>>(QDataStream &in, InformationChangedCommand &command)
{
    in >> command.m_informationVector;

    return in;
}

QDebug operator<<(QDebug debug, const InformationChangedCommand &command)
{
    debug << QLatin1String("InformationChangedCommand:\n");
    foreach (const InformationContainer &information, command.informations()) {
        if (information.nameAsString() == "Transform" ||
            information.nameAsString() == "IsMovable" ||
            information.nameAsString() == "BoundingRect") {

            debug << QLatin1String("instanceId: ");
            debug << information.instanceId();
            debug << QLatin1String("\n");
            debug << QLatin1String("name: ");
            debug << information.nameAsString();
            debug << QLatin1String("\n");
            debug << information.information();
            debug << QLatin1String("\n");
            debug <<  information.secondInformation();
            debug << QLatin1String("\n");
            debug <<  information.thirdInformation();
            debug << QLatin1String("\n");
            debug << QLatin1String("\n");
        }
    }

    return debug;
}

} // namespace QmlDesigner
