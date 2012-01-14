/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#include "fakevimplugin.h"

#include "fakevimhandler.h"
#include "ui_fakevimoptions.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/actionmanager/commandmappings.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/openeditorsmodel.h>
#include <coreplugin/filemanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/ifile.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/id.h>
#include <coreplugin/statusbarwidget.h>
#include <coreplugin/statusbarmanager.h>

#include <projectexplorer/projectexplorerconstants.h>

#include <texteditor/basetextdocumentlayout.h>
#include <texteditor/basetexteditor.h>
#include <texteditor/basetextmark.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/typingsettings.h>
#include <texteditor/tabsettings.h>
#include <texteditor/icodestylepreferences.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/indenter.h>
#include <texteditor/codeassist/basicproposalitem.h>
#include <texteditor/codeassist/basicproposalitemlistmodel.h>
#include <texteditor/codeassist/completionassistprovider.h>
#include <texteditor/codeassist/iassistprocessor.h>
#include <texteditor/codeassist/iassistinterface.h>
#include <texteditor/codeassist/genericproposal.h>

#include <find/findplugin.h>
#include <find/textfindconstants.h>

#include <utils/qtcassert.h>
#include <utils/savedaction.h>
#include <utils/treewidgetcolumnstretcher.h>
#include <utils/stylehelper.h>

#include <cppeditor/cppeditorconstants.h>

#include <cpptools/cpptoolsconstants.h>

#include <QtCore/QAbstractTableModel>
#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QtPlugin>
#include <QtCore/QObject>
#include <QtCore/QSettings>
#include <QtCore/QTextStream>

#include <QtGui/QDesktopServices>
#include <QtGui/QItemDelegate>
#include <QtGui/QMessageBox>
#include <QtGui/QPlainTextEdit>
#include <QtGui/QShortcut>
#include <QtGui/QTextBlock>
#include <QtGui/QTextCursor>
#include <QtGui/QTextEdit>
#include <QtGui/QTreeWidgetItem>

using namespace FakeVim::Internal;
using namespace TextEditor;
using namespace Core;

namespace FakeVim {
namespace Constants {

const char INSTALL_HANDLER[]                = "TextEditor.FakeVimHandler";
const char MINI_BUFFER[]                    = "TextEditor.FakeVimMiniBuffer";
const char INSTALL_KEY[]                    = "Alt+V,Alt+V";
const char SETTINGS_CATEGORY[]              = "D.FakeVim";
const char SETTINGS_CATEGORY_FAKEVIM_ICON[] = ":/core/images/category_fakevim.png";
const char SETTINGS_ID[]                    = "A.General";
const char SETTINGS_EX_CMDS_ID[]            = "B.ExCommands";
const char SETTINGS_USER_CMDS_ID[]          = "C.UserCommands";

} // namespace Constants
} // namespace FakeVim


namespace FakeVim {
namespace Internal {

class MiniBuffer : public QLabel
{
    Q_OBJECT

public:
    void setContents(const QString &contents, int cursorPos)
    {
        QString msg = contents;
        if (cursorPos != -1)
            msg = contents.left(cursorPos) + QChar(10073) + contents.mid(cursorPos);
        setText("  " + msg);
    }
};

class MiniBuffer1 : public QLineEdit
{
    Q_OBJECT

public:
    MiniBuffer1()
    {
        setFrame(false);
    }
    void showEvent(QShowEvent *ev)
    {
        QLineEdit::showEvent(ev);
        QColor color = Qt::black;
        QPalette pal = parentWidget()->palette();
        pal.setBrush(QPalette::All, QPalette::WindowText, color);
        pal.setBrush(QPalette::All, QPalette::ButtonText, color);
        pal.setBrush(QPalette::All, QPalette::Foreground, color);
        pal.setBrush(QPalette::All, QPalette::Background, color);
        //color.setAlpha(100);
        //pal.setBrush(QPalette::Disabled, QPalette::WindowText, color);
        //pal.setBrush(QPalette::Disabled, QPalette::ButtonText, color);
        //pal.setBrush(QPalette::Disabled, QPalette::Foreground, color);
        setPalette(pal);
    }
    void setContents(const QString &contents, int cursorPos)
    {
        setText(contents);
        setCursorPosition(cursorPos);
    }
};

///////////////////////////////////////////////////////////////////////
//
// FakeVimOptionPage
//
///////////////////////////////////////////////////////////////////////

typedef QMap<QString, QRegExp> ExCommandMap;
typedef QMap<int, QString> UserCommandMap;
typedef QLatin1String _;

class FakeVimOptionPage : public Core::IOptionsPage
{
    Q_OBJECT

public:
    FakeVimOptionPage() {}

    // IOptionsPage
    QString id() const { return _(Constants::SETTINGS_ID); }
    QString displayName() const { return tr("General"); }
    QString category() const { return _(Constants::SETTINGS_CATEGORY); }
    QString displayCategory() const { return tr("FakeVim"); }
    QIcon categoryIcon() const
        { return QIcon(_(Constants::SETTINGS_CATEGORY_FAKEVIM_ICON)); }

