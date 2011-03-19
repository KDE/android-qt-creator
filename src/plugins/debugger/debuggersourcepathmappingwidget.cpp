/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "debuggersourcepathmappingwidget.h"

#include <utils/pathchooser.h>
#include <utils/qtcassert.h>

#include <QtGui/QVBoxLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QStandardItemModel>
#include <QtGui/QStandardItem>
#include <QtGui/QTreeView>
#include <QtGui/QLineEdit>
#include <QtGui/QSpacerItem>
#include <QtGui/QPushButton>
#include <QtGui/QFormLayout>
#include <QtGui/QFileDialog>
#include <QtGui/QLabel>

#include <QtCore/QDir>
#include <QtCore/QPair>

// Qt's various build paths for unpatched versions.
#if defined(Q_OS_WIN)
static const char* qtBuildPaths[] = {
"C:/qt-greenhouse/Trolltech/Code_less_create_more/Trolltech/Code_less_create_more/Troll/4.6/qt",
"C:/iwmake/build_mingw_opensource",
"C:/ndk_buildrepos/qt-desktop/src"};
#elif defined(Q_OS_MAC)
static const char* qtBuildPaths[] = {};
#else
static const char* qtBuildPaths[] = {"/var/tmp/qt-src"};
#endif

enum { SourceColumn, TargetColumn, ColumnCount };

namespace Debugger {
namespace Internal {

/*!
    \class SourcePathMappingModel

    \brief Model for DebuggerSourcePathMappingWidget.

    Maintains mappings and a dummy placeholder row for adding new mappings.
*/

class SourcePathMappingModel : public QStandardItemModel
{
public:
    typedef QPair<QString, QString> Mapping;
    typedef DebuggerSourcePathMappingWidget::SourcePathMap SourcePathMap;

    explicit SourcePathMappingModel(QObject *parent);

    SourcePathMap sourcePathMap() const;
    void setSourcePathMap(const SourcePathMap&);

    Mapping mappingAt(int row) const;
    bool isNewPlaceHolderAt(int row) { return isNewPlaceHolder(rawMappingAt(row)); }

    void addMapping(const QString &source, const QString &target)
        { addRawMapping(QDir::toNativeSeparators(source), QDir::toNativeSeparators(target)); }

    void addNewMappingPlaceHolder()
        { addRawMapping(m_newSourcePlaceHolder, m_newTargetPlaceHolder); }

    void setSource(int row, const QString &);
    void setTarget(int row, const QString &);

private:
    inline bool isNewPlaceHolder(const Mapping &m) const;
    inline Mapping rawMappingAt(int row) const;
    void addRawMapping(const QString &source, const QString &target);

    const QString m_newSourcePlaceHolder;
    const QString m_newTargetPlaceHolder;
};

SourcePathMappingModel::SourcePathMappingModel(QObject *parent) :
    QStandardItemModel(0, ColumnCount, parent),
    m_newSourcePlaceHolder(DebuggerSourcePathMappingWidget::tr("<new source>")),
    m_newTargetPlaceHolder(DebuggerSourcePathMappingWidget::tr("<new target>"))
{
    QStringList headers;
    headers << DebuggerSourcePathMappingWidget::tr("Source path") << DebuggerSourcePathMappingWidget::tr("Target path");
    setHorizontalHeaderLabels(headers);
}

SourcePathMappingModel::SourcePathMap SourcePathMappingModel::sourcePathMap() const
{
    SourcePathMap rc;
    const int rows = rowCount();
    for (int r = 0; r < rows; r++) {
        const QPair<QString, QString> m = mappingAt(r); // Skip placeholders.
        if (!m.first.isEmpty() && !m.second.isEmpty())
            rc.insert(m.first, m.second);
    }
    return rc;
}

// Check a mapping whether it still contains a placeholder.
bool SourcePathMappingModel::isNewPlaceHolder(const Mapping &m) const
{
    const QLatin1Char lessThan('<');
    const QLatin1Char greaterThan('<');
    return m.first.isEmpty() || m.first.startsWith(lessThan) || m.first.endsWith(greaterThan)
           || m.first == m_newSourcePlaceHolder
           || m.second.isEmpty() || m.second.startsWith(lessThan) || m.second.endsWith(greaterThan)
           || m.second == m_newTargetPlaceHolder;
}

// Return raw, unfixed mapping
SourcePathMappingModel::Mapping SourcePathMappingModel::rawMappingAt(int row) const
{
    return Mapping(item(row, SourceColumn)->text(), item(row, TargetColumn)->text());
}

// Return mapping, empty if it is the place holder.
SourcePathMappingModel::Mapping SourcePathMappingModel::mappingAt(int row) const
{
    const Mapping raw = rawMappingAt(row);
    return isNewPlaceHolder(raw) ? Mapping() : Mapping(QDir::cleanPath(raw.first), QDir::cleanPath(raw.second));
}

void SourcePathMappingModel::setSourcePathMap(const SourcePathMap &m)
{
    removeRows(0, rowCount());
    const SourcePathMap::const_iterator cend = m.constEnd();
    for (SourcePathMap::const_iterator it = m.constBegin(); it != cend; ++it)
        addMapping(it.key(), it.value());
}

void SourcePathMappingModel::addRawMapping(const QString &source, const QString &target)
{
    QList<QStandardItem *> items;
    QStandardItem *sourceItem = new QStandardItem(source);
    sourceItem->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable);
    QStandardItem *targetItem = new QStandardItem(target);
    targetItem->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable);
    items << sourceItem << targetItem;
    appendRow(items);
}

