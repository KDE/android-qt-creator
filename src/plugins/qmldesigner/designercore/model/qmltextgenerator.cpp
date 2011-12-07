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

#include <QtCore/QVariant>
#include <QtGui/QColor>

#include "bindingproperty.h"
#include "nodeproperty.h"
#include "nodelistproperty.h"
#include "qmltextgenerator.h"
#include "variantproperty.h"
#include <nodemetainfo.h>
#include "model.h"

using namespace QmlDesigner;
using namespace QmlDesigner::Internal;

inline static QString properColorName(const QColor &color)
{
    QString s;
    if (color.alpha() == 255)
        s.sprintf("#%02x%02x%02x", color.red(), color.green(), color.blue());
    else
        s.sprintf("#%02x%02x%02x%02x", color.alpha(), color.red(), color.green(), color.blue());
    return s;
}

inline static QString doubleToString(double d)
{
    QString string = QString::number(d, 'f', 3);
    if (string.endsWith(".000"))
        string.chop(4);
    return string;
}

QmlTextGenerator::QmlTextGenerator(const QStringList &propertyOrder, int indentDepth):
        m_propertyOrder(propertyOrder),
        m_indentDepth(indentDepth)
{
}

QString QmlTextGenerator::toQml(const AbstractProperty &property, int indentDepth) const
{
    if (property.isBindingProperty()) {
        return property.toBindingProperty().expression();
    } else if (property.isNodeProperty()) {
        return toQml(property.toNodeProperty().modelNode(), indentDepth);
    } else if (property.isNodeListProperty()) {
        const QList<ModelNode> nodes = property.toNodeListProperty().toModelNodeList();
        if (property.isDefaultProperty()) {
            QString result;
            for (int i = 0; i < nodes.length(); ++i) {
                if (i > 0)
                    result += QLatin1String("\n\n");
                result += QString(indentDepth, QLatin1Char(' '));
                result += toQml(nodes.at(i), indentDepth);
            }
            return result;
        } else {
            QString result = QLatin1String("[");
            const int arrayContentDepth = indentDepth + 4;
            const QString arrayContentIndentation(arrayContentDepth, QLatin1Char(' '));
            for (int i = 0; i < nodes.length(); ++i) {
                if (i > 0)
                    result += QLatin1Char(',');
                result += QLatin1Char('\n');
                result += arrayContentIndentation;
                result += toQml(nodes.at(i), arrayContentDepth);
            }
            return result + QLatin1Char(']');
        }
    } else if (property.isVariantProperty()) {
        const VariantProperty variantProperty = property.toVariantProperty();
        const QVariant value = variantProperty.value();
        const QString stringValue = value.toString();

        if (property.name() == QLatin1String("id"))
            return stringValue;

          if (false) {
          }
        if (variantProperty.parentModelNode().metaInfo().isValid() &&
            variantProperty.parentModelNode().metaInfo().propertyIsEnumType(variantProperty.name())) {
            return variantProperty.parentModelNode().metaInfo().propertyEnumScope(variantProperty.name()) + '.' + stringValue;
        } else {

            switch (value.type()) {
            case QVariant::Bool:
                if (value.value<bool>())
                    return QLatin1String("true");
                else
                    return QLatin1String("false");

            case QVariant::Color:
                return QString(QLatin1String("\"%1\"")).arg(properColorName(value.value<QColor>()));

            case QVariant::Double:
                return doubleToString(value.toDouble());
            case QVariant::Int:
            case QVariant::LongLong:
            case QVariant::UInt:
            case QVariant::ULongLong:
                return stringValue;

            default:
                return QString(QLatin1String("\"%1\"")).arg(escape(stringValue));
            }
        }
    } else {
        Q_ASSERT("Unknown property type");
        return QString();
    }
}

QString QmlTextGenerator::toQml(const ModelNode &node, int indentDepth) const
{
    QString type = node.type();
    QString url;
    if (type.contains('.')) {
        QStringList nameComponents = type.split('.');
        url = nameComponents.first();
        type = nameComponents.last();
    }

    QString alias;
    if (!url.isEmpty()) {
        const QString &versionUrl = QString("%1.%2").arg(QString::number(node.majorVersion()), QString::number(node.minorVersion()));
        foreach (const Import &import, node.model()->imports()) {
            if (import.url() == url
                && import.version() == versionUrl) {
                alias = import.alias();
                break;
            }
        }
    }

    QString result;

    if (!alias.isEmpty())
        result = alias + '.';

    result += type;
    result += QLatin1String(" {\n");

    const int propertyIndentDepth = indentDepth + 4;

    const QString properties = propertiesToQml(node, propertyIndentDepth);

    return result + properties + QString(indentDepth, QLatin1Char(' ')) + QLatin1Char('}');
}

QString QmlTextGenerator::propertiesToQml(const ModelNode &node, int indentDepth) const
{
    QString topPart;
    QString bottomPart;

    QStringList nodePropertyNames = node.propertyNames();
    bool addToTop = true;

    foreach (const QString &propertyName, m_propertyOrder) {
        if (QLatin1String("id") == propertyName) {
            // the model handles the id property special, so:
            if (!node.id().isEmpty()) {
                QString idLine(indentDepth, QLatin1Char(' '));
                idLine += QLatin1String("id: ");
                idLine += node.id();
                idLine += QLatin1Char('\n');

                if (addToTop)
                    topPart.append(idLine);
                else
                    bottomPart.append(idLine);
            }
        } else if (propertyName.isEmpty()) {
            addToTop = false;
        } else if (nodePropertyNames.removeAll(propertyName)) {
            const QString newContent = propertyToQml(node.property(propertyName), indentDepth);

            if (addToTop)
                topPart.append(newContent);
            else
                bottomPart.append(newContent);
        }
    }

    foreach (const QString &propertyName, nodePropertyNames) {
        bottomPart.prepend(propertyToQml(node.property(propertyName), indentDepth));
    }

    return topPart + bottomPart;
}

QString QmlTextGenerator::propertyToQml(const AbstractProperty &property, int indentDepth) const
{
    QString result;

    if (property.isDefaultProperty())
        result = toQml(property, indentDepth);
    else
        result = QString(indentDepth, QLatin1Char(' ')) + property.name() + QLatin1String(": ") + toQml(property, indentDepth);

    result += QLatin1Char('\n');

    return result;
}

QString QmlTextGenerator::escape(const QString &value)
{
    QString result = value;

    result.replace(QLatin1String("\\"), QLatin1String("\\\\"));

    result.replace(QLatin1String("\""), QLatin1String("\\\""));
    result.replace(QLatin1String("\t"), QLatin1String("\\t"));
    result.replace(QLatin1String("\r"), QLatin1String("\\r"));
    result.replace(QLatin1String("\n"), QLatin1String("\\n"));

    return result;
}
