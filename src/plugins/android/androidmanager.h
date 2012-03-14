/*
I BogDan Vatra < bog_dan_ro@yahoo.com >, the copyright holder of this work,
hereby release it into the public domain. This applies worldwide.

In case this is not legally possible, I grant any entity the right to use
this work for any purpose, without any conditions, unless such conditions
are required by law.
*/

#ifndef ANDROIDMANAGER_H
#define ANDROIDMANAGER_H

#include <QtCore/QObject>

#include <extensionsystem/iplugin.h>

namespace Android {

class AndroidManager : public ExtensionSystem::IPlugin
{
    Q_OBJECT

public:
    AndroidManager();
    ~AndroidManager();

    bool initialize(const QStringList &arguments, QString *error_message);
    void extensionsInitialized();

};
} // namespace Qt4ProjectManager

#endif // ANDROIDMANAGER_H