    QWidget *createPage(QWidget *parent);
    void apply() { m_group.apply(ICore::instance()->settings()); }
    void finish() { m_group.finish(); }
    virtual bool matches(const QString &) const;

private slots:
    void copyTextEditorSettings();
    void setQtStyle();
    void setPlainStyle();

private:
    friend class DebuggerPlugin;
    Ui::FakeVimOptionPage m_ui;
    QString m_searchKeywords;
    Utils::SavedActionSet m_group;
};

QWidget *FakeVimOptionPage::createPage(QWidget *parent)
{
    QWidget *w = new QWidget(parent);
    m_ui.setupUi(w);

    m_group.clear();
    m_group.insert(theFakeVimSetting(ConfigUseFakeVim),
        m_ui.checkBoxUseFakeVim);
    m_group.insert(theFakeVimSetting(ConfigReadVimRc),
        m_ui.checkBoxReadVimRc);

    m_group.insert(theFakeVimSetting(ConfigExpandTab),
        m_ui.checkBoxExpandTab);
    m_group.insert(theFakeVimSetting(ConfigHlSearch),
        m_ui.checkBoxHlSearch);
    m_group.insert(theFakeVimSetting(ConfigShiftWidth),
        m_ui.spinBoxShiftWidth);
    m_group.insert(theFakeVimSetting(ConfigShowMarks),
        m_ui.checkBoxShowMarks);

    m_group.insert(theFakeVimSetting(ConfigSmartTab),
        m_ui.checkBoxSmartTab);
    m_group.insert(theFakeVimSetting(ConfigStartOfLine),
        m_ui.checkBoxStartOfLine);
    m_group.insert(theFakeVimSetting(ConfigTabStop),
        m_ui.spinBoxTabStop);
    m_group.insert(theFakeVimSetting(ConfigBackspace),
        m_ui.lineEditBackspace);
    m_group.insert(theFakeVimSetting(ConfigIsKeyword),
        m_ui.lineEditIsKeyword);

    m_group.insert(theFakeVimSetting(ConfigPassControlKey),
        m_ui.checkBoxPassControlKey);
    m_group.insert(theFakeVimSetting(ConfigAutoIndent),
        m_ui.checkBoxAutoIndent);
    m_group.insert(theFakeVimSetting(ConfigSmartIndent),
        m_ui.checkBoxSmartIndent);
    m_group.insert(theFakeVimSetting(ConfigIncSearch),
        m_ui.checkBoxIncSearch);
    m_group.insert(theFakeVimSetting(ConfigUseCoreSearch),
        m_ui.checkBoxUseCoreSearch);

    connect(m_ui.pushButtonCopyTextEditorSettings, SIGNAL(clicked()),
        SLOT(copyTextEditorSettings()));
    connect(m_ui.pushButtonSetQtStyle, SIGNAL(clicked()),
        SLOT(setQtStyle()));
    connect(m_ui.pushButtonSetPlainStyle, SIGNAL(clicked()),
        SLOT(setPlainStyle()));

    if (m_searchKeywords.isEmpty()) {
        QLatin1Char sep(' ');
        QTextStream(&m_searchKeywords)
                << sep << m_ui.checkBoxUseFakeVim->text()
                << sep << m_ui.checkBoxReadVimRc->text()
                << sep << m_ui.checkBoxAutoIndent->text()
                << sep << m_ui.checkBoxSmartIndent->text()
                << sep << m_ui.checkBoxExpandTab->text()
                << sep << m_ui.checkBoxSmartTab->text()
                << sep << m_ui.checkBoxHlSearch->text()
                << sep << m_ui.checkBoxIncSearch->text()
                << sep << m_ui.checkBoxStartOfLine->text()
                << sep << m_ui.checkBoxUseCoreSearch->text()
                << sep << m_ui.checkBoxShowMarks->text()
                << sep << m_ui.checkBoxPassControlKey->text()
                << sep << m_ui.labelShiftWidth->text()
                << sep << m_ui.labelTabulator->text()
                << sep << m_ui.labelBackspace->text()
                << sep << m_ui.labelIsKeyword->text();
        m_searchKeywords.remove(QLatin1Char('&'));
    }
    return w;
}

void FakeVimOptionPage::copyTextEditorSettings()
{
    TabSettings ts = TextEditorSettings::instance()->codeStyle()->tabSettings();
    TypingSettings tps = TextEditorSettings::instance()->typingSettings();
    m_ui.checkBoxExpandTab->setChecked(ts.m_tabPolicy != TabSettings::TabsOnlyTabPolicy);
    m_ui.spinBoxTabStop->setValue(ts.m_tabSize);
    m_ui.spinBoxShiftWidth->setValue(ts.m_indentSize);
    m_ui.checkBoxSmartTab->setChecked(
        tps.m_smartBackspaceBehavior == TypingSettings::BackspaceFollowsPreviousIndents);
    m_ui.checkBoxAutoIndent->setChecked(true);
    m_ui.checkBoxSmartIndent->setChecked(tps.m_autoIndent);
    m_ui.checkBoxIncSearch->setChecked(true);
}

void FakeVimOptionPage::setQtStyle()
{
    m_ui.checkBoxExpandTab->setChecked(true);
    m_ui.spinBoxTabStop->setValue(4);
    m_ui.spinBoxShiftWidth->setValue(4);
    m_ui.checkBoxSmartTab->setChecked(true);
    m_ui.checkBoxAutoIndent->setChecked(true);
    m_ui.checkBoxSmartIndent->setChecked(true);
    m_ui.checkBoxIncSearch->setChecked(true);
    m_ui.lineEditBackspace->setText(_("indent,eol,start"));
}

void FakeVimOptionPage::setPlainStyle()
{
    m_ui.checkBoxExpandTab->setChecked(false);
    m_ui.spinBoxTabStop->setValue(8);
    m_ui.spinBoxShiftWidth->setValue(8);
    m_ui.checkBoxSmartTab->setChecked(false);
    m_ui.checkBoxAutoIndent->setChecked(false);
    m_ui.checkBoxSmartIndent->setChecked(false);
    m_ui.checkBoxIncSearch->setChecked(false);
    m_ui.lineEditBackspace->setText(QString());
}

bool FakeVimOptionPage::matches(const QString &s) const
{
    return m_searchKeywords.contains(s, Qt::CaseInsensitive);
}

//const char *FAKEVIM_CONTEXT = "FakeVim";

///////////////////////////////////////////////////////////////////////
//
// FakeVimExCommandsPage
//
///////////////////////////////////////////////////////////////////////

enum { CommandRole = Qt::UserRole };

class FakeVimExCommandsPage : public Core::CommandMappings
{
    Q_OBJECT

public:
    FakeVimExCommandsPage(FakeVimPluginPrivate *q) : m_q(q) {}
    ~FakeVimExCommandsPage() {}

    // IOptionsPage
    QString id() const { return _(Constants::SETTINGS_EX_CMDS_ID); }
    QString displayName() const { return tr("Ex Command Mapping"); }
    QString category() const { return _(Constants::SETTINGS_CATEGORY); }
    QString displayCategory() const { return tr("FakeVim"); }
    QIcon categoryIcon() const { return QIcon(); } // TODO: Icon for FakeVim

    QWidget *createPage(QWidget *parent);
    void initialize();
    ExCommandMap &exCommandMap();
    ExCommandMap &defaultExCommandMap();

public slots:
    void commandChanged(QTreeWidgetItem *current);
    void targetIdentifierChanged();
    void resetTargetIdentifier();
    void removeTargetIdentifier();
    void defaultAction();

private:
    FakeVimPluginPrivate *m_q;
};

QWidget *FakeVimExCommandsPage::createPage(QWidget *parent)
{
    QWidget *w = CommandMappings::createPage(parent);
    setPageTitle(tr("Ex Command Mapping"));
    setTargetHeader(tr("Ex Trigger Expression"));
    setTargetLabelText(tr("Regular expression:"));
    setTargetEditTitle(tr("Ex Command"));
    setImportExportEnabled(false);
    return w;
}

void FakeVimExCommandsPage::initialize()
{
    ActionManager *am = ICore::instance()->actionManager();
    QTC_ASSERT(am, return);

    QMap<QString, QTreeWidgetItem *> sections;

    foreach (Command *c, am->commands()) {
        if (c->action() && c->action()->isSeparator())
            continue;

        QTreeWidgetItem *item = new QTreeWidgetItem;
        const QString name = c->id().toString();
        const int pos = name.indexOf(QLatin1Char('.'));
        const QString section = name.left(pos);
        const QString subId = name.mid(pos + 1);
        item->setData(0, CommandRole, name);

        if (!sections.contains(section)) {
            QTreeWidgetItem *categoryItem =
                new QTreeWidgetItem(commandList(), QStringList() << section);
            QFont f = categoryItem->font(0);
            f.setBold(true);
            categoryItem->setFont(0, f);
            sections.insert(section, categoryItem);
            commandList()->expandItem(categoryItem);
        }
        sections[section]->addChild(item);

        item->setText(0, subId);
        item->setText(1, c->description());

        QString regex;
        if (exCommandMap().contains(name))
            regex = exCommandMap()[name].pattern();
        item->setText(2, regex);

        if (regex != defaultExCommandMap()[name].pattern())
            setModified(item, true);
    }

    commandChanged(0);
}

void FakeVimExCommandsPage::commandChanged(QTreeWidgetItem *current)
{
    CommandMappings::commandChanged(current);
    if (current)
        targetEdit()->setText(current->text(2));
}

void FakeVimExCommandsPage::targetIdentifierChanged()
{
    QTreeWidgetItem *current = commandList()->currentItem();
    if (!current)
        return;

    const QString name =  current->data(0, CommandRole).toString();
    const QString regex = targetEdit()->text();

    if (current->data(0, Qt::UserRole).isValid()) {
        current->setText(2, regex);
        exCommandMap()[name] = QRegExp(regex);
    }

    setModified(current, regex != defaultExCommandMap()[name].pattern());
}

void FakeVimExCommandsPage::resetTargetIdentifier()
{
    QTreeWidgetItem *current = commandList()->currentItem();
    if (!current)
        return;
    const QString name = current->data(0, CommandRole).toString();
    QString regex;
    if (defaultExCommandMap().contains(name))
        regex = defaultExCommandMap()[name].pattern();
    targetEdit()->setText(regex);
}

void FakeVimExCommandsPage::removeTargetIdentifier()
{
    targetEdit()->clear();
}

void FakeVimExCommandsPage::defaultAction()
{
    int n = commandList()->topLevelItemCount();
    for (int i = 0; i != n; ++i) {
        QTreeWidgetItem *section = commandList()->topLevelItem(i);
        int m = section->childCount();
        for (int j = 0; j != m; ++j) {
            QTreeWidgetItem *item = section->child(j);
            const QString name = item->data(0, CommandRole).toString();
            QString regex;
            if (defaultExCommandMap().contains(name))
                regex = defaultExCommandMap()[name].pattern();
            setModified(item, false);
            item->setText(2, regex);
            if (item == commandList()->currentItem())
                commandChanged(item);
        }
    }
}

///////////////////////////////////////////////////////////////////////
//
// FakeVimUserCommandsPage
//
///////////////////////////////////////////////////////////////////////

class FakeVimUserCommandsModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    FakeVimUserCommandsModel(FakeVimPluginPrivate *q) : m_q(q) {}
    ~FakeVimUserCommandsModel() {}

    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    bool setData(const QModelIndex &index, const QVariant &data, int role);
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;

private:
    FakeVimPluginPrivate *m_q;
};

int FakeVimUserCommandsModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 9;
}

int FakeVimUserCommandsModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 2;
}


QVariant FakeVimUserCommandsModel::headerData(int section,
    Qt::Orientation orient, int role) const
{
    if (orient == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
            case 0: return tr("Action");
            case 1: return tr("Command");
        };
    }
    return QVariant();
}

Qt::ItemFlags FakeVimUserCommandsModel::flags(const QModelIndex &index) const
{
    if (index.column() == 1)
        return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
    return QAbstractTableModel::flags(index);
}

class FakeVimUserCommandsDelegate : public QItemDelegate
{
public:
    explicit FakeVimUserCommandsDelegate(QObject *parent)
        : QItemDelegate(parent)
    {}

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &,
        const QModelIndex &) const
    {
        QLineEdit *lineEdit = new QLineEdit(parent);
        lineEdit->setFrame(false);
        return lineEdit;
    }

    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const
    {
        QLineEdit *lineEdit = qobject_cast<QLineEdit *>(editor);
        QTC_ASSERT(lineEdit, return);
        model->setData(index, lineEdit->text(), Qt::EditRole);
    }
};

class FakeVimUserCommandsPage : public Core::IOptionsPage
{
    Q_OBJECT

public:
    FakeVimUserCommandsPage(FakeVimPluginPrivate *q) : m_q(q) {}
    ~FakeVimUserCommandsPage() {}

    // IOptionsPage
    QString id() const { return _(Constants::SETTINGS_USER_CMDS_ID); }
    QString displayName() const { return tr("User Command Mapping"); }
    QString category() const { return _(Constants::SETTINGS_CATEGORY); }
    QString displayCategory() const { return tr("FakeVim"); }
    QIcon categoryIcon() const { return QIcon(); }
    void apply();
    void finish() {}

    QWidget *createPage(QWidget *parent);
    void initialize() {}
    UserCommandMap &userCommandMap();
    UserCommandMap &defaultUserCommandMap();

private:
    FakeVimPluginPrivate *m_q;
};

QWidget *FakeVimUserCommandsPage::createPage(QWidget *parent)
{
    QGroupBox *box = new QGroupBox(parent);

    FakeVimUserCommandsModel *model = new FakeVimUserCommandsModel(m_q);
    QTreeView *widget = new QTreeView;
    widget->setModel(model);
    widget->resizeColumnToContents(0);

    FakeVimUserCommandsDelegate *delegate = new FakeVimUserCommandsDelegate(widget);
    widget->setItemDelegateForColumn(1, delegate);

    QGridLayout *layout = new QGridLayout(box);
    layout->addWidget(widget, 0, 0);
    box->setLayout(layout);

    return box;
}

void FakeVimUserCommandsPage::apply()
{
    //m_q->writeSettings();
}


///////////////////////////////////////////////////////////////////////
//
// WordCompletion
//
///////////////////////////////////////////////////////////////////////

class FakeVimCompletionAssistProvider : public TextEditor::CompletionAssistProvider
{
public:
    bool supportsEditor(const Core::Id &) const
    {
        return false;
    }

    TextEditor::IAssistProcessor *createProcessor() const;

    void setActive(const QString &needle, bool forward, FakeVimHandler *handler)
    {
        Q_UNUSED(forward);
        m_handler = handler;
        if (!m_handler)
            return;

        BaseTextEditorWidget *editor = qobject_cast<BaseTextEditorWidget *>(handler->widget());
        if (!editor)
            return;

        //qDebug() << "ACTIVATE: " << needle << forward;
        m_needle = needle;
        editor->invokeAssist(Completion, this);
    }

    void setInactive()
    {
        m_needle.clear();
        m_handler = 0;
    }

    const QString &needle() const
    {
        return m_needle;
    }

    void appendNeedle(const QChar &c)
    {
        m_needle.append(c);
    }

    FakeVimHandler *handler() const
    {
        return m_handler;
    }

private:
    FakeVimHandler *m_handler;
    QString m_needle;
};

class FakeVimAssistProposalItem : public BasicProposalItem
{
public:
    FakeVimAssistProposalItem(const FakeVimCompletionAssistProvider *provider)
        : m_provider(const_cast<FakeVimCompletionAssistProvider *>(provider))
    {}

    virtual bool implicitlyApplies() const
    {
        return false;
    }

    virtual bool prematurelyApplies(const QChar &c) const
    {
        m_provider->appendNeedle(c);
        return text() == m_provider->needle();
    }

    virtual void applyContextualContent(BaseTextEditor *, int) const
    {
        QTC_ASSERT(m_provider->handler(), return);
        m_provider->handler()->handleReplay(text().mid(m_provider->needle().size()));
        const_cast<FakeVimCompletionAssistProvider *>(m_provider)->setInactive();
    }

private:
    FakeVimCompletionAssistProvider *m_provider;
};


class FakeVimAssistProposalModel : public BasicProposalItemListModel
{
public:
    FakeVimAssistProposalModel(const QList<BasicProposalItem *> &items)
        : BasicProposalItemListModel(items)
    {}

    virtual bool supportsPrefixExpansion() const
    {
        return false;
    }
};

class FakeVimCompletionAssistProcessor : public IAssistProcessor
{
public:
    FakeVimCompletionAssistProcessor(const TextEditor::IAssistProvider *provider)
        : m_provider(static_cast<const FakeVimCompletionAssistProvider *>(provider))
    {}

    virtual TextEditor::IAssistProposal *perform(const IAssistInterface *interface)
    {
        const QString &needle = m_provider->needle();

        const int basePosition = interface->position() - needle.size();

        QTextCursor tc(interface->document());
        tc.setPosition(interface->position());
        tc.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);

        QList<BasicProposalItem *> items;
        QSet<QString> seen;
        QTextDocument::FindFlags flags = QTextDocument::FindCaseSensitively;
        while (1) {
            tc = tc.document()->find(needle, tc.position(), flags);
            if (tc.isNull())
                break;
            QTextCursor sel = tc;
            sel.select(QTextCursor::WordUnderCursor);
            QString found = sel.selectedText();
            // Only add "real" completions.
            if (found.startsWith(needle)
                    && !seen.contains(found)
                    && sel.anchor() != basePosition) {
                seen.insert(found);
                BasicProposalItem *item = new FakeVimAssistProposalItem(m_provider);
                item->setText(found);
                items.append(item);
            }
            tc.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor);
        }
        //qDebug() << "COMPLETIONS" << completions->size();

        delete interface;
        return new GenericProposal(basePosition, new FakeVimAssistProposalModel(items));
    }

private:
    const FakeVimCompletionAssistProvider *m_provider;
};

