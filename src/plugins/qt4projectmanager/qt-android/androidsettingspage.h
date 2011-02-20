/*
I BogDan Vatra < bog_dan_ro@yahoo.com >, the copyright holder of this work,
hereby release it into the public domain. This applies worldwide.

In case this is not legally possible, I grant any entity the right to use
this work for any purpose, without any conditions, unless such conditions
are required by law.
*/

#ifndef ANDROIDSETTINGSPAGE_H
#define ANDROIDSETTINGSPAGE_H

#include <coreplugin/dialogs/ioptionspage.h>

namespace Qt4ProjectManager{
namespace Internal {

class AndroidSettingsWidget;

class AndroidSettingsPage : public Core::IOptionsPage
{
    Q_OBJECT
public:
    AndroidSettingsPage(QObject *parent);
    ~AndroidSettingsPage();

    virtual QString id() const;
    virtual QString displayName() const;
    virtual QString category() const;
    virtual QString displayCategory() const;
    virtual QIcon categoryIcon() const;
    virtual bool matches(const QString &searchKeyWord) const;
    virtual QWidget *createPage(QWidget *parent);
    virtual void apply();
    virtual void finish();

private:
    QString m_keywords;
    AndroidSettingsWidget *m_widget;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // ANDROIDSETTINGSPAGE_H
