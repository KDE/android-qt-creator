/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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

#include "easingcontextpane.h"
#include "ui_easingcontextpane.h"
#include <qmljs/qmljspropertyreader.h>

#include <QtGui/QGraphicsPixmapItem>
#include <QtGui/QGraphicsScene>
#include <QtCore/QPropertyAnimation>
#include <QtCore/QSequentialAnimationGroup>

namespace QmlEditorWidgets {

class PixmapItem : public QObject, public QGraphicsPixmapItem
{
    Q_OBJECT
    Q_PROPERTY(QPointF pos READ pos WRITE setPos)
public:
    PixmapItem(const QPixmap &pix) : QGraphicsPixmapItem(pix) { }
};

class EasingSimulation : public QObject
{
    Q_OBJECT
public:
    QGraphicsView *m_g;
    EasingSimulation(QObject *parent=0, QGraphicsView *v=0):QObject(parent) {
        m_qtLogo = new PixmapItem(QPixmap(":/qt_logo.png"));
        m_scene.addItem(m_qtLogo);
        m_scene.setSceneRect(0,0,v->viewport()->width(),m_qtLogo->boundingRect().height());
        m_qtLogo->hide();
        m_sequential = 0;
        m_g = v;
        m_g->setScene(&m_scene);
    }

    ~EasingSimulation() { delete m_qtLogo; }

