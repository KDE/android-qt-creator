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

#include "itemlibrarywidget.h"

#include <utils/filterlineedit.h>
#include <coreplugin/coreconstants.h>
#include "itemlibrarycomponents.h"
#include "itemlibrarymodel.h"
#include "itemlibraryimageprovider.h"
#include "customdraganddrop.h"
#include <model.h>
#include <metainfo.h>
#include "rewritingexception.h"

#include <QFileInfo>
#include <QFileIconProvider>
#include <QDirModel>
#include <QStackedWidget>
#include <QGridLayout>
#include <QTabBar>
#include <QImageReader>
#include <QMimeData>
#include <QWheelEvent>
#include <QMenu>
#include <QApplication>

#include <QDeclarativeView>
#include <QDeclarativeItem>
#include <private/qdeclarativeengine_p.h>


namespace QmlDesigner {

class MyFileIconProvider : public QFileIconProvider
{
public:
    MyFileIconProvider(const QSize &iconSize)
        : QFileIconProvider(),
          m_iconSize(iconSize)
    {}

    virtual QIcon icon ( const QFileInfo & info ) const
    {
        QPixmap pixmap(info.absoluteFilePath());
        if (pixmap.isNull()) {
            QIcon defaultIcon(QFileIconProvider::icon(info));
            pixmap = defaultIcon.pixmap(defaultIcon.actualSize(m_iconSize));
        }

        if (pixmap.width() == m_iconSize.width()
            && pixmap.height() == m_iconSize.height())
            return pixmap;

        if ((pixmap.width() > m_iconSize.width())
            || (pixmap.height() > m_iconSize.height()))
            return pixmap.scaled(m_iconSize, Qt::KeepAspectRatio,
                                 Qt::SmoothTransformation);

        QPoint offset((m_iconSize.width() - pixmap.width()) / 2,
                      (m_iconSize.height() - pixmap.height()) / 2);
        QImage newIcon(m_iconSize, QImage::Format_ARGB32_Premultiplied);
        newIcon.fill(Qt::transparent);
        QPainter painter(&newIcon);
        painter.drawPixmap(offset, pixmap);
        return QPixmap::fromImage(newIcon);
    }

private:
    QSize m_iconSize;
};


// ---------- ItemLibraryPrivate
class ItemLibraryWidgetPrivate {
public:
    ItemLibraryWidgetPrivate(QObject *object);

    Internal::ItemLibraryModel *m_itemLibraryModel;
    QDirModel *m_resourcesDirModel;

    QStackedWidget *m_stackedWidget;
    Utils::FilterLineEdit *m_lineEdit;
    QDeclarativeView *m_itemsView;
    Internal::ItemLibraryTreeView *m_resourcesView;
    QWeakPointer<ItemLibraryInfo> m_itemLibraryInfo;

