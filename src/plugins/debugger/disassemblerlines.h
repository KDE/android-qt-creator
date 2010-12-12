/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef DEBUGGER_DISASSEMBLERLINES_H
#define DEBUGGER_DISASSEMBLERLINES_H

#include <QtCore/QString>
#include <QtCore/QHash>
#include <QtCore/QVector>

namespace Debugger {
namespace Internal {

class DisassemblerLine
{
public:
    DisassemblerLine() : address(0) {}
    DisassemblerLine(const QString &unparsed);

public:
    quint64 address;
    QString data;
};

class DisassemblerLines
{
public:
    DisassemblerLines() {}

    bool coversAddress(quint64 address) const;
    void appendLine(const DisassemblerLine &dl);
    void appendComment(const QString &comment);
    int size() const { return m_data.size(); }
    const DisassemblerLine &at(int i) const { return m_data.at(i); }
    int lineForAddress(quint64 address) const;

private:
    QVector<DisassemblerLine> m_data;
    QHash<quint64, int> m_rowCache;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_DISASSEMBLERLINES_H
