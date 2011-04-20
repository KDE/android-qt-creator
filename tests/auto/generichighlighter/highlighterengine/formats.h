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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef FORMATS_H
#define FORMATS_H

#include <QtGui/QTextCharFormat>

class Formats
{
public:
    static Formats &instance();

    const QTextCharFormat &keywordFormat() const { return m_keywordFormat; }
    const QTextCharFormat &dataTypeFormat() const { return m_dataTypeFormat; }
    const QTextCharFormat &decimalFormat() const { return m_decimalFormat; }
    const QTextCharFormat &baseNFormat() const { return m_baseNFormat; }
    const QTextCharFormat &floatFormat() const { return m_floatFormat; }
    const QTextCharFormat &charFormat() const { return m_charFormat; }
    const QTextCharFormat &stringFormat() const { return m_stringFormat; }
    const QTextCharFormat &commentFormat() const { return m_commentFormat; }
    const QTextCharFormat &alertFormat() const { return m_alertFormat; }
    const QTextCharFormat &errorFormat() const { return m_errorFormat; }
    const QTextCharFormat &functionFormat() const { return m_functionFormat; }
    const QTextCharFormat &regionMarketFormat() const { return m_regionMarkerFormat; }
    const QTextCharFormat &othersFormat() const { return m_othersFormat; }

    QString name(const QTextCharFormat &format) const;

private:
    Formats();
    Q_DISABLE_COPY(Formats);

    QTextCharFormat m_keywordFormat;
    QTextCharFormat m_dataTypeFormat;
    QTextCharFormat m_decimalFormat;
    QTextCharFormat m_baseNFormat;
    QTextCharFormat m_floatFormat;
    QTextCharFormat m_charFormat;
    QTextCharFormat m_stringFormat;
    QTextCharFormat m_commentFormat;
    QTextCharFormat m_alertFormat;
    QTextCharFormat m_errorFormat;
    QTextCharFormat m_functionFormat;
    QTextCharFormat m_regionMarkerFormat;
    QTextCharFormat m_othersFormat;
};

#endif // FORMATS_H