    QSize m_itemIconSize, m_resIconSize;
    MyFileIconProvider m_iconProvider;
    Model *model;
};

ItemLibraryWidgetPrivate::ItemLibraryWidgetPrivate(QObject *object) :
    m_itemLibraryModel(0),
    m_resourcesDirModel(0),
    m_stackedWidget(0),
    m_lineEdit(0),
    m_itemsView(0),
    m_resourcesView(0),
    m_itemIconSize(24, 24),
    m_resIconSize(24, 24),
    m_iconProvider(m_resIconSize),
    model(0)
{
    Q_UNUSED(object);
}

ItemLibraryWidget::ItemLibraryWidget(QWidget *parent) :
    QFrame(parent),
    m_d(new ItemLibraryWidgetPrivate(this)),
    m_filterFlag(QtBasic)
{
    setWindowTitle(tr("Library", "Title of library view"));

    /* create Items view and its model */
    m_d->m_itemsView = new QDeclarativeView(this);
    m_d->m_itemsView->setAttribute(Qt::WA_OpaquePaintEvent);
    m_d->m_itemsView->setAttribute(Qt::WA_NoSystemBackground);
    m_d->m_itemsView->setAcceptDrops(false);
    m_d->m_itemsView->setFocusPolicy(Qt::ClickFocus);
    m_d->m_itemsView->setResizeMode(QDeclarativeView::SizeRootObjectToView);
    m_d->m_itemLibraryModel = new Internal::ItemLibraryModel(QDeclarativeEnginePrivate::getScriptEngine(m_d->m_itemsView->engine()), this);
    m_d->m_itemLibraryModel->setItemIconSize(m_d->m_itemIconSize);

    QDeclarativeContext *rootContext = m_d->m_itemsView->rootContext();
    rootContext->setContextProperty(QLatin1String("itemLibraryModel"), m_d->m_itemLibraryModel);
    rootContext->setContextProperty(QLatin1String("itemLibraryIconWidth"), m_d->m_itemIconSize.width());
    rootContext->setContextProperty(QLatin1String("itemLibraryIconHeight"), m_d->m_itemIconSize.height());

    QColor highlightColor = palette().highlight().color();
    if (0.5*highlightColor.saturationF()+0.75-highlightColor.valueF() < 0)
        highlightColor.setHsvF(highlightColor.hsvHueF(),0.1 + highlightColor.saturationF()*2.0, highlightColor.valueF());
    m_d->m_itemsView->rootContext()->setContextProperty(QLatin1String("highlightColor"), highlightColor);

    // loading the qml has to come after all needed context properties are set
    m_d->m_itemsView->setSource(QUrl("qrc:/ItemLibrary/qml/ItemsView.qml"));

    QDeclarativeItem *rootItem = qobject_cast<QDeclarativeItem*>(m_d->m_itemsView->rootObject());
    connect(rootItem, SIGNAL(itemSelected(int)), this, SLOT(showItemInfo(int)));
    connect(rootItem, SIGNAL(itemDragged(int)), this, SLOT(startDragAndDrop(int)));
    connect(this, SIGNAL(scrollItemsView(QVariant)), rootItem, SLOT(scrollView(QVariant)));
    connect(this, SIGNAL(resetItemsView()), rootItem, SLOT(resetView()));

    /* create Resources view and its model */
    m_d->m_resourcesDirModel = new QDirModel(this);
    m_d->m_resourcesDirModel->setIconProvider(&m_d->m_iconProvider);
    m_d->m_resourcesView = new Internal::ItemLibraryTreeView(this);
    m_d->m_resourcesView->setModel(m_d->m_resourcesDirModel);
    m_d->m_resourcesView->setIconSize(m_d->m_resIconSize);

    /* create image provider for loading item icons */
    m_d->m_itemsView->engine()->addImageProvider(QLatin1String("qmldesigner_itemlibrary"), new Internal::ItemLibraryImageProvider);

    /* other widgets */
    QTabBar *tabBar = new QTabBar(this);
    tabBar->addTab(tr("Items", "Title of library items view"));
    tabBar->addTab(tr("Resources", "Title of library resources view"));
    tabBar->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    m_d->m_lineEdit = new Utils::FilterLineEdit(this);
    m_d->m_lineEdit->setObjectName(QLatin1String("itemLibrarySearchInput"));
    m_d->m_lineEdit->setPlaceholderText(tr("<Filter>", "Library search input hint text"));
    m_d->m_lineEdit->setDragEnabled(false);
    m_d->m_lineEdit->setMinimumWidth(75);
    m_d->m_lineEdit->setTextMargins(0, 0, 20, 0);
    QWidget *lineEditFrame = new QWidget(this);
    lineEditFrame->setObjectName(QLatin1String("itemLibrarySearchInputFrame"));
    QGridLayout *lineEditLayout = new QGridLayout(lineEditFrame);
    lineEditLayout->setMargin(2);
    lineEditLayout->setSpacing(0);
    lineEditLayout->addItem(new QSpacerItem(5, 3, QSizePolicy::Fixed, QSizePolicy::Fixed), 0, 0, 1, 3);
    lineEditLayout->addItem(new QSpacerItem(5, 5, QSizePolicy::Fixed, QSizePolicy::Fixed), 1, 0);
    lineEditLayout->addWidget(m_d->m_lineEdit, 1, 1, 1, 1);
    lineEditLayout->addItem(new QSpacerItem(5, 5, QSizePolicy::Fixed, QSizePolicy::Fixed), 1, 2);
    connect(m_d->m_lineEdit, SIGNAL(filterChanged(QString)), this, SLOT(setSearchFilter(QString)));

    m_d->m_stackedWidget = new QStackedWidget(this);
    m_d->m_stackedWidget->addWidget(m_d->m_itemsView);
    m_d->m_stackedWidget->addWidget(m_d->m_resourcesView);
    connect(tabBar, SIGNAL(currentChanged(int)),
            m_d->m_stackedWidget, SLOT(setCurrentIndex(int)));
    connect(tabBar, SIGNAL(currentChanged(int)),
            this, SLOT(updateSearch()));

    QWidget *spacer = new QWidget(this);
    spacer->setObjectName(QLatin1String("itemLibrarySearchInputSpacer"));
    spacer->setFixedHeight(4);

    QGridLayout *layout = new QGridLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(tabBar, 0, 0, 1, 1);
    layout->addWidget(spacer, 1, 0);
    layout->addWidget(lineEditFrame, 2, 0, 1, 1);
    layout->addWidget(m_d->m_stackedWidget, 3, 0, 1, 1);

    setResourcePath(QDir::currentPath());
    setSearchFilter(QString());

    /* style sheets */
    {
        QFile file(":/qmldesigner/stylesheet.css");
        file.open(QFile::ReadOnly);
        QString styleSheet = QLatin1String(file.readAll());
        setStyleSheet(styleSheet);
    }

    {
        QFile file(":/qmldesigner/scrollbar.css");
        file.open(QFile::ReadOnly);
        QString styleSheet = QLatin1String(file.readAll());
        m_d->m_resourcesView->setStyleSheet(styleSheet);
    }
}

ItemLibraryWidget::~ItemLibraryWidget()
{
    /* workaround: delete the items view before the model is deleted.
       This prevents qml warnings when the item library is destructed. */
    delete m_d->m_itemsView;
    delete m_d->m_resourcesView;
    delete m_d;
}

void ItemLibraryWidget::setItemLibraryInfo(ItemLibraryInfo *itemLibraryInfo)
{
    if (m_d->m_itemLibraryInfo.data() == itemLibraryInfo)
        return;

    if (m_d->m_itemLibraryInfo)
        disconnect(m_d->m_itemLibraryInfo.data(), SIGNAL(entriesChanged()),
                   this, SLOT(updateModel()));
    m_d->m_itemLibraryInfo = itemLibraryInfo;
    if (itemLibraryInfo)
        connect(m_d->m_itemLibraryInfo.data(), SIGNAL(entriesChanged()),
                this, SLOT(updateModel()));

    updateModel();
    updateSearch();
}

void ItemLibraryWidget::updateImports()
{
    FilterChangeFlag filter;
    filter = QtBasic;
    if (m_d->model) {
        QStringList imports;
        foreach (const Import &import, m_d->model->imports())
            if (import.isLibraryImport())
                imports << import.url();
        if (imports.contains("com.nokia.symbian", Qt::CaseInsensitive))
            filter = Symbian;
        if (imports.contains("com.Meego", Qt::CaseInsensitive))
            filter = Meego;
    }

    setImportFilter(filter);
}

QList<QToolButton *> ItemLibraryWidget::createToolBarWidgets()
{
    QList<QToolButton *> buttons;
    buttons << new QToolButton();
    buttons.first()->setText("I ");
    buttons.first()->setIcon(QIcon(QLatin1String(Core::Constants::ICON_FILTER)));
    buttons.first()->setToolTip("Manage imports for components");
    buttons.first()->setPopupMode(QToolButton::InstantPopup);
    QMenu * menu = new QMenu;
    QAction * basicQtAction = new QAction(menu);
    basicQtAction->setCheckable(true);
    basicQtAction->setText("Basic Qt Quick only");
    QAction * symbianAction = new QAction(menu);
    symbianAction->setCheckable(true);
    symbianAction->setText("Symbian Components");
    QAction * meegoAction= new QAction(menu);
    meegoAction->setCheckable(true);
    meegoAction->setText("Meego Components");
    menu->addAction(basicQtAction);
    menu->addAction(meegoAction);
    menu->addAction(symbianAction);
    buttons.first()->setMenu(menu);

    connect(basicQtAction, SIGNAL(toggled(bool)), this, SLOT(onQtBasicOnlyChecked(bool)));
    connect(this, SIGNAL(qtBasicOnlyChecked(bool)), basicQtAction, SLOT(setChecked(bool)));

    connect(symbianAction, SIGNAL(toggled(bool)), this, SLOT(onSymbianChecked(bool)));
    connect(this, SIGNAL(symbianChecked(bool)), symbianAction, SLOT(setChecked(bool)));

    connect(meegoAction, SIGNAL(toggled(bool)), this, SLOT(onMeegoChecked(bool)));
    connect(this, SIGNAL(meegoChecked(bool)), meegoAction, SLOT(setChecked(bool)));

    updateImports();

    return buttons;
}

void ItemLibraryWidget::setSearchFilter(const QString &searchFilter)
{
    if (m_d->m_stackedWidget->currentIndex() == 0) {
        m_d->m_itemLibraryModel->setSearchText(searchFilter);
        emit resetItemsView();
        m_d->m_itemsView->update();
    } else {
        QStringList nameFilterList;
        if (searchFilter.contains('.')) {
            nameFilterList.append(QString("*%1*").arg(searchFilter));
        } else {
            foreach (const QByteArray &extension, QImageReader::supportedImageFormats()) {
                nameFilterList.append(QString("*%1*.%2").arg(searchFilter, QString::fromAscii(extension)));
            }
        }

        m_d->m_resourcesDirModel->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);
        m_d->m_resourcesDirModel->setNameFilters(nameFilterList);
        m_d->m_resourcesView->expandToDepth(1);
        m_d->m_resourcesView->scrollToTop();
    }
}

