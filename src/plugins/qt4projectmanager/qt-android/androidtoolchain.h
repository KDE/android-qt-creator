/*
I BogDan Vatra < bog_dan_ro@yahoo.com >, the copyright holder of this work,
hereby release it into the public domain. This applies worldwide.

In case this is not legally possible, I grant any entity the right to use
this work for any purpose, without any conditions, unless such conditions
are required by law.
*/

#ifndef ANDROIDTOOLCHAIN_H
#define ANDROIDTOOLCHAIN_H

#include <projectexplorer/toolchain.h>

namespace Qt4ProjectManager {
    class QtVersion;
    namespace Internal {

class AndroidToolChain : public ProjectExplorer::GccToolChain
{
public:
    AndroidToolChain(const QString &gccPath);
    virtual ~AndroidToolChain();

    void addToEnvironment(Utils::Environment &env);
    ProjectExplorer::ToolChainType type() const;
    QString makeCommand() const;


protected:
    bool equals(const ToolChain *other) const;

private:
};

    } // namespace Internal
} // namespace Qt4ProjectManager

#endif // ANDROIDTOOLCHAIN_H