    QGraphicsScene *scene() { return &m_scene; }
    void show() { m_qtLogo->show(); }
    void hide() { m_qtLogo->hide(); }
    void reset() {
        m_qtLogo->setPos(0,0);
        m_scene.setSceneRect(0,0,m_g->viewport()->width(),m_qtLogo->boundingRect().height());
    }
    void stop() {
        if (m_sequential) {
            m_sequential->stop();
            reset();
            emit finished();
        }
    }
    bool running() {
        if (m_sequential)
            return m_sequential->state()==QAbstractAnimation::Running;
        else
            return false;
    }
    void updateCurve(QEasingCurve newCurve, int newDuration) {
        if (running()) {
            m_sequential->pause();
            static_cast<QPropertyAnimation *>(m_sequential->animationAt(1))->setEasingCurve(newCurve);
            static_cast<QPropertyAnimation *>(m_sequential->animationAt(1))->setDuration(newDuration);
            m_sequential->resume();
        }
    }
    void animate(int duration, QEasingCurve curve) {
        reset();
        m_sequential = new QSequentialAnimationGroup;
        connect(m_sequential,SIGNAL(finished()),this,SIGNAL(finished()));
        m_sequential->addPause(150);
        QPropertyAnimation *m_anim = new QPropertyAnimation (m_qtLogo, "pos");
        m_anim->setStartValue(QPointF(0, 0));
        m_anim->setEndValue(QPointF(m_scene.sceneRect().width() - m_qtLogo->boundingRect().width(), 0));
        m_anim->setDuration(duration);
        m_anim->setEasingCurve(curve);
        m_sequential->addAnimation(m_anim);
        m_sequential->addPause(300);
        m_sequential->start();
    }

signals:
    void finished();

private:
    PixmapItem *m_qtLogo;
    QGraphicsScene m_scene;
    QSequentialAnimationGroup *m_sequential;
};


EasingContextPane::EasingContextPane(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::EasingContextPane)
{
    ui->setupUi(this);

    m_simulation = new EasingSimulation(this,ui->graphicsView);

    m_easingGraph = new EasingGraph(this);
    m_easingGraph->raise();
    setLinear();

    ui->playButton->setIcon(QIcon(QLatin1String(":/playicon.png")));



    setGraphDisplayMode(GraphMode);

    connect(m_simulation,SIGNAL(finished()),this,SLOT(switchToGraph()));
}

EasingContextPane::~EasingContextPane()
{
    delete ui;
}


bool EasingContextPane::acceptsType(const QStringList &types)
{
    return types.contains("NumberAnimation") ||
            types.contains("PropertyAnimation") ||
            types.contains("ColorAnimation") ||
            types.contains("RotationAnimation");
}

void EasingContextPane::setProperties(QmlJS::PropertyReader *propertyReader)
{
    m_easingGraph->setGeometry(ui->graphicsView->geometry().adjusted(2,2,-2,-2));
    QString newEasingType = QString("Linear");
    if (propertyReader->hasProperty(QLatin1String("easing.type"))) {
        newEasingType = propertyReader->readProperty(QLatin1String("easing.type")).toString();
        if (newEasingType.contains("."))
            newEasingType = newEasingType.right(newEasingType.length() - newEasingType.indexOf(".") - 1);
    }

    m_easingGraph->setEasingName(newEasingType);
    ui->easingShapeComboBox->setCurrentIndex(ui->easingShapeComboBox->findText(m_easingGraph->easingShape()));
    ui->easingExtremesComboBox->setCurrentIndex(ui->easingExtremesComboBox->findText(m_easingGraph->easingExtremes()));


    if (propertyReader->hasProperty(QLatin1String("easing.period"))) {
        qreal period = propertyReader->readProperty(QLatin1String("easing.period")).toDouble();
        if (period < ui->periodSpinBox->minimum() || period > ui->periodSpinBox->maximum())
            ui->periodSpinBox->setValue(ui->periodSpinBox->minimum());
        else
            ui->periodSpinBox->setValue(period);
    }
    else
        ui->periodSpinBox->setValue(0.3);

    if (propertyReader->hasProperty(QLatin1String("easing.amplitude"))) {
        qreal amplitude = propertyReader->readProperty(QLatin1String("easing.amplitude")).toDouble();
        if (amplitude < ui->amplitudeSpinBox->minimum() || amplitude > ui->amplitudeSpinBox->maximum())
            ui->amplitudeSpinBox->setValue(ui->amplitudeSpinBox->minimum());
        else
            ui->amplitudeSpinBox->setValue(amplitude);
    }
    else
        ui->amplitudeSpinBox->setValue(1.0);

    if (propertyReader->hasProperty(QLatin1String("easing.overshoot"))) {
        qreal overshoot = propertyReader->readProperty(QLatin1String("easing.overshoot")).toDouble();
        if (overshoot < ui->overshootSpinBox->minimum() || overshoot > ui->overshootSpinBox->maximum())
            ui->overshootSpinBox->setValue(ui->overshootSpinBox->minimum());
        else
            ui->overshootSpinBox->setValue(overshoot);
    }
    else
        ui->overshootSpinBox->setValue(1.70158);

    if (propertyReader->hasProperty(QLatin1String("duration"))) {
        qreal duration = propertyReader->readProperty(QLatin1String("duration")).toInt();
        if (duration < ui->durationSpinBox->minimum() || duration > ui->durationSpinBox->maximum())
            ui->durationSpinBox->setValue(ui->durationSpinBox->minimum());
        else
            ui->durationSpinBox->setValue(duration);
    }
    else
        ui->durationSpinBox->setValue(250);
}

void EasingContextPane::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void EasingContextPane::setGraphDisplayMode(GraphDisplayMode newMode)
{
    m_displayMode = newMode;
    switch (newMode) {
    case GraphMode: {
            m_simulation->hide();
            m_easingGraph->show();
            break;
        }
    case SimulationMode: {
            m_simulation->show();
            m_easingGraph->hide();
            break;
        }
    default: break;
    }

}

void EasingContextPane::startAnimation()
{
    if (m_simulation->running())
        m_simulation->stop();
    else {
        m_simulation->animate(ui->durationSpinBox->value(), m_easingGraph->easingCurve());
        ui->playButton->setIcon(QIcon(QLatin1String(":/stopicon.png")));
    }

}

void EasingContextPane::switchToGraph()
{
    ui->playButton->setIcon(QIcon(QLatin1String(":/playicon.png")));
    setGraphDisplayMode(GraphMode);
}

void EasingContextPane::setOthers()
{
    ui->easingExtremesComboBox->setEnabled(true);
    ui->amplitudeSpinBox->setEnabled(false);
    ui->overshootSpinBox->setEnabled(false);
    ui->overshootSpinBox->setEnabled(false);
    ui->periodSpinBox->setEnabled(false);
}

void EasingContextPane::setLinear()
{
    ui->easingExtremesComboBox->setEnabled(false);
    ui->amplitudeSpinBox->setEnabled(false);
    ui->overshootSpinBox->setEnabled(false);
    ui->periodSpinBox->setEnabled(false);
}

void EasingContextPane::setBack()
{
    ui->easingExtremesComboBox->setEnabled(true);
    ui->amplitudeSpinBox->setEnabled(false);
    ui->overshootSpinBox->setEnabled(true);
    ui->periodSpinBox->setEnabled(false);
}

void EasingContextPane::setElastic()
{
    ui->easingExtremesComboBox->setEnabled(true);
    ui->amplitudeSpinBox->setEnabled(true);
    ui->overshootSpinBox->setEnabled(false);
    ui->periodSpinBox->setEnabled(true);
}

void EasingContextPane::setBounce()
{
    ui->easingExtremesComboBox->setEnabled(true);
    ui->amplitudeSpinBox->setEnabled(true);
    ui->overshootSpinBox->setEnabled(false);
    ui->periodSpinBox->setEnabled(false);
}

} //QmlDesigner

