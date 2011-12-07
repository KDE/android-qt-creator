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

#include "helpitem.h"

#include <coreplugin/helpmanager.h>
#include <utils/htmldocextractor.h>

#include <QtCore/QUrl>
#include <QtCore/QByteArray>
#include <QtCore/QMap>

using namespace TextEditor;

HelpItem::HelpItem()
{}

HelpItem::HelpItem(const QString &helpId, Category category) :
    m_helpId(helpId), m_docMark(helpId), m_category(category)
{}

HelpItem::HelpItem(const QString &helpId, const QString &docMark, Category category) :
    m_helpId(helpId), m_docMark(docMark), m_category(category)
{}

HelpItem::~HelpItem()
{}

void HelpItem::setHelpId(const QString &id)
{ m_helpId = id; }

const QString &HelpItem::helpId() const
{ return m_helpId; }

void HelpItem::setDocMark(const QString &mark)
{ m_docMark = mark; }

const QString &HelpItem::docMark() const
{ return m_docMark; }

void HelpItem::setCategory(Category cat)
{ m_category = cat; }

HelpItem::Category HelpItem::category() const
{ return m_category; }

bool HelpItem::isValid() const
{
    if (!Core::HelpManager::instance()->linksForIdentifier(m_helpId).isEmpty())
        return true;
    if (QUrl(m_helpId).isValid())
        return true;
    return false;
}

QString HelpItem::extractContent(bool extended) const
{
    Utils::HtmlDocExtractor htmlExtractor;
    if (extended)
        htmlExtractor.setMode(Utils::HtmlDocExtractor::Extended);
    else
        htmlExtractor.setMode(Utils::HtmlDocExtractor::FirstParagraph);

    QString contents;
    QMap<QString, QUrl> helpLinks = Core::HelpManager::instance()->linksForIdentifier(m_helpId);
    if (helpLinks.isEmpty()) {
        // Maybe this is already an URL...
        QUrl url(m_helpId);
        if (url.isValid())
            helpLinks.insert(m_helpId, m_helpId);
    }
    foreach (const QUrl &url, helpLinks) {
        const QByteArray &html = Core::HelpManager::instance()->fileData(url);
        switch (m_category) {
        case Brief:
            contents = htmlExtractor.getClassOrNamespaceBrief(html, m_docMark);
            break;
        case ClassOrNamespace:
            contents = htmlExtractor.getClassOrNamespaceDescription(html, m_docMark);
            break;
        case Function:
            contents = htmlExtractor.getFunctionDescription(html, m_docMark);
            break;
        case Enum:
            contents = htmlExtractor.getEnumDescription(html, m_docMark);
            break;
        case Typedef:
            contents = htmlExtractor.getTypedefDescription(html, m_docMark);
            break;
        case Macro:
            contents = htmlExtractor.getMacroDescription(html, m_docMark);
            break;
        case QmlComponent:
            contents = htmlExtractor.getQmlComponentDescription(html, m_docMark);
            break;
        case QmlProperty:
            contents = htmlExtractor.getQmlPropertyDescription(html, m_docMark);
            break;
        case QMakeVariableOfFunction:
            contents = htmlExtractor.getQMakeVariableOrFunctionDescription(html, m_docMark);
            break;

        default:
            break;
        }

        if (!contents.isEmpty())
            break;
    }
    return contents;
}
