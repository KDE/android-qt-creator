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

#include "propertyparser.h"
#include <modelnode.h>
#include <metainfo.h>

#include <QUrl>
#include <QDebug>

#include <private/qdeclarativestringconverters_p.h>

namespace QmlDesigner {
namespace Internal {
namespace PropertyParser {

QVariant read(const QString &typeStr, const QString &str, const MetaInfo &)
{
    return read(typeStr, str);
}

QVariant read(const QString &typeStr, const QString &str)
{
    int type = QMetaType::type(typeStr.toAscii().constData());
    if (type == 0) {
        qWarning() << "Type " << typeStr
                << " is unknown to QMetaType system. Cannot create properly typed QVariant for value "
                << str;
        // Fall back to a QVariant of type String
        return QVariant(str);
    }
    return read(type, str);
}

QVariant read(int variantType, const QString &str)
{
    QVariant value;

    bool conversionOk = true;
    switch (variantType) {
    case QMetaType::QPoint:
        value = QDeclarativeStringConverters::pointFFromString(str, &conversionOk).toPoint();
        break;
    case QMetaType::QPointF:
        value = QDeclarativeStringConverters::pointFFromString(str, &conversionOk);
        break;
    case QMetaType::QSize:
        value = QDeclarativeStringConverters::sizeFFromString(str, &conversionOk).toSize();
        break;
    case QMetaType::QSizeF:
        value = QDeclarativeStringConverters::sizeFFromString(str, &conversionOk);
        break;
    case QMetaType::QRect:
        value = QDeclarativeStringConverters::rectFFromString(str, &conversionOk).toRect();
        break;
    case QMetaType::QRectF:
        value = QDeclarativeStringConverters::rectFFromString(str, &conversionOk);
        break;
    case QMetaType::QUrl:
        value = QVariant(QUrl(str));
        break;
    case QMetaType::QColor:
        value = QDeclarativeStringConverters::colorFromString(str);
        break;
    default: {
        value = QVariant(str);
        value.convert(static_cast<QVariant::Type>(variantType));
        break;
        }
    }

    if (!conversionOk) {
        qWarning() << "Could not convert" << str
                   << "to" << QMetaType::typeName(variantType);
        value = QVariant(str);
    }

    return value;
}

QString write(const QVariant &variant)
{
    if (!variant.isValid()) {
        qWarning() << "Trying to serialize invalid QVariant";
        return QString();
    }
    QString value;
    switch (variant.type()) {
    case QMetaType::QPoint:
    {
        QPoint p = variant.toPoint();
        value = QString("%1,%2").arg(QString::number(p.x()), QString::number(p.y()));
        break;
    }
    case QMetaType::QPointF:
    {
        QPointF p = variant.toPointF();
        value = QString("%1,%2").arg(QString::number(p.x(), 'f'), QString::number(p.y(), 'f'));
        break;
    }
    case QMetaType::QSize:
    {
        QSize s = variant.toSize();
        value = QString("%1x%2").arg(QString::number(s.width()), QString::number(s.height()));
        break;
    }
    case QMetaType::QSizeF:
    {
        QSizeF s = variant.toSizeF();
        value = QString("%1x%2").arg(QString::number(s.width(), 'f'), QString::number(s.height(), 'f'));
        break;
    }
    case QMetaType::QRect:
    {
        QRect r = variant.toRect();
        value = QString("%1,%2,%3x%4").arg(QString::number(r.x()), QString::number(r.y()),
                                           QString::number(r.width()), QString::number(r.height()));
        break;
    }
    case QMetaType::QRectF:
    {
        QRectF r = variant.toRectF();
        value = QString("%1,%2,%3x%4").arg(QString::number(r.x(), 'f'), QString::number(r.y(), 'f'),
                                           QString::number(r.width(), 'f'), QString::number(r.height(), 'f'));
        break;
    }
    default:
        QVariant strVariant = variant;
        strVariant.convert(QVariant::String);
        if (!strVariant.isValid())
            qWarning() << Q_FUNC_INFO << "cannot serialize type " << QMetaType::typeName(variant.type());
        value = strVariant.toString();
    }

    return value;
}

} // namespace PropertyParser
} // namespace Internal
} // namespace Designer