void ItemLibraryWidget::setModel(Model *model)
{
    m_d->model = model;
    if (!model)
        return;
    setItemLibraryInfo(model->metaInfo().itemLibraryInfo());
    updateModel();
}

void ItemLibraryWidget::emitImportChecked()
{
    if (!m_d->model)
        return;

    bool qtOnlyImport = false;
    bool meegoImport = false;
    bool symbianImport = false;

    foreach (const Import &import, m_d->model->imports()) {
        if (import.isLibraryImport()) {
            if (import.url().contains(QString("meego"), Qt::CaseInsensitive))
                meegoImport = true;
            if (import.url().contains(QString("Qt"), Qt::CaseInsensitive) || import.url().contains(QString("QtQuick"), Qt::CaseInsensitive))
                qtOnlyImport = true;
            if (import.url().contains(QString("symbian"), Qt::CaseInsensitive))
                symbianImport = true;
        }
    }

    if (meegoImport || symbianImport)
        qtOnlyImport = false;

    emit qtBasicOnlyChecked(qtOnlyImport);
    emit meegoChecked(meegoImport);
    emit symbianChecked(symbianImport);
}

void ItemLibraryWidget::setImportFilter(FilterChangeFlag flag)
{

    static bool block = false;
    if (!m_d->model)
        return;
    if (flag == m_filterFlag)
        return;

    if (block == true)
        return;


    FilterChangeFlag oldfilterFlag = m_filterFlag;

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    try {
        block = true;
        if (flag == QtBasic) {
            removeImport(QLatin1String("com.meego"));
            removeImport(QLatin1String("com.nokia.symbian"));
        } else  if (flag == Symbian) {
            removeImport(QLatin1String("com.meego"));
            addImport(QLatin1String("com.nokia.symbian"), QLatin1String("1.0"));
        }  else  if (flag == Meego) {
            removeImport(QLatin1String("com.nokia.symbian"));
            addImport(QLatin1String("com.meego"), QLatin1String("1.0"));
        }
        QApplication::restoreOverrideCursor();
        block = false;
        m_filterFlag = flag;
    } catch (RewritingException &xcetion) {
        QApplication::restoreOverrideCursor();
        m_filterFlag = oldfilterFlag;
        block = false;
        // do something about it
    }

    emitImportChecked();
}

