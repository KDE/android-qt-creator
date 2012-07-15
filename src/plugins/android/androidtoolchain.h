/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#ifndef ANDROIDTOOLCHAIN_H
#define ANDROIDTOOLCHAIN_H

#include <projectexplorer/gcctoolchain.h>
#include <projectexplorer/toolchainconfigwidget.h>

namespace Android {

namespace Internal {

// --------------------------------------------------------------------------
// AndroidToolChain
// --------------------------------------------------------------------------

class AndroidToolChain : public ProjectExplorer::GccToolChain
{
public:
    ~AndroidToolChain();

    QString type() const;
    QString typeDisplayName() const;

    bool isValid() const;

    void addToEnvironment(Utils::Environment &env) const;

    bool operator ==(const ProjectExplorer::ToolChain &) const;

    ProjectExplorer::ToolChainConfigWidget *configurationWidget();

    QVariantMap toMap() const;
    bool fromMap(const QVariantMap &data);
    QList<Utils::FileName> suggestedMkspecList() const;
    virtual QString makeCommand() const;

    void setQtVersionId(int);
    int qtVersionId() const;

protected:
    virtual QList<ProjectExplorer::Abi> detectSupportedAbis() const;

private:
    explicit AndroidToolChain(bool);
    AndroidToolChain(const AndroidToolChain &);

    int m_qtVersionId;
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
    void handleQtVersionChanges(const QList<int> &added, const QList<int> &removed, const QList<int> &changed);
    QList<ProjectExplorer::ToolChain *> createToolChainList(const QList<int> &);
    void createDefaultProfiles();
};

} // namespace Internal
} // namespace Android

#endif // ANDROIDTOOLCHAIN_H