IAssistProcessor *FakeVimCompletionAssistProvider::createProcessor() const
{
    return new FakeVimCompletionAssistProcessor(this);
}


///////////////////////////////////////////////////////////////////////
//
// FakeVimPluginPrivate
//
///////////////////////////////////////////////////////////////////////

class FakeVimPluginPrivate : public QObject
{
    Q_OBJECT

public:
    FakeVimPluginPrivate(FakeVimPlugin *);
    ~FakeVimPluginPrivate();
    friend class FakeVimPlugin;
    friend class FakeVimExCommandsPage;
    friend class FakeVimUserCommandsPage;
    friend class FakeVimUserCommandsModel;

    bool initialize();
    void aboutToShutdown();

private slots:
    void onCoreAboutToClose();
    void editorOpened(Core::IEditor *);
    void editorAboutToClose(Core::IEditor *);

    void setUseFakeVim(const QVariant &value);
    void setUseFakeVimInternal(bool on);
    void quitFakeVim();
    void triggerCompletions();
    void triggerSimpleCompletions(const QString &needle, bool forward);
    void windowCommand(int key);
    void find(bool reverse);
    void findNext(bool reverse);
    void showSettingsDialog();
    void maybeReadVimRc();
    void setBlockSelection(bool);
    void hasBlockSelection(bool*);

    void showCommandBuffer(const QString &contents, int cursorPos);
    void showExtraInformation(const QString &msg);
    void changeSelection(const QList<QTextEdit::ExtraSelection> &selections);
    void moveToMatchingParenthesis(bool *moved, bool *forward, QTextCursor *cursor);
    void checkForElectricCharacter(bool *result, QChar c);
    void indentRegion(int beginLine, int endLine, QChar typedChar);
    void handleExCommand(bool *handled, const ExCommand &cmd);

    void writeSettings();
    void readSettings();

    void handleDelayedQuitAll(bool forced);
    void handleDelayedQuit(bool forced, Core::IEditor *editor);
    void userActionTriggered();

    void switchToFile(int n);
    int currentFile() const;

signals:
    void delayedQuitRequested(bool forced, Core::IEditor *editor);
    void delayedQuitAllRequested(bool forced);

private:
    FakeVimPlugin *q;
    FakeVimOptionPage *m_fakeVimOptionsPage;
    FakeVimExCommandsPage *m_fakeVimExCommandsPage;
    FakeVimUserCommandsPage *m_fakeVimUserCommandsPage;
    QHash<Core::IEditor *, FakeVimHandler *> m_editorToHandler;

    void triggerAction(const Core::Id &id);
    void setActionChecked(const Core::Id &id, bool check);

    typedef int (*DistFunction)(const QRect &cursor, const QRect &other);
    void moveSomewhere(DistFunction f);

    ExCommandMap &exCommandMap() { return m_exCommandMap; }
    ExCommandMap &defaultExCommandMap() { return m_defaultExCommandMap; }
    ExCommandMap m_exCommandMap;
    ExCommandMap m_defaultExCommandMap;

    UserCommandMap &userCommandMap() { return m_userCommandMap; }
    UserCommandMap &defaultUserCommandMap() { return m_defaultUserCommandMap; }
    UserCommandMap m_userCommandMap;
    UserCommandMap m_defaultUserCommandMap;

    Core::StatusBarWidget *m_statusBar;
    // @TODO: Delete
    //WordCompletion *m_wordCompletion;
    FakeVimCompletionAssistProvider *m_wordProvider;
};

QVariant FakeVimUserCommandsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        switch (index.column()) {
        case 0: // Action
            return tr("User command #%1").arg(index.row() + 1);
        case 1: // Command
            return m_q->userCommandMap().value(index.row() + 1);
        }
    }

    return QVariant();
}

bool FakeVimUserCommandsModel::setData(const QModelIndex &index,
    const QVariant &data, int role)
{
    if (role == Qt::DisplayRole || role == Qt::EditRole)
        if (index.column() == 1)
            m_q->userCommandMap()[index.row() + 1] = data.toString();
    return true;
}

FakeVimPluginPrivate::FakeVimPluginPrivate(FakeVimPlugin *plugin)
{
    q = plugin;
    m_fakeVimOptionsPage = 0;
    m_fakeVimExCommandsPage = 0;
    m_fakeVimUserCommandsPage = 0;
    defaultExCommandMap()[CppTools::Constants::SWITCH_HEADER_SOURCE] =
        QRegExp("^A$");
    defaultExCommandMap()["Coreplugin.OutputPane.previtem"] =
        QRegExp("^(cN(ext)?|cp(revious)?)!?( (.*))?$");
    defaultExCommandMap()["Coreplugin.OutputPane.nextitem"] =
        QRegExp("^cn(ext)?!?( (.*))?$");
    defaultExCommandMap()[CppEditor::Constants::JUMP_TO_DEFINITION] =
        QRegExp("^tag?$");
    defaultExCommandMap()[Core::Constants::GO_BACK] =
        QRegExp("^pop?$");
    defaultExCommandMap()[_("QtCreator.Locate")] =
        QRegExp("^e$");

    for (int i = 1; i < 10; ++i) {
        QString cmd = QString::fromLatin1(":echo User command %1 executed.<CR>");
        defaultUserCommandMap().insert(i, cmd.arg(i));
    }

    m_statusBar = 0;
}

FakeVimPluginPrivate::~FakeVimPluginPrivate()
{
    q->removeObject(m_fakeVimOptionsPage);
    delete m_fakeVimOptionsPage;
    m_fakeVimOptionsPage = 0;
    delete theFakeVimSettings();

    q->removeObject(m_fakeVimExCommandsPage);
    delete m_fakeVimExCommandsPage;
    m_fakeVimExCommandsPage = 0;

    q->removeObject(m_fakeVimUserCommandsPage);
    delete m_fakeVimUserCommandsPage;
    m_fakeVimUserCommandsPage = 0;
}

void FakeVimPluginPrivate::onCoreAboutToClose()
{
    // Don't attach to editors anymore.
    disconnect(ICore::instance()->editorManager(), SIGNAL(editorOpened(Core::IEditor*)),
        this, SLOT(editorOpened(Core::IEditor*)));
}

void FakeVimPluginPrivate::aboutToShutdown()
{
}

