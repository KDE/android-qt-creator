/*
  This file was generated by the Html5 Application wizard of Qt Creator.
  Html5ApplicationViewer is a convenience class containing mobile device specific
  code such as screen orientation handling.
  It is recommended not to modify this file, since newer versions of Qt Creator
  may offer an updated version of it.
*/

#include "html5applicationviewer.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtGui/QVBoxLayout>
#include <QtGui/QGraphicsView>
#include <QtGui/QGraphicsScene>
#include <QtGui/QGraphicsLinearLayout>
#include <QtWebKit/QGraphicsWebView>
#include <QtWebKit/QWebFrame>

#if defined(Q_OS_SYMBIAN) && defined(ORIENTATIONLOCK)
#include <eikenv.h>
#include <eikappui.h>
#include <aknenv.h>
#include <aknappui.h>
#endif // Q_OS_SYMBIAN && ORIENTATIONLOCK

class Html5ApplicationViewerPrivate : public QGraphicsView
{
    Q_OBJECT
public:
    Html5ApplicationViewerPrivate(QWidget *parent = 0);

    void resizeEvent(QResizeEvent *event);
    static QString adjustPath(const QString &path);

public slots:
    void quit();

private slots:
    void addToJavaScript();

signals:
    void quitRequested();

public:
    QGraphicsWebView *m_webView;
};

Html5ApplicationViewerPrivate::Html5ApplicationViewerPrivate(QWidget *parent)
    : QGraphicsView(parent)
{
    QGraphicsScene *scene = new QGraphicsScene;
    setScene(scene);
    setFrameShape(QFrame::NoFrame);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_webView = new QGraphicsWebView;
    scene->addItem(m_webView);
    connect(m_webView->page()->mainFrame(),
            SIGNAL(javaScriptWindowObjectCleared()), SLOT(addToJavaScript()));
}

void Html5ApplicationViewerPrivate::resizeEvent(QResizeEvent *event)
{
    m_webView->resize(event->size());
}

QString Html5ApplicationViewerPrivate::adjustPath(const QString &path)
{
#ifdef Q_OS_UNIX
#ifdef Q_OS_MAC
    if (!QDir::isAbsolutePath(path))
        return QCoreApplication::applicationDirPath()
                + QLatin1String("/../Resources/") + path;
#else
    const QString pathInShareDir = QCoreApplication::applicationDirPath()
        + QLatin1String("/../share/")
        + QFileInfo(QCoreApplication::applicationFilePath()).fileName()
        + QLatin1Char('/') + path;
    if (QFileInfo(pathInShareDir).exists())
        return pathInShareDir;
#endif
#endif
    return path;
}

void Html5ApplicationViewerPrivate::quit()
{
    emit quitRequested();
}

void Html5ApplicationViewerPrivate::addToJavaScript()
{
    m_webView->page()->mainFrame()->addToJavaScriptWindowObject("Qt", this);
}

Html5ApplicationViewer::Html5ApplicationViewer(QWidget *parent)
    : QWidget(parent)
    , m_d(new Html5ApplicationViewerPrivate(this))
{
    connect(m_d, SIGNAL(quitRequested()), SLOT(close()));
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(m_d);
    layout->setMargin(0);
    setLayout(layout);
}

Html5ApplicationViewer::~Html5ApplicationViewer()
{
    delete m_d;
}

void Html5ApplicationViewer::loadFile(const QString &fileName)
{
    m_d->m_webView->setUrl(QUrl(Html5ApplicationViewerPrivate::adjustPath(fileName)));
}

void Html5ApplicationViewer::loadUrl(const QUrl &url)
{
    m_d->m_webView->setUrl(url);
}

void Html5ApplicationViewer::setOrientation(ScreenOrientation orientation)
{
#ifdef Q_OS_SYMBIAN
    if (orientation != ScreenOrientationAuto) {
#if defined(ORIENTATIONLOCK)
        const CAknAppUiBase::TAppUiOrientation uiOrientation =
                (orientation == ScreenOrientationLockPortrait) ? CAknAppUi::EAppUiOrientationPortrait
                    : CAknAppUi::EAppUiOrientationLandscape;
        CAknAppUi* appUi = dynamic_cast<CAknAppUi*> (CEikonEnv::Static()->AppUi());
        TRAPD(error,
            if (appUi)
                appUi->SetOrientationL(uiOrientation);
        );
        Q_UNUSED(error)
#else // ORIENTATIONLOCK
        qWarning("'ORIENTATIONLOCK' needs to be defined on Symbian when locking the orientation.");
#endif // ORIENTATIONLOCK
    }
#elif defined(Q_WS_MAEMO_5)
    Qt::WidgetAttribute attribute;
    switch (orientation) {
    case ScreenOrientationLockPortrait:
        attribute = Qt::WA_Maemo5PortraitOrientation;
        break;
    case ScreenOrientationLockLandscape:
        attribute = Qt::WA_Maemo5LandscapeOrientation;
        break;
    case ScreenOrientationAuto:
    default:
        attribute = Qt::WA_Maemo5AutoOrientation;
        break;
    }
    setAttribute(attribute, true);
#else // Q_OS_SYMBIAN
    Q_UNUSED(orientation);
#endif // Q_OS_SYMBIAN
}

void Html5ApplicationViewer::showExpanded()
{
#ifdef Q_OS_SYMBIAN
    showFullScreen();
#elif defined(Q_WS_MAEMO_5)
    showMaximized();
#else
    show();
#endif
}

QGraphicsWebView *Html5ApplicationViewer::webView() const
{
    return m_d->m_webView;
}

#include "html5applicationviewer.moc"
