/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "itemlibrarywidget.h"

#include <utils/filterlineedit.h>
#include "itemlibrarycomponents.h"
#include "itemlibrarymodel.h"
#include "itemlibraryimageprovider.h"
#include "customdraganddrop.h"
#include <model.h>
#include <metainfo.h>

#include <QFileInfo>
#include <QFileIconProvider>
#include <QDirModel>
#include <QStackedWidget>
#include <QGridLayout>
#include <QTabBar>
#include <QImageReader>
#include <QMimeData>
#include <QWheelEvent>

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
    m_iconProvider(m_resIconSize)
{
    Q_UNUSED(object);
}

ItemLibraryWidget::ItemLibraryWidget(QWidget *parent) :
    QFrame(parent),
    m_d(new ItemLibraryWidgetPrivate(this))
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

void ItemLibraryWidget::updateModel()
{
    m_d->m_itemLibraryModel->update(m_d->m_itemLibraryInfo.data(), m_d->model);
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

}