bool FakeVimPluginPrivate::initialize()
{
    Core::ICore *core = Core::ICore::instance();
    EditorManager *editorManager = core->editorManager();
    ActionManager *actionManager = core->actionManager();
    QTC_ASSERT(actionManager, return false);

    //m_wordCompletion = new WordCompletion;
    //q->addAutoReleasedObject(m_wordCompletion);
    m_wordProvider = new FakeVimCompletionAssistProvider;

/*
    // Set completion settings and keep them up to date.
    TextEditorSettings *textEditorSettings = TextEditorSettings::instance();
    completion->setCompletionSettings(textEditorSettings->completionSettings());
    connect(textEditorSettings,
        SIGNAL(completionSettingsChanged(TextEditor::CompletionSettings)),
        completion,
        SLOT(setCompletionSettings(TextEditor::CompletionSettings)));
*/

    Context globalcontext(Core::Constants::C_GLOBAL);

    m_fakeVimOptionsPage = new FakeVimOptionPage;
    q->addObject(m_fakeVimOptionsPage);

    m_fakeVimExCommandsPage = new FakeVimExCommandsPage(this);
    q->addObject(m_fakeVimExCommandsPage);

    m_fakeVimUserCommandsPage = new FakeVimUserCommandsPage(this);
    q->addObject(m_fakeVimUserCommandsPage);

    readSettings();

    Core::Command *cmd = 0;
    cmd = actionManager->registerAction(theFakeVimSetting(ConfigUseFakeVim),
        Constants::INSTALL_HANDLER, globalcontext, true);
    cmd->setDefaultKeySequence(QKeySequence(Constants::INSTALL_KEY));

    ActionContainer *advancedMenu =
        actionManager->actionContainer(Core::Constants::M_EDIT_ADVANCED);
    advancedMenu->addAction(cmd, Core::Constants::G_EDIT_EDITOR);

    for (int i = 1; i < 10; ++i) {
        QAction *act = new QAction(this);
        act->setText(tr("Execute User Action #%1").arg(i));
        act->setData(i);
        QString id = QString("FakeVim.UserAction%1").arg(i);
        QString keys = QString("Alt+V,%1").arg(i);
        cmd = actionManager->registerAction(act, Core::Id(id), globalcontext);
        cmd->setDefaultKeySequence(QKeySequence(keys));
        connect(act, SIGNAL(triggered()), SLOT(userActionTriggered()));
    }

    connect(core, SIGNAL(coreAboutToClose()), this, SLOT(onCoreAboutToClose()));

    // EditorManager
    connect(editorManager, SIGNAL(editorAboutToClose(Core::IEditor*)),
        this, SLOT(editorAboutToClose(Core::IEditor*)));
    connect(editorManager, SIGNAL(editorOpened(Core::IEditor*)),
        this, SLOT(editorOpened(Core::IEditor*)));

    connect(theFakeVimSetting(ConfigUseFakeVim), SIGNAL(valueChanged(QVariant)),
        this, SLOT(setUseFakeVim(QVariant)));
    connect(theFakeVimSetting(ConfigReadVimRc), SIGNAL(valueChanged(QVariant)),
        this, SLOT(maybeReadVimRc()));

    // Delayed operations.
    connect(this, SIGNAL(delayedQuitRequested(bool,Core::IEditor*)),
        this, SLOT(handleDelayedQuit(bool,Core::IEditor*)), Qt::QueuedConnection);
    connect(this, SIGNAL(delayedQuitAllRequested(bool)),
        this, SLOT(handleDelayedQuitAll(bool)), Qt::QueuedConnection);
    maybeReadVimRc();
    //    << "MODE: " << theFakeVimSetting(ConfigUseFakeVim)->value();

    return true;
}

void FakeVimPluginPrivate::userActionTriggered()
{
    QAction *act = qobject_cast<QAction *>(sender());
    if (!act)
        return;
    const int key = act->data().toInt();
    if (!key)
        return;
    QString cmd = userCommandMap().value(key);
    Core::ICore *core = Core::ICore::instance();
    EditorManager *editorManager = core->editorManager();
    IEditor *editor = editorManager->currentEditor();
    FakeVimHandler *handler = m_editorToHandler[editor];
    if (handler)
        handler->handleInput(cmd);
}

static const char exCommandMapGroup[] = "FakeVimExCommand";
static const char userCommandMapGroup[] = "FakeVimUserCommand";
static const char reKey[] = "RegEx";
static const char cmdKey[] = "Cmd";
static const char idKey[] = "Command";

void FakeVimPluginPrivate::writeSettings()
{
    QSettings *settings = ICore::instance()->settings();

    theFakeVimSettings()->writeSettings(settings);

    { // block
    settings->beginWriteArray(_(exCommandMapGroup));
    int count = 0;
    typedef ExCommandMap::const_iterator Iterator;
    const Iterator end = exCommandMap().constEnd();
    for (Iterator it = exCommandMap().constBegin(); it != end; ++it) {
        const QString id = it.key();
        const QRegExp re = it.value();

        if ((defaultExCommandMap().contains(id) && defaultExCommandMap()[id] != re)
            || (!defaultExCommandMap().contains(id) && !re.pattern().isEmpty())) {
            settings->setArrayIndex(count);
            settings->setValue(_(idKey), id);
            settings->setValue(_(reKey), re.pattern());
            ++count;
        }
    }
    settings->endArray();
    } // block

    { // block
    settings->beginWriteArray(_(userCommandMapGroup));
    int count = 0;
    typedef UserCommandMap::const_iterator Iterator;
    const Iterator end = userCommandMap().constEnd();
    for (Iterator it = userCommandMap().constBegin(); it != end; ++it) {
        const int key = it.key();
        const QString cmd = it.value();

        if ((defaultUserCommandMap().contains(key)
                && defaultUserCommandMap()[key] != cmd)
            || (!defaultUserCommandMap().contains(key) && !cmd.isEmpty())) {
            settings->setArrayIndex(count);
            settings->setValue(_(idKey), key);
            settings->setValue(_(cmdKey), cmd);
            ++count;
        }
    }
    settings->endArray();
    } // block
}

void FakeVimPluginPrivate::readSettings()
{
    QSettings *settings = ICore::instance()->settings();

    theFakeVimSettings()->readSettings(settings);

    exCommandMap() = defaultExCommandMap();
    int size = settings->beginReadArray(_(exCommandMapGroup));
    for (int i = 0; i < size; ++i) {
        settings->setArrayIndex(i);
        const QString id = settings->value(_(idKey)).toString();
        const QString re = settings->value(_(reKey)).toString();
        exCommandMap()[id] = QRegExp(re);
    }
    settings->endArray();

    userCommandMap() = defaultUserCommandMap();
    size = settings->beginReadArray(_(userCommandMapGroup));
    for (int i = 0; i < size; ++i) {
        settings->setArrayIndex(i);
        const int id = settings->value(_(idKey)).toInt();
        const QString cmd = settings->value(_(cmdKey)).toString();
        userCommandMap()[id] = cmd;
    }
    settings->endArray();
}

void FakeVimPluginPrivate::maybeReadVimRc()
{
    //qDebug() << theFakeVimSetting(ConfigReadVimRc)
    //    << theFakeVimSetting(ConfigReadVimRc)->value();
    //qDebug() << theFakeVimSetting(ConfigShiftWidth)->value();
    if (!theFakeVimSetting(ConfigReadVimRc)->value().toBool())
        return;
    QString fileName =
        QDesktopServices::storageLocation(QDesktopServices::HomeLocation)
            + "/.vimrc";
    //qDebug() << "READING VIMRC: " << fileName;
    // Read it into a temporary handler for effects modifying global state.
    QPlainTextEdit editor;
    FakeVimHandler handler(&editor);
    handler.handleCommand("source " + fileName);
    //writeSettings();
    //qDebug() << theFakeVimSetting(ConfigShiftWidth)->value();
}

void FakeVimPluginPrivate::showSettingsDialog()
{
    ICore::instance()->showOptionsDialog(
        _(Constants::SETTINGS_CATEGORY),
        _(Constants::SETTINGS_ID));
}

void FakeVimPluginPrivate::triggerAction(const Id &id)
{
    Core::ActionManager *am = ICore::instance()->actionManager();
    QTC_ASSERT(am, return);
    Core::Command *cmd = am->command(id);
    QTC_ASSERT(cmd, qDebug() << "UNKNOWN CODE: " << id.name(); return);
    QAction *action = cmd->action();
    QTC_ASSERT(action, return);
    action->trigger();
}

void FakeVimPluginPrivate::setActionChecked(const Id &id, bool check)
{
    Core::ActionManager *am = ICore::instance()->actionManager();
    QTC_ASSERT(am, return);
    Core::Command *cmd = am->command(id);
    QTC_ASSERT(cmd, return);
    QAction *action = cmd->action();
    QTC_ASSERT(action, return);
    QTC_ASSERT(action->isCheckable(), return);
    action->setChecked(!check); // trigger negates the action's state
    action->trigger();
}

static int moveRightWeight(const QRect &cursor, const QRect &other)
{
    int dx = other.left() - cursor.right();
    if (dx < 0)
        return -1;
    int w = 10000 * dx;
    int dy1 = cursor.top() - other.bottom();
    int dy2 = cursor.bottom() - other.top();
    w += dy1 * (dy1 > 0);
    w += dy2 * (dy2 > 0);
    qDebug() << "      DX: " << dx << dy1 << dy2 << w;
    return w;
}

