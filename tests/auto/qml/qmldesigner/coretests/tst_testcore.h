/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef TESTCORE_H
#define TESTCORE_H

#include <QObject>

#include <QtTest>


class tst_TestCore : public QObject
{
    Q_OBJECT
public:
    tst_TestCore();

private slots:
    void initTestCase();
    void cleanupTestCase();

    //
    // unit tests MetaInfo, NodeMetaInfo, PropertyMetaInfo
    //
    void testMetaInfo();
    void testMetaInfoSimpleType();
    void testMetaInfoUncreatableType();
    void testMetaInfoExtendedType();
    void testMetaInfoInterface();
    void testMetaInfoCustomType();
    void testMetaInfoEnums();
    void testMetaInfoProperties();
    void testMetaInfoDotProperties();
    void testMetaInfoListProperties();

    //
    // unit tests Model, ModelNode, NodeProperty, AbstractView
    //
    void testModelCreateCoreModel();
    void testModelCreateRect();
    void testModelViewNotification();
    void testModelCreateSubNode();
    void testModelRemoveNode();
    void testModelRootNode();
    void testModelReorderSiblings();
    void testModelAddAndRemoveProperty();
    void testModelSliding();
    void testModelBindings();
    void testModelDynamicProperties();
    void testModelDefaultProperties();
    void testModelBasicOperations();
    void testModelResolveIds();
    void testModelNodeListProperty();
    void testModelPropertyValueTypes();
    void testModelNodeInHierarchy();
    void testModelNodeIsAncestorOf();

    //
    // unit tests Rewriter
    //
    void testRewriterView();
    void testRewriterErrors();
    void testRewriterChangeId();
    void testRewriterRemoveId();
    void testRewriterChangeValueProperty();
    void testRewriterRemoveValueProperty();
    void testRewriterSignalProperty();
    void testRewriterObjectTypeProperty();
    void testRewriterPropertyChanges();
    void testRewriterListModel();
    void testRewriterAddProperty();
    void testRewriterAddPropertyInNestedObject();
    void testRewriterAddObjectDefinition();
    void testRewriterAddStatesArray();
    void testRewriterRemoveStates();
    void testRewriterRemoveObjectDefinition();
    void testRewriterRemoveScriptBinding();
    void testRewriterNodeReparenting();
    void testRewriterNodeReparentingWithTransaction();
    void testRewriterMovingInOut();
    void testRewriterMovingInOutWithTransaction();
    void testRewriterComplexMovingInOut();
    void testRewriterAutomaticSemicolonAfterChangedProperty();
    void testRewriterTransaction();
    void testRewriterId();
    void testRewriterNodeReparentingTransaction1();
    void testRewriterNodeReparentingTransaction2();
    void testRewriterNodeReparentingTransaction3();
    void testRewriterNodeReparentingTransaction4();
    void testRewriterAddNodeTransaction();
    void testRewriterComponentId();
    void testRewriterPropertyDeclarations();
    void testRewriterPropertyAliases();
    void testRewriterPositionAndOffset();
    void testRewriterFirstDefinitionInside();
    void testRewriterComponentTextModifier();
    void testRewriterPreserveType();
    void testRewriterForArrayMagic();
    void testRewriterWithSignals();
    void testRewriterNodeSliding();
    void testRewriterExceptionHandling();
    void testRewriterDynamicProperties();
    void testRewriterGroupedProperties();
    void testRewriterPreserveOrder();
    void testRewriterActionCompression();
    void testRewriterImports();
    void testRewriterChangeImports();

    //
    // unit tests QmlModelNodeFacade/QmlModelState
    //

    void testQmlModelStates();
    void testQmlModelAddMultipleStates();
    void testQmlModelRemoveStates();
    void testQmlModelStatesInvalidForRemovedNodes();
    void testQmlModelStateWithName();

    //
    // unit tests Instances
    //
    void testInstancesAttachToExistingModel();
    void testInstancesStates();
    void testInstancesIdResolution();
    void testInstancesNotInScene();
    void testInstancesBindingsInStatesStress();
    void testInstancesPropertyChangeTargets();
    void testInstancesDeletePropertyChanges();
    void testInstancesChildrenLowLevel();
    void testInstancesResourcesLowLevel();
    void testInstancesFlickableLowLevel();
    void testInstancesReorderChildrenLowLevel();

    //
    // integration tests
    //
    void testBasicStates();
    void testStates();
    void testStatesBaseState();
    void testStatesRewriter();
    void testGradientsRewriter();
    void testTypicalRewriterOperations();
    void testBasicOperationsWithView();
    void testQmlModelView();
    void reparentingNode();
    void reparentingNodeInModificationGroup();
    void testRewriterTransactionRewriter();
    void testCopyModelRewriter1();
    void testCopyModelRewriter2();
    void testSubComponentManager();
    void testAnchorsAndRewriting();
    void testAnchorsAndRewritingCenter();

    //
    // regression tests
    //

    void reparentingNodeLikeDragAndDrop();
    void testRewriterForGradientMagic();

    //
    // old tests
    //

    void loadEmptyCoreModel();
    void saveEmptyCoreModel();
    void loadAttributesInCoreModel();
    void saveAttributesInCoreModel();
    void loadSubItems();


    void createInvalidCoreModel();

    void loadQml();

    void defaultPropertyValues();

    // Detailed tests for the rewriting:

    // Anchor tests:
    void loadAnchors();
    void changeAnchors();
    void anchorToSibling();
    void removeFillAnchorByDetaching();
    void removeFillAnchorByChanging();
    void removeCenteredInAnchorByDetaching();

    // Property bindings:
    void changePropertyBinding();

    // Bigger tests:
    void loadTestFiles();

    // Object bindings as properties:
    void loadGradient();
    void changeGradientId();
};

#endif // TESTCORE_H