void SourcePathMappingModel::setSource(int row, const QString &s)
{
    QStandardItem *sourceItem = item(row, SourceColumn);
    QTC_ASSERT(sourceItem, return; )
    sourceItem->setText(s.isEmpty() ? m_newSourcePlaceHolder : QDir::toNativeSeparators(s));
}

void SourcePathMappingModel::setTarget(int row, const QString &t)
{
    QStandardItem *targetItem = item(row, TargetColumn);
    QTC_ASSERT(targetItem, return; )
    targetItem->setText(t.isEmpty() ? m_newTargetPlaceHolder : QDir::toNativeSeparators(t));
}

/*!
    \class DebuggerSourcePathMappingWidget

    \brief Widget for maintaining a set of source path mappings for the debugger.

    Path mappings to be applied using source path substitution in gdb.
*/

DebuggerSourcePathMappingWidget::DebuggerSourcePathMappingWidget(QWidget *parent) :
    QGroupBox(parent),
    m_model(new SourcePathMappingModel(this)),
    m_treeView(new QTreeView),
    m_addButton(new QPushButton(tr("Add"))),
    m_addQtButton(new QPushButton(tr("Add Qt sources..."))),
    m_removeButton(new QPushButton(tr("Remove"))),
    m_sourceLineEdit(new QLineEdit),
    m_targetChooser(new Utils::PathChooser)
{
    setTitle(tr("Source Paths Mapping"));
    setToolTip(tr("<html><head/><body><p>Mappings of source file folders to be used in the debugger can be entered here.</p>"
                  "<p>This is useful when using a copy of the source tree at a location different from the one "
                  "at which the modules where built, for example, while doing remote debugging.</body></html>"));
    // Top list/left part.
    m_treeView->setRootIsDecorated(false);
    m_treeView->setUniformRowHeights(true);
    m_treeView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_treeView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_treeView->setModel(m_model);
    connect(m_treeView->selectionModel(), SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
            this, SLOT(slotCurrentRowChanged(QModelIndex,QModelIndex)));

    // Top list/Right part: Buttons.
    QVBoxLayout *buttonLayout = new QVBoxLayout;
    buttonLayout->addWidget(m_addButton);
    buttonLayout->addWidget(m_addQtButton);
    m_addQtButton->setVisible(sizeof(qtBuildPaths) > 0);
    m_addQtButton->setToolTip(tr("Add a mapping for Qt's source folders when using an unpatched version of Qt."));
    buttonLayout->addWidget(m_removeButton);
    connect(m_addButton, SIGNAL(clicked()), this, SLOT(slotAdd()));
    connect(m_addQtButton, SIGNAL(clicked()), this, SLOT(slotAddQt()));

    connect(m_removeButton, SIGNAL(clicked()), this, SLOT(slotRemove()));
    buttonLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Ignored, QSizePolicy::MinimumExpanding));

    // Assemble top
    QHBoxLayout *treeHLayout = new QHBoxLayout;
    treeHLayout->addWidget(m_treeView);
    treeHLayout->addLayout(buttonLayout);

    // Edit part
    m_targetChooser->setExpectedKind(Utils::PathChooser::ExistingDirectory);
    connect(m_sourceLineEdit, SIGNAL(textChanged(QString)), this, SLOT(slotEditSourceFieldChanged()));
    connect(m_targetChooser, SIGNAL(changed(QString)), this, SLOT(slotEditTargetFieldChanged()));
    QFormLayout *editLayout = new QFormLayout;
    const QString sourceToolTip = tr("The source path contained in the executable's debug information as reported by the debugger");
    QLabel *editSourceLabel = new QLabel(tr("&Source path:"));
    editSourceLabel->setToolTip(sourceToolTip);
    m_sourceLineEdit->setToolTip(sourceToolTip);
    editSourceLabel->setBuddy(m_sourceLineEdit);
    editLayout->addRow(editSourceLabel, m_sourceLineEdit);

    const QString targetToolTip = tr("The actual location of the source tree on the local machine");
    QLabel *editTargetLabel = new QLabel(tr("&Target path:"));
    editTargetLabel->setToolTip(targetToolTip);
    editTargetLabel->setBuddy(m_targetChooser);
    m_targetChooser->setToolTip(targetToolTip);
    editLayout->addRow(editTargetLabel, m_targetChooser);

    // Main layout
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(treeHLayout);
    mainLayout->addLayout(editLayout);
    setLayout(mainLayout);
    updateEnabled();
}