static int moveLeftWeight(const QRect &cursor, const QRect &other)
{
    int dx = other.right() - cursor.left();
    if (dx < 0)
        return -1;
    int w = 10000 * dx;
    int dy1 = cursor.top() - other.bottom();
    int dy2 = cursor.bottom() - other.top();
    w += dy1 * (dy1 > 0);
    w += dy2 * (dy2 > 0);
    return w;
}

static int moveUpWeight(const QRect &cursor, const QRect &other)
{
    int dy = other.bottom() - cursor.top();
    if (dy < 0)
        return -1;
    int w = 10000 * dy;
    int dx1 = cursor.left() - other.right();
    int dx2 = cursor.right() - other.left();
    w += dx1 * (dx1 > 0);
    w += dx2 * (dx2 > 0);
    return w;
}

static int moveDownWeight(const QRect &cursor, const QRect &other)
{
    int dy = other.top() - cursor.bottom();
    if (dy < 0)
        return -1;
    int w = 10000 * dy;
    int dx1 = cursor.left() - other.right();
    int dx2 = cursor.right() - other.left();
    w += dx1 * (dx1 > 0);
    w += dx2 * (dx2 > 0);
    return w;
}

void FakeVimPluginPrivate::windowCommand(int key)
{
#    define control(n) (256 + n)
    switch (key) {
        case 'c': case 'C': case control('c'):
            triggerAction(Core::Constants::CLOSE);
            break;
        case 'n': case 'N': case control('n'):
            triggerAction(Core::Constants::GOTONEXT);
            break;
        case 'o': case 'O': case control('o'):
            //triggerAction(Core::Constants::REMOVE_ALL_SPLITS);
            triggerAction(Core::Constants::REMOVE_CURRENT_SPLIT);
            break;
        case 'p': case 'P': case control('p'):
            triggerAction(Core::Constants::GOTOPREV);
            break;
        case 's': case 'S': case control('s'):
            triggerAction(Core::Constants::SPLIT);
            break;
        case 'w': case 'W': case control('w'):
            triggerAction(Core::Constants::GOTO_OTHER_SPLIT);
            break;
        case Qt::Key_Right:
            moveSomewhere(&moveRightWeight);
            break;
        case Qt::Key_Left:
            moveSomewhere(&moveLeftWeight);
            break;
        case Qt::Key_Up:
            moveSomewhere(&moveUpWeight);
            break;
        case Qt::Key_Down:
            moveSomewhere(&moveDownWeight);
            break;
        default:
            qDebug() << "UNKNOWN WINDOWS COMMAND: " << key;
            break;
    }
#    undef control
}

void FakeVimPluginPrivate::moveSomewhere(DistFunction f)
{
    EditorManager *editorManager = ICore::instance()->editorManager();
    IEditor *editor = editorManager->currentEditor();
    QWidget *w = editor->widget();
    QPlainTextEdit *pe =
        qobject_cast<QPlainTextEdit *>(editor->widget());
    QTC_ASSERT(pe, return);
    QRect rc = pe->cursorRect();
    QRect cursorRect(w->mapToGlobal(rc.topLeft()),
            w->mapToGlobal(rc.bottomRight()));
    //qDebug() << "\nCURSOR: " << cursorRect;

    IEditor *bestEditor = 0;
    int bestValue = 1 << 30;

    QList<IEditor*> editors = editorManager->visibleEditors();
    foreach (IEditor *editor, editors) {
        QWidget *w = editor->widget();
        QRect editorRect(w->mapToGlobal(w->geometry().topLeft()),
                w->mapToGlobal(w->geometry().bottomRight()));
        //qDebug() << "   EDITOR: " << editorRect << editor;

        int value = f(cursorRect, editorRect);
        if (value != -1 && value < bestValue) {
            bestValue = value;
            bestEditor = editor;
            //qDebug() << "          BEST SO FAR: " << bestValue << bestEditor;
        }
    }
    //qDebug() << "     BEST: " << bestValue << bestEditor;

    // FIME: This is know to fail as the EditorManager will fall back to
    // the current editor's view. Needs additional public API there.
    if (bestEditor)
        editorManager->activateEditor(bestEditor);
}

void FakeVimPluginPrivate::find(bool reverse)
{
    if (Find::FindPlugin *plugin = Find::FindPlugin::instance()) {
        plugin->setUseFakeVim(true);
        plugin->openFindToolBar(reverse
                ? Find::FindPlugin::FindBackward
                : Find::FindPlugin::FindForward);
    }
}

void FakeVimPluginPrivate::findNext(bool reverse)
{
    if (reverse)
        triggerAction(Find::Constants::FIND_PREVIOUS);
    else
        triggerAction(Find::Constants::FIND_NEXT);
}

// This class defers deletion of a child FakeVimHandler using deleteLater().
class DeferredDeleter : public QObject
{
    Q_OBJECT

    FakeVimHandler *m_handler;

public:
    DeferredDeleter(QObject *parent, FakeVimHandler *handler)
        : QObject(parent), m_handler(handler)
    {}

    virtual ~DeferredDeleter()
    {
        if (m_handler) {
            m_handler->disconnectFromEditor();
            m_handler->deleteLater();
            m_handler = 0;
        }
    }
};

void FakeVimPluginPrivate::editorOpened(Core::IEditor *editor)
{
    if (!editor)
        return;

    QWidget *widget = editor->widget();
    if (!widget)
        return;

    // we can only handle QTextEdit and QPlainTextEdit
    if (!qobject_cast<QTextEdit *>(widget) && !qobject_cast<QPlainTextEdit *>(widget))
        return;

    //qDebug() << "OPENING: " << editor << editor->widget()
    //    << "MODE: " << theFakeVimSetting(ConfigUseFakeVim)->value();

    FakeVimHandler *handler = new FakeVimHandler(widget, 0);
    // the handler might have triggered the deletion of the editor:
    // make sure that it can return before being deleted itself
    new DeferredDeleter(widget, handler);
    m_editorToHandler[editor] = handler;

    connect(handler, SIGNAL(extraInformationChanged(QString)),
        SLOT(showExtraInformation(QString)));
    connect(handler, SIGNAL(commandBufferChanged(QString,int)),
        SLOT(showCommandBuffer(QString,int)));
    connect(handler, SIGNAL(selectionChanged(QList<QTextEdit::ExtraSelection>)),
        SLOT(changeSelection(QList<QTextEdit::ExtraSelection>)));
    connect(handler, SIGNAL(moveToMatchingParenthesis(bool*,bool*,QTextCursor*)),
        SLOT(moveToMatchingParenthesis(bool*,bool*,QTextCursor*)));
    connect(handler, SIGNAL(indentRegion(int,int,QChar)),
        SLOT(indentRegion(int,int,QChar)));
    connect(handler, SIGNAL(checkForElectricCharacter(bool*,QChar)),
        SLOT(checkForElectricCharacter(bool*,QChar)));
    connect(handler, SIGNAL(requestSetBlockSelection(bool)),
        SLOT(setBlockSelection(bool)));
    connect(handler, SIGNAL(requestHasBlockSelection(bool*)),
        SLOT(hasBlockSelection(bool*)));
    connect(handler, SIGNAL(completionRequested()),
        SLOT(triggerCompletions()));
    connect(handler, SIGNAL(simpleCompletionRequested(QString,bool)),
        SLOT(triggerSimpleCompletions(QString,bool)));
    connect(handler, SIGNAL(windowCommandRequested(int)),
        SLOT(windowCommand(int)));
    connect(handler, SIGNAL(findRequested(bool)),
        SLOT(find(bool)));
    connect(handler, SIGNAL(findNextRequested(bool)),
        SLOT(findNext(bool)));

    connect(handler, SIGNAL(handleExCommandRequested(bool*,ExCommand)),
        SLOT(handleExCommand(bool*,ExCommand)));

    connect(ICore::instance(), SIGNAL(saveSettingsRequested()),
        SLOT(writeSettings()));

    handler->setCurrentFileName(editor->file()->fileName());
    handler->installEventFilter();

    // pop up the bar
    if (theFakeVimSetting(ConfigUseFakeVim)->value().toBool()) {
       showCommandBuffer(QString(), -1);
       handler->setupWidget();
    }
}

