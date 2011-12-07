/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef PROPERTYEDITORCONTEXTOBJECT_H
#define PROPERTYEDITORCONTEXTOBJECT_H

#include <QObject>
#include <QUrl>
#include <QDeclarativePropertyMap>

namespace QmlDesigner {

class PropertyEditorContextObject : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QUrl globalBaseUrl READ globalBaseUrl WRITE setGlobalBaseUrl NOTIFY globalBaseUrlChanged)
    Q_PROPERTY(QUrl specificsUrl READ specificsUrl WRITE setSpecificsUrl NOTIFY specificsUrlChanged)

    Q_PROPERTY(QString specificQmlData READ specificQmlData WRITE setSpecificQmlData NOTIFY specificQmlDataChanged)
    Q_PROPERTY(QString stateName READ stateName WRITE setStateName NOTIFY stateNameChanged)

    Q_PROPERTY(bool isBaseState READ isBaseState WRITE setIsBaseState NOTIFY isBaseStateChanged)
    Q_PROPERTY(bool selectionChanged READ selectionChanged WRITE setSelectionChanged NOTIFY selectionChangedChanged)

    Q_PROPERTY(QDeclarativePropertyMap* backendValues READ backendValues WRITE setBackendValues NOTIFY backendValuesChanged)

public:
    PropertyEditorContextObject(QObject *parent = 0);

    QUrl globalBaseUrl() const {return m_globalBaseUrl; }
    QUrl specificsUrl() const {return m_specificsUrl; }
    QString specificQmlData() const {return m_specificQmlData; }
    QString stateName() const {return m_stateName; }

    bool isBaseState() const { return m_isBaseState; }
    bool selectionChanged() const { return m_selectionChanged; }

    QDeclarativePropertyMap* backendValues() const { return m_backendValues; }

signals:
    void globalBaseUrlChanged();
    void specificsUrlChanged();
    void specificQmlDataChanged();
    void stateNameChanged();
    void isBaseStateChanged();
    void selectionChangedChanged();
    void backendValuesChanged();

public slots:
     void setGlobalBaseUrl(const QUrl &newBaseUrl)
     {
         if (newBaseUrl == m_globalBaseUrl)
             return;

         m_globalBaseUrl = newBaseUrl;
         emit globalBaseUrlChanged();
     }

     void setSpecificsUrl(const QUrl &newSpecificsUrl)
     {
         if (newSpecificsUrl == m_specificsUrl)
             return;

         m_specificsUrl = newSpecificsUrl;
         emit specificsUrlChanged();
     }

     void setSpecificQmlData(const QString &newSpecificQmlData)
     {
         if (m_specificQmlData == newSpecificQmlData)
             return;

         m_specificQmlData = newSpecificQmlData;
         emit specificQmlDataChanged();
     }

     void setStateName(const QString &newStateName)
     {
         if (newStateName == m_stateName)
             return;

         m_stateName = newStateName;
         emit stateNameChanged();
     }

     void setIsBaseState(bool newIsBaseState)
     {
         if (newIsBaseState ==  m_isBaseState)
             return;

         m_isBaseState = newIsBaseState;
         emit isBaseStateChanged();
     }

     void setSelectionChanged(bool newSelectionChanged)
     {
         if (newSelectionChanged ==  m_selectionChanged)
             return;

         m_selectionChanged = newSelectionChanged;
         emit selectionChangedChanged();
     }

     void setBackendValues(QDeclarativePropertyMap* newBackendValues)
     {
         if (newBackendValues ==  m_backendValues)
             return;

         m_backendValues = newBackendValues;
         emit backendValuesChanged();
     }

    void triggerSelectionChanged()
    {
        setSelectionChanged(!m_selectionChanged);
    }

private:
    QUrl m_globalBaseUrl;
    QUrl m_specificsUrl;

    QString m_specificQmlData;
    QString m_stateName;

    bool m_isBaseState;
    bool m_selectionChanged;

    QDeclarativePropertyMap* m_backendValues;

};

} //QmlDesigner {

#endif // PROPERTYEDITORCONTEXTOBJECT_H