QString DebuggerSourcePathMappingWidget::editSourceField() const
{
    return QDir::cleanPath(m_sourceLineEdit->text().trimmed());
}

QString DebuggerSourcePathMappingWidget::editTargetField() const
{
    return m_targetChooser->path();
}

void DebuggerSourcePathMappingWidget::setEditFieldMapping(const QPair<QString, QString> &m)
{
    m_sourceLineEdit->setText(QDir::toNativeSeparators(m.first));
    m_targetChooser->setPath(m.second);
}

void DebuggerSourcePathMappingWidget::slotCurrentRowChanged(const QModelIndex &current,const QModelIndex &)
{
    setEditFieldMapping(current.isValid() ? m_model->mappingAt(current.row()) : QPair<QString, QString>());
    updateEnabled();
}

void DebuggerSourcePathMappingWidget::resizeColumns()
{
    m_treeView->resizeColumnToContents(SourceColumn);
}

void DebuggerSourcePathMappingWidget::updateEnabled()
{
    // Allow for removing the current item.
    const int row = currentRow();
    const bool hasCurrent = row >= 0;
    m_sourceLineEdit->setEnabled(hasCurrent);
    m_targetChooser->setEnabled(hasCurrent);
    m_removeButton->setEnabled(hasCurrent);
    // Allow for adding only if the current item no longer is the place holder for new items.
    const bool canAdd = !hasCurrent || !m_model->isNewPlaceHolderAt(row);
    m_addButton->setEnabled(canAdd);
    m_addQtButton->setEnabled(canAdd);
}

DebuggerSourcePathMappingWidget::SourcePathMap DebuggerSourcePathMappingWidget::sourcePathMap() const
{
    return m_model->sourcePathMap();
}

void DebuggerSourcePathMappingWidget::setSourcePathMap(const SourcePathMap &m)
{
    m_model->setSourcePathMap(m);
    if (!m.isEmpty())
        resizeColumns();
}

int DebuggerSourcePathMappingWidget::currentRow() const
{
    const QModelIndex index = m_treeView->selectionModel()->currentIndex();
    return index.isValid() ? index.row() : -1;
}

void DebuggerSourcePathMappingWidget::setCurrentRow(int r)
{
    m_treeView->selectionModel()->setCurrentIndex(m_model->index(r, 0),
                                                  QItemSelectionModel::ClearAndSelect
                                                  |QItemSelectionModel::Current
                                                  |QItemSelectionModel::Rows);
}

void DebuggerSourcePathMappingWidget::slotAdd()
{
    m_model->addNewMappingPlaceHolder();
    setCurrentRow(m_model->rowCount() - 1);
}

void DebuggerSourcePathMappingWidget::slotAddQt()
{
    // Add a mapping for various Qt build locations in case of unpatched builds.
    const QString qtSourcesPath = QFileDialog::getExistingDirectory(this, tr("Qt Sources"));
    if (qtSourcesPath.isEmpty())
        return;
    const size_t buildPathCount = sizeof(qtBuildPaths)/sizeof(const char *);
    for (size_t i = 0; i < buildPathCount; i++)
        m_model->addMapping(QString::fromLatin1(qtBuildPaths[i]), qtSourcesPath);
    resizeColumns();
    setCurrentRow(m_model->rowCount() - 1);
}

void DebuggerSourcePathMappingWidget::slotRemove()
{
    const int row = currentRow();
    if (row >= 0)
        m_model->removeRow(row);
}

void DebuggerSourcePathMappingWidget::slotEditSourceFieldChanged()
{
    const int row = currentRow();
    if (row >= 0) {
        m_model->setSource(row, editSourceField());
        updateEnabled();
    }
}

void DebuggerSourcePathMappingWidget::slotEditTargetFieldChanged()
{
    const int row = currentRow();
    if (row >= 0) {
        m_model->setTarget(row, editTargetField());
        updateEnabled();
    }
}

/* Merge settings for an installed Qt (unless another setting
 * is already in the map. */
DebuggerSourcePathMappingWidget::SourcePathMap
    DebuggerSourcePathMappingWidget::mergePlatformQtPath(const QString &qtInstallPath,
                                                         const SourcePathMap &in)
{
    SourcePathMap rc = in;
    const size_t buildPathCount = sizeof(qtBuildPaths)/sizeof(const char *);
    if (qtInstallPath.isEmpty() || buildPathCount == 0)
        return rc;

    for (size_t i = 0; i < buildPathCount; i++) {
        const QString buildPath = QString::fromLatin1(qtBuildPaths[i]);
        if (!rc.contains(buildPath)) // Do not overwrite user settings.
            rc.insert(buildPath, qtInstallPath);
    }
    return rc;
}

} // namespace Internal
} // namespace Debugger