void FakeVimPluginPrivate::editorAboutToClose(Core::IEditor *editor)
{
    //qDebug() << "CLOSING: " << editor << editor->widget();
    m_editorToHandler.remove(editor);
}

void FakeVimPluginPrivate::setUseFakeVim(const QVariant &value)
{
    //qDebug() << "SET USE FAKEVIM" << value;
    bool on = value.toBool();
    if (Find::FindPlugin::instance())
        Find::FindPlugin::instance()->setUseFakeVim(on);
    setUseFakeVimInternal(on);
}

void FakeVimPluginPrivate::setUseFakeVimInternal(bool on)
{
    if (on) {
        //ICore *core = ICore::instance();
        //core->updateAdditionalContexts(Core::Context(FAKEVIM_CONTEXT),
        // Core::Context());
        foreach (Core::IEditor *editor, m_editorToHandler.keys())
            m_editorToHandler[editor]->setupWidget();
    } else {
        //ICore *core = ICore::instance();
        //core->updateAdditionalContexts(Core::Context(),
        // Core::Context(FAKEVIM_CONTEXT));
        showCommandBuffer(QString(), -1);
        foreach (Core::IEditor *editor, m_editorToHandler.keys()) {
            if (TextEditor::BaseTextEditorWidget *textEditor =
                    qobject_cast<TextEditor::BaseTextEditorWidget *>(editor->widget())) {
                m_editorToHandler[editor]->restoreWidget(textEditor->tabSettings().m_tabSize);
            }
        }
    }
}

void FakeVimPluginPrivate::triggerCompletions()
{
    FakeVimHandler *handler = qobject_cast<FakeVimHandler *>(sender());
    if (!handler)
        return;
    if (BaseTextEditorWidget *editor = qobject_cast<BaseTextEditorWidget *>(handler->widget()))
        editor->invokeAssist(Completion, m_wordProvider);
//        CompletionSupport::instance()->complete(editor->editor(), TextCompletion, false);
}

void FakeVimPluginPrivate::triggerSimpleCompletions(const QString &needle,
   bool forward)
{
//    m_wordCompletion->setActive(needle, forward, qobject_cast<FakeVimHandler *>(sender()));
    m_wordProvider->setActive(needle, forward, qobject_cast<FakeVimHandler *>(sender()));
}

void FakeVimPluginPrivate::setBlockSelection(bool on)
{
    FakeVimHandler *handler = qobject_cast<FakeVimHandler *>(sender());
    if (!handler)
        return;
    if (BaseTextEditorWidget *bt = qobject_cast<BaseTextEditorWidget *>(handler->widget()))
        bt->setBlockSelection(on);
}

void FakeVimPluginPrivate::hasBlockSelection(bool *on)
{
    FakeVimHandler *handler = qobject_cast<FakeVimHandler *>(sender());
    if (!handler)
        return;
    if (BaseTextEditorWidget *bt = qobject_cast<BaseTextEditorWidget *>(handler->widget()))
        *on = bt->hasBlockSelection();
}

void FakeVimPluginPrivate::checkForElectricCharacter(bool *result, QChar c)
{
    FakeVimHandler *handler = qobject_cast<FakeVimHandler *>(sender());
    if (!handler)
        return;
    if (BaseTextEditorWidget *bt = qobject_cast<BaseTextEditorWidget *>(handler->widget()))
        *result = bt->indenter()->isElectricCharacter(c);
}

void FakeVimPluginPrivate::handleExCommand(bool *handled, const ExCommand &cmd)
{
    using namespace Core;
    //qDebug() << "PLUGIN HANDLE: " << cmd.cmd << cmd.count;

    *handled = false;

    FakeVimHandler *handler = qobject_cast<FakeVimHandler *>(sender());
    if (!handler)
        return;

    *handled = true;
    if (cmd.matches("w", "write") || cmd.cmd == "wq") {
        // :w[rite]
        Core::IEditor *editor = m_editorToHandler.key(handler);
        const QString fileName = handler->currentFileName();
        if (editor && editor->file()->fileName() == fileName) {
            // Handle that as a special case for nicer interaction with core
            Core::ICore::instance()->fileManager()->saveFile(editor->file());
            // Check result by reading back.
            QFile file3(fileName);
            file3.open(QIODevice::ReadOnly);
            QByteArray ba = file3.readAll();
            handler->showBlackMessage(FakeVimHandler::tr("\"%1\" %2 %3L, %4C written")
                .arg(fileName).arg(" ")
                .arg(ba.count('\n')).arg(ba.size()));
            if (cmd.cmd == "wq")
                delayedQuitRequested(cmd.hasBang, m_editorToHandler.key(handler));
        } else {
            handler->showRedMessage(tr("File not saved"));
        }
    } else if (cmd.matches("wa", "wall")) {
        // :w[all]
        FileManager *fm = ICore::instance()->fileManager();
        QList<IFile *> toSave = fm->modifiedFiles();
        QList<IFile *> failed = fm->saveModifiedFilesSilently(toSave);
        if (failed.isEmpty())
            handler->showBlackMessage(tr("Saving succeeded"));
        else
            handler->showRedMessage(tr("%n files not saved", 0, failed.size()));
    } else if (cmd.matches("q", "quit")) {
        // :q[uit]
        emit delayedQuitRequested(cmd.hasBang, m_editorToHandler.key(handler));
    } else if (cmd.matches("qa", "qall")) {
        // :qa[ll]
        emit delayedQuitAllRequested(cmd.hasBang);
    } else if (cmd.matches("sp", "split")) {
        // :sp[lit]
        triggerAction(Core::Constants::SPLIT);
    } else if (cmd.matches("vs", "vsplit")) {
        // :vs[plit]
        triggerAction(Core::Constants::SPLIT_SIDE_BY_SIDE);
    } else if (cmd.matches("mak", "make")) {
        // :mak[e][!] [arguments]
        triggerAction(ProjectExplorer::Constants::BUILD);
    } else if (cmd.matches("se", "set")) {
        if (cmd.args.isEmpty()) {
            // :se[t]
            showSettingsDialog();
        } else if (cmd.args == "ic" || cmd.args == "ignorecase") {
            // :set nc
            setActionChecked(Find::Constants::CASE_SENSITIVE, false);
        } else if (cmd.args == "noic" || cmd.args == "noignorecase") {
            // :set noic
            setActionChecked(Find::Constants::CASE_SENSITIVE, true);
        } else {
            *handled = false; // Let the handler see it as well.
        }
    } else if (cmd.matches("n", "next")) {
        // :n[ext]
        switchToFile(currentFile() + cmd.count);
    } else if (cmd.matches("prev", "previous") || cmd.matches("N", "Next")) {
        // :prev[ious], :N[ext]
        switchToFile(currentFile() - cmd.count);
    } else if (cmd.matches("bn", "bnext")) {
        // :bn[ext]
        switchToFile(currentFile() + cmd.count);
    } else if (cmd.matches("bp", "bprevious") || cmd.matches("bN", "bNext")) {
        // :bp[revious], :bN[ext]
        switchToFile(currentFile() - cmd.count);
    } else if (cmd.matches("on", "only")) {
        // :on[ly]
        //triggerAction(Core::Constants::REMOVE_ALL_SPLITS);
        triggerAction(Core::Constants::REMOVE_CURRENT_SPLIT);
    } else if (cmd.cmd == "AS") {
        triggerAction(Core::Constants::SPLIT);
        triggerAction(CppTools::Constants::SWITCH_HEADER_SOURCE);
    } else if (cmd.cmd == "AV") {
        triggerAction(Core::Constants::SPLIT_SIDE_BY_SIDE);
        triggerAction(CppTools::Constants::SWITCH_HEADER_SOURCE);
    } else {
        // Check whether one of the configure commands matches.
        typedef ExCommandMap::const_iterator Iterator;
        const Iterator end = exCommandMap().constEnd();
        for (Iterator it = exCommandMap().constBegin(); it != end; ++it) {
            const QString &id = it.key();
            const QRegExp &re = it.value();
            if (!re.pattern().isEmpty() && re.indexIn(cmd.cmd) != -1) {
                triggerAction(Core::Id(id));
                return;
            }
        }
        *handled = false;
    }
}

