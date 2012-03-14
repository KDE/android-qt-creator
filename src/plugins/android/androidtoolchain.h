/*
I BogDan Vatra < bog_dan_ro@yahoo.com >, the copyright holder of this work,
hereby release it into the public domain. This applies worldwide.

In case this is not legally possible, I grant any entity the right to use
this work for any purpose, without any conditions, unless such conditions
are required by law.
*/

#ifndef ANDROIDTOOLCHAIN_H
#define ANDROIDTOOLCHAIN_H

#include <projectexplorer/gcctoolchain.h>
#include <projectexplorer/toolchainconfigwidget.h>

namespace Android {

namespace Internal {

// --------------------------------------------------------------------------
// MaemoToolChain
// --------------------------------------------------------------------------

class AndroidToolChain : public ProjectExplorer::GccToolChain
{
public:
    ~AndroidToolChain();

    QString typeName() const;

    bool isValid() const;

    void addToEnvironment(Utils::Environment &env) const;
    QString sysroot() const;

    bool operator ==(const ProjectExplorer::ToolChain &) const;

    ProjectExplorer::ToolChainConfigWidget *configurationWidget();


    QVariantMap toMap() const;
    bool fromMap(const QVariantMap &data);
    virtual Utils::FileName mkspec() const;
    virtual QString makeCommand() const;

    void setQtVersionId(int);
    int qtVersionId() const;
    QString legacyId() const;
protected:
    virtual QList<ProjectExplorer::Abi> detectSupportedAbis() const;

private:
    explicit AndroidToolChain(bool);
    AndroidToolChain(const AndroidToolChain &);

    int m_qtVersionId;
    mutable QString m_sysroot;
    friend class AndroidToolChainFactory;
};


class AndroidToolChainConfigWidget : public ProjectExplorer::ToolChainConfigWidget
{
    Q_OBJECT

public:
    AndroidToolChainConfigWidget(AndroidToolChain *);

    void apply();
    void discard();
    bool isDirty() const;
};


class AndroidToolChainFactory : public ProjectExplorer::ToolChainFactory
{
    Q_OBJECT

public:
    AndroidToolChainFactory();

    QString displayName() const;
    QString id() const;

    QList<ProjectExplorer::ToolChain *> autoDetect();
    bool canRestore(const QVariantMap &data);
    ProjectExplorer::ToolChain *restore(const QVariantMap &data);

private slots:
    void handleQtVersionChanges(const QList<int> & added, const QList<int> &removed, const QList<int> &changed);
    QList<ProjectExplorer::ToolChain *> createToolChainList(const QList<int> &);
};

} // namespace Internal
} // namespace Android

#endif // ANDROIDTOOLCHAIN_H
