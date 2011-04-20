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

// WARNING: This code is shared with the qmlplugindump tool code in Qt.
//          Modifications to this file need to be applied there.

#include "qmlstreamwriter.h"

#include <QtCore/QBuffer>
#include <QtCore/QStringList>

QmlStreamWriter::QmlStreamWriter(QByteArray *array)
    : m_indentDepth(0)
    , m_pendingLineLength(0)
    , m_maybeOneline(false)
    , m_stream(new QBuffer(array))
{
    m_stream->open(QIODevice::WriteOnly);
}

void QmlStreamWriter::writeStartDocument()
{
}

void QmlStreamWriter::writeEndDocument()
{
}

void QmlStreamWriter::writeLibraryImport(const QString &uri, int majorVersion, int minorVersion, const QString &as)
{
    m_stream->write(QString("import %1 %2.%3").arg(uri, QString::number(majorVersion), QString::number(minorVersion)).toUtf8());
    if (!as.isEmpty())
        m_stream->write(QString(" as %1").arg(as).toUtf8());
    m_stream->write("\n");
}

void QmlStreamWriter::writeStartObject(const QString &component)
{
    flushPotentialLinesWithNewlines();
    writeIndent();
    m_stream->write(QString("%1 {").arg(component).toUtf8());
    ++m_indentDepth;
    m_maybeOneline = true;
}

void QmlStreamWriter::writeEndObject()
{
    if (m_maybeOneline && !m_pendingLines.isEmpty()) {
        --m_indentDepth;
        for (int i = 0; i < m_pendingLines.size(); ++i) {
            m_stream->write(" ");
            m_stream->write(m_pendingLines.at(i).trimmed());
            if (i != m_pendingLines.size() - 1)
                m_stream->write(";");
        }
        m_stream->write(" }\n");
        m_pendingLines.clear();
        m_pendingLineLength = 0;
        m_maybeOneline = false;
    } else {
        if (m_maybeOneline)
            flushPotentialLinesWithNewlines();
        --m_indentDepth;
        writeIndent();
        m_stream->write("}\n");
    }
}

void QmlStreamWriter::writeScriptBinding(const QString &name, const QString &rhs)
{
    writePotentialLine(QString("%1: %2").arg(name, rhs).toUtf8());
}

void QmlStreamWriter::writeArrayBinding(const QString &name, const QStringList &elements)
{
    flushPotentialLinesWithNewlines();
    writeIndent();
    m_stream->write(QString("%1: [\n").arg(name).toUtf8());
    ++m_indentDepth;
    for (int i = 0; i < elements.size(); ++i) {
        writeIndent();
        m_stream->write(elements.at(i).toUtf8());
        if (i != elements.size() - 1) {
            m_stream->write(",\n");
        } else {
            m_stream->write("\n");
        }
    }
    --m_indentDepth;
    writeIndent();
    m_stream->write("]\n");
}

void QmlStreamWriter::write(const QString &data)
{
    flushPotentialLinesWithNewlines();
    m_stream->write(data.toUtf8());
}

void QmlStreamWriter::writeScriptObjectLiteralBinding(const QString &name, const QList<QPair<QString, QString> > &keyValue)
{
    flushPotentialLinesWithNewlines();
    writeIndent();
    m_stream->write(QString("%1: {\n").arg(name).toUtf8());
    ++m_indentDepth;
    for (int i = 0; i < keyValue.size(); ++i) {
        const QString key = keyValue.at(i).first;
        const QString value = keyValue.at(i).second;
        writeIndent();
        m_stream->write(QString("%1: %2").arg(key, value).toUtf8());
        if (i != keyValue.size() - 1) {
            m_stream->write(",\n");
        } else {
            m_stream->write("\n");
        }
    }
    --m_indentDepth;
    writeIndent();
    m_stream->write("}\n");
}

void QmlStreamWriter::writeIndent()
{
    m_stream->write(QByteArray(m_indentDepth * 4, ' '));
}

void QmlStreamWriter::writePotentialLine(const QByteArray &line)
{
    m_pendingLines.append(line);
    m_pendingLineLength += line.size();
    if (m_pendingLineLength >= 80) {
        flushPotentialLinesWithNewlines();
    }
}

void QmlStreamWriter::flushPotentialLinesWithNewlines()
{
    if (m_maybeOneline)
        m_stream->write("\n");
    foreach (const QByteArray &line, m_pendingLines) {
        writeIndent();
        m_stream->write(line);
        m_stream->write("\n");
    }
    m_pendingLines.clear();
    m_pendingLineLength = 0;
    m_maybeOneline = false;
}