void ItemLibraryWidget::onQtBasicOnlyChecked(bool b)
{
    if (b)
        setImportFilter(QtBasic);

}

void ItemLibraryWidget::onMeegoChecked(bool b)
{
    if (b)
        setImportFilter(Meego);
}

void ItemLibraryWidget::onSymbianChecked(bool b)
{
    if (b)
        setImportFilter(Symbian);
}



void ItemLibraryWidget::updateModel()
{
    m_d->m_itemLibraryModel->update(m_d->m_itemLibraryInfo.data(), m_d->model);
    updateImports();
    updateSearch();
}

void ItemLibraryWidget::updateSearch()
{
    setSearchFilter(m_d->m_lineEdit->text());
}

void ItemLibraryWidget::setResourcePath(const QString &resourcePath)
{
    if (m_d->m_resourcesView->model() == m_d->m_resourcesDirModel)
        m_d->m_resourcesView->setRootIndex(m_d->m_resourcesDirModel->index(resourcePath));
    updateSearch();
}

void ItemLibraryWidget::startDragAndDrop(int itemLibId)
{
    QMimeData *mimeData = m_d->m_itemLibraryModel->getMimeData(itemLibId);
    CustomItemLibraryDrag *drag = new CustomItemLibraryDrag(this);
    const QImage image = qvariant_cast<QImage>(mimeData->imageData());

    drag->setPixmap(m_d->m_itemLibraryModel->getIcon(itemLibId).pixmap(32, 32));
    drag->setPreview(QPixmap::fromImage(image));
    drag->setMimeData(mimeData);

    QDeclarativeItem *rootItem = qobject_cast<QDeclarativeItem*>(m_d->m_itemsView->rootObject());
    connect(rootItem, SIGNAL(stopDragAndDrop()), drag, SLOT(stopDrag()));

    drag->exec();
}

void ItemLibraryWidget::showItemInfo(int /*itemLibId*/)
{
//    qDebug() << "showing item info about id" << itemLibId;
}

void ItemLibraryWidget::wheelEvent(QWheelEvent *event)
{
    if (m_d->m_stackedWidget->currentIndex() == 0 &&
        m_d->m_itemsView->rect().contains(event->pos())) {
        emit scrollItemsView(event->delta());
        event->accept();
    } else
        QFrame::wheelEvent(event);
}

 void ItemLibraryWidget::removeImport(const QString &name)
 {
     if (!m_d->model)
         return;
     foreach (const Import &import, m_d->model->imports())
         if (import.isLibraryImport() && import.url().compare(name, Qt::CaseInsensitive) == 0)
             m_d->model->removeImport(import);
 }

 void ItemLibraryWidget::addImport(const QString &name, const QString &version)
 {
     if (!m_d->model)
         return;

     m_d->model->addImport(Import::createLibraryImport(name, version));

 }

}