void QmlEditorWidgets::EasingContextPane::on_durationSpinBox_valueChanged(int newValue)
{
    m_simulation->updateCurve(m_easingGraph->easingCurve(),ui->durationSpinBox->value());
    emit propertyChanged(QLatin1String("duration"), newValue);
}

void QmlEditorWidgets::EasingContextPane::on_easingShapeComboBox_currentIndexChanged(QString newShape)
{
    if (newShape=="Linear")
        setLinear();
    else if (newShape=="Bounce")
        setBounce();
    else if (newShape=="Elastic")
        setElastic();
    else if (newShape=="Back")
        setBack();
    else
        setOthers();

    if (m_easingGraph->easingShape() != newShape) {
        m_easingGraph->setEasingShape(newShape);
        // reload easing parameters
        m_easingGraph->setAmplitude(ui->amplitudeSpinBox->value());
        m_easingGraph->setPeriod(ui->periodSpinBox->value());
        m_easingGraph->setOvershoot(ui->overshootSpinBox->value());
        m_simulation->updateCurve(m_easingGraph->easingCurve(),ui->durationSpinBox->value());
        emit propertyChanged(QLatin1String("easing.type"), QVariant(QLatin1String("Easing.")+m_easingGraph->easingName()));
    }
}

void QmlEditorWidgets::EasingContextPane::on_easingExtremesComboBox_currentIndexChanged(QString newExtremes)
{
    if (m_easingGraph->easingExtremes() != newExtremes) {
        m_easingGraph->setEasingExtremes(newExtremes);
        m_easingGraph->setAmplitude(ui->amplitudeSpinBox->value());
        m_easingGraph->setPeriod(ui->periodSpinBox->value());
        m_easingGraph->setOvershoot(ui->overshootSpinBox->value());
        m_simulation->updateCurve(m_easingGraph->easingCurve(),ui->durationSpinBox->value());
        emit propertyChanged(QLatin1String("easing.type"), QVariant(QLatin1String("Easing.")+m_easingGraph->easingName()));
    }
}

void QmlEditorWidgets::EasingContextPane::on_amplitudeSpinBox_valueChanged(double newAmplitude)
{
    if ((newAmplitude != m_easingGraph->amplitude()) &&
        (m_easingGraph->easingShape()=="Bounce" || m_easingGraph->easingShape()=="Elastic")) {
        m_easingGraph->setAmplitude(newAmplitude);
        m_simulation->updateCurve(m_easingGraph->easingCurve(),ui->durationSpinBox->value());
        emit propertyChanged(QLatin1String("easing.amplitude"), newAmplitude);
    }
}

void QmlEditorWidgets::EasingContextPane::on_periodSpinBox_valueChanged(double newPeriod)
{
    if ((newPeriod != m_easingGraph->period()) && (m_easingGraph->easingShape()=="Elastic")) {
        m_easingGraph->setPeriod(newPeriod);
        m_simulation->updateCurve(m_easingGraph->easingCurve(),ui->durationSpinBox->value());
        emit propertyChanged(QLatin1String("easing.period"), newPeriod);
    }

}

void QmlEditorWidgets::EasingContextPane::on_overshootSpinBox_valueChanged(double newOvershoot)
{
    if ((newOvershoot != m_easingGraph->overshoot()) && (m_easingGraph->easingShape()=="Back")) {
        m_easingGraph->setOvershoot(newOvershoot);
        m_simulation->updateCurve(m_easingGraph->easingCurve(),ui->durationSpinBox->value());
        emit propertyChanged(QLatin1String("easing.overshoot"), newOvershoot);
    }
}

void QmlEditorWidgets::EasingContextPane::on_playButton_clicked()
{
    setGraphDisplayMode(SimulationMode);
    startAnimation();
}

#include "easingcontextpane.moc"
