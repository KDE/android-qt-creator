/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nicolas Arnaud-Cormos.
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

#include "macro.h"
#include "macroevent.h"

#include <app/app_version.h>
#include <utils/fileutils.h>

#include <QtCore/QFileInfo>
#include <QtCore/QDataStream>

using namespace Macros;

/*!
    \class Macros::Macro

    \brief Represents a macro, which is more or less a list of Macros::MacroEvent

    A macro is a list of events that can be replayed in Qt Creator. A macro has
    an header consisting of the Qt Creator version where the macro was created
    and a description.
    The name of the macro is the filename without the extension.
*/

class Macro::MacroPrivate
{
public:
    MacroPrivate();

    QString description;
    QString version;
    QString fileName;
    QList<MacroEvent> events;
};

Macro::MacroPrivate::MacroPrivate() :
    version(Core::Constants::IDE_VERSION_LONG)
{
}



// ---------- Macro ------------

Macro::Macro() :
    d(new MacroPrivate)
{
}

Macro::Macro(const Macro &other):
    d(new MacroPrivate)
{
    d->description = other.d->description;
    d->version = other.d->version;
    d->fileName = other.d->fileName;
    d->events = other.d->events;
}

Macro::~Macro()
{
    delete d;
}

Macro& Macro::operator=(const Macro &other)
{
    if (this == &other)
        return *this;
    d->description = other.d->description;
    d->version = other.d->version;
    d->fileName = other.d->fileName;
    d->events = other.d->events;
    return *this;
}

bool Macro::load(QString fileName)
{
    if (d->events.count())
        return true; // the macro is not empty

    // Take the current filename if the parameter is null
    if (fileName.isNull())
        fileName = d->fileName;
    else
        d->fileName = fileName;

    // Load all the macroevents
    QFile file(fileName);
    if (file.open(QFile::ReadOnly)) {
        QDataStream stream(&file);
        stream >> d->version;
        stream >> d->description;
        while (!stream.atEnd()) {
            MacroEvent macroEvent;
            macroEvent.load(stream);
            append(macroEvent);
        }
        return true;
    }
    return false;
}

bool Macro::loadHeader(const QString &fileName)
{
    d->fileName = fileName;
    QFile file(fileName);
    if (file.open(QFile::ReadOnly)) {
        QDataStream stream(&file);
        stream >> d->version;
        stream >> d->description;
        return true;
    }
    return false;
}

bool Macro::save(const QString &fileName, QWidget *parent)
{
    Utils::FileSaver saver(fileName);
    if (!saver.hasError()) {
        QDataStream stream(saver.file());
        stream << d->version;
        stream << d->description;
        foreach (const MacroEvent &event, d->events) {
            event.save(stream);
        }
        saver.setResult(&stream);
    }
    if (!saver.finalize(parent))
        return false;
    d->fileName = fileName;
    return true;
}

QString Macro::displayName() const
{
    QFileInfo fileInfo(d->fileName);
    return fileInfo.baseName();
}

void Macro::append(const MacroEvent &event)
{
    d->events.append(event);
}

const QString &Macro::description() const
{
    return d->description;
}

void Macro::setDescription(const QString &text)
{
    d->description = text;
}

const QString &Macro::version() const
{
    return d->version;
}

const QString &Macro::fileName() const
{
    return d->fileName;
}

const QList<MacroEvent> &Macro::events() const
{
    return d->events;
}

bool Macro::isWritable() const
{
    QFileInfo fileInfo(d->fileName);
    return fileInfo.exists() && fileInfo.isWritable();
}
