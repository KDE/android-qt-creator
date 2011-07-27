/*
I BogDan Vatra < bog_dan_ro@yahoo.com >, the copyright holder of this work,
hereby release it into the public domain. This applies worldwide.

In case this is not legally possible, I grant any entity the right to use
this work for any purpose, without any conditions, unless such conditions
are required by law.
*/

#ifndef JAVAPARSER_H
#define JAVAPARSER_H
#include <projectexplorer/ioutputparser.h>

namespace Android {
namespace Internal {

class JavaParser : public ProjectExplorer::IOutputParser
{
    Q_OBJECT
public:
    JavaParser();
    virtual void stdOutput(const QString &line);
    virtual void stdError(const QString &line);

private:
    QRegExp m_javaRegExp;
};

} // namespace Internal
} // namespace Android

#endif // JAVAPARSER_H