void FakeVimPluginPrivate::handleDelayedQuit(bool forced, Core::IEditor *editor)
{
    // This tries to simulate vim behaviour. But the models of vim and
    // Qt Creator core do not match well...
    EditorManager *editorManager = ICore::instance()->editorManager();
    if (editorManager->hasSplitter()) {
        triggerAction(Core::Constants::REMOVE_CURRENT_SPLIT);
    } else {
        QList<Core::IEditor *> editors;
        editors.append(editor);
        editorManager->closeEditors(editors, !forced);
    }
}

void FakeVimPluginPrivate::handleDelayedQuitAll(bool forced)
{
    triggerAction(Core::Constants::REMOVE_ALL_SPLITS);
    ICore::instance()->editorManager()->closeAllEditors(!forced);
}

void FakeVimPluginPrivate::moveToMatchingParenthesis(bool *moved, bool *forward,
        QTextCursor *cursor)
{
    *moved = false;

    bool undoFakeEOL = false;
    if (cursor->atBlockEnd() && cursor->block().length() > 1) {
        cursor->movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, 1);
        undoFakeEOL = true;
    }
    TextBlockUserData::MatchType match
        = TextBlockUserData::matchCursorForward(cursor);
    if (match == TextBlockUserData::Match) {
        *moved = true;
        *forward = true;
    } else {
        if (undoFakeEOL)
            cursor->movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 1);
        if (match == TextBlockUserData::NoMatch) {
            // Backward matching is according to the character before the cursor.
            bool undoMove = false;
            if (!cursor->atBlockEnd()) {
                cursor->movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 1);
                undoMove = true;
            }
            match = TextBlockUserData::matchCursorBackward(cursor);
            if (match == TextBlockUserData::Match) {
                *moved = true;
                *forward = false;
            } else if (undoMove) {
                cursor->movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, 1);
            }
        }
    }
}

void FakeVimPluginPrivate::indentRegion(int beginLine, int endLine,
      QChar typedChar)
{
    FakeVimHandler *handler = qobject_cast<FakeVimHandler *>(sender());
    if (!handler)
        return;

    BaseTextEditorWidget *bt = qobject_cast<BaseTextEditorWidget *>(handler->widget());
    if (!bt)
        return;

    TabSettings tabSettings;
    tabSettings.m_indentSize = theFakeVimSetting(ConfigShiftWidth)->value().toInt();
    tabSettings.m_tabSize = theFakeVimSetting(ConfigTabStop)->value().toInt();
    tabSettings.m_tabPolicy = theFakeVimSetting(ConfigExpandTab)->value().toBool()
            ? TabSettings::SpacesOnlyTabPolicy : TabSettings::TabsOnlyTabPolicy;

    QTextDocument *doc = bt->document();
    QTextBlock startBlock = doc->findBlockByNumber(beginLine);

    // Record line lenghts for mark adjustments
    QVector<int> lineLengths(endLine - beginLine + 1);
    QTextBlock block = startBlock;

    for (int i = beginLine; i <= endLine; ++i) {
        lineLengths[i - beginLine] = block.text().length();
        if (typedChar == 0 && block.text().simplified().isEmpty()) {
            // clear empty lines
            QTextCursor cursor(block);
            while (!cursor.atBlockEnd())
                cursor.deleteChar();
        } else {
            bt->indenter()->indentBlock(doc, block, typedChar, tabSettings);
        }
        block = block.next();
    }
}

void FakeVimPluginPrivate::quitFakeVim()
{
    theFakeVimSetting(ConfigUseFakeVim)->setValue(false);
}

void FakeVimPluginPrivate::showCommandBuffer(const QString &contents, int cursorPos)
{
    //qDebug() << "SHOW COMMAND BUFFER" << contents;
    if (MiniBuffer *w = qobject_cast<MiniBuffer *>(m_statusBar->widget()))
        w->setContents(contents, cursorPos);
}

void FakeVimPluginPrivate::showExtraInformation(const QString &text)
{
    FakeVimHandler *handler = qobject_cast<FakeVimHandler *>(sender());
    if (handler)
        QMessageBox::information(handler->widget(), tr("FakeVim Information"), text);
}

void FakeVimPluginPrivate::changeSelection
    (const QList<QTextEdit::ExtraSelection> &selection)
{
    if (FakeVimHandler *handler = qobject_cast<FakeVimHandler *>(sender()))
        if (BaseTextEditorWidget *bt = qobject_cast<BaseTextEditorWidget *>(handler->widget()))
            bt->setExtraSelections(BaseTextEditorWidget::FakeVimSelection, selection);
}

int FakeVimPluginPrivate::currentFile() const
{
    EditorManager *editorManager = ICore::instance()->editorManager();
    OpenEditorsModel *model = editorManager->openedEditorsModel();
    IEditor *editor = editorManager->currentEditor();
    return model->indexOf(editor).row();
}

void FakeVimPluginPrivate::switchToFile(int n)
{
    EditorManager *editorManager = ICore::instance()->editorManager();
    Core::OpenEditorsModel *model = editorManager->openedEditorsModel();
    int size = model->rowCount();
    QTC_ASSERT(size, return);
    n = n % size;
    if (n < 0)
        n += size;
    editorManager->activateEditorForIndex(model->index(n, 0));
}

ExCommandMap &FakeVimExCommandsPage::exCommandMap()
{
    return m_q->exCommandMap();
}

ExCommandMap &FakeVimExCommandsPage::defaultExCommandMap()
{
    return m_q->defaultExCommandMap();
}

UserCommandMap &FakeVimUserCommandsPage::userCommandMap()
{
    return m_q->userCommandMap();
}

UserCommandMap &FakeVimUserCommandsPage::defaultUserCommandMap()
{
    return m_q->defaultUserCommandMap();
}

///////////////////////////////////////////////////////////////////////
//
// FakeVimPlugin
//
///////////////////////////////////////////////////////////////////////

FakeVimPlugin::FakeVimPlugin()
    : d(new FakeVimPluginPrivate(this))
{}

FakeVimPlugin::~FakeVimPlugin()
{
    delete d;
}

bool FakeVimPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorMessage)
    return d->initialize();
}

ExtensionSystem::IPlugin::ShutdownFlag FakeVimPlugin::aboutToShutdown()
{
    d->aboutToShutdown();
    return SynchronousShutdown;
}

void FakeVimPlugin::extensionsInitialized()
{
    d->m_statusBar = new Core::StatusBarWidget;
    d->m_statusBar->setWidget(new MiniBuffer);
    d->m_statusBar->setPosition(StatusBarWidget::Last);
    addAutoReleasedObject(d->m_statusBar);
}

} // namespace Internal
} // namespace FakeVim

#include "fakevimplugin.moc"

Q_EXPORT_PLUGIN(FakeVimPlugin)
