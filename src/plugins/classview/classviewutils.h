/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Denis Mingulov.
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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef CLASSVIEWUTILS_H
#define CLASSVIEWUTILS_H

#include "classviewsymbollocation.h"
#include "classviewsymbolinformation.h"
#include <QtCore/QVariant>
#include <QtCore/QList>
#include <QtCore/QSet>

QT_FORWARD_DECLARE_CLASS(QStandardItem)

namespace ClassView {
namespace Internal {

/*!
   \class Utils
   \brief Some common utils
 */

class Utils
{
    //! Private constructor
    Utils();
public:

    /*!
       \brief convert internal location container to QVariant compatible
       \param locations Set of SymbolLocations
       \return List of variant locations (can be added to an item's data)
     */
    static QList<QVariant> locationsToRole(const QSet<SymbolLocation> &locations);

    /*!
       \brief convert QVariant location container to internal
       \param locations List of variant locations (from an item's data)
       \return Set of SymbolLocations
     */
    static QSet<SymbolLocation> roleToLocations(const QList<QVariant> &locations);

    /*!
       \brief Returns sort order value for the icon type
       \param iconType Icon type
       \return Sort order value for the provided icon type
     */
    static int iconTypeSortOrder(int iconType);

    /*!
       \brief Get symbol information for the \a QStandardItem
       \param item Item
       \return Filled symbol information.
     */
    static SymbolInformation symbolInformationFromItem(const QStandardItem *item);

    /*!
       \brief Set symbol information to the \a QStandardItem
       \param information
       \param item Item
       \return Filled item
     */
    static QStandardItem *setSymbolInformationToItem(const SymbolInformation &information,
                                                     QStandardItem *item);

    /*!
       \brief Update an item to the target. (sorted, for fetching)
       \param item Source item
       \param target Target item
     */
    static void fetchItemToTarget(QStandardItem *item, const QStandardItem *target);

    /*!
       \brief Move an item to the target. (sorted)
       \param item Source item
       \param target Target item
     */
    static void moveItemToTarget(QStandardItem *item, const QStandardItem *target);
};

} // namespace Internal
} // namespace ClassView

#endif // CLASSVIEWUTILS_H
