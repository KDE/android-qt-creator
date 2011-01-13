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

#include "parsemanager.h"
#include "cplusplus/CppDocument.h"
#include "Control.h"
#include "Literals.h"
#include "Overview.h"
#include "Scope.h"
#include "TranslationUnit.h"
#include "AST.h"
#include "Symbols.h"
#include "Bind.h"
#include <QDebug>
#include "Name.h"
#include "cpptools/cppmodelmanager.h"
#include <QTextStream>

using namespace CppTools;
using namespace CppTools::Internal;


using namespace CPlusPlus;

//<------------------------------------------------------- Compare function for the internal structures
/**********************************
Compares function with function
with return type, function name
and their arguments and arguments
types.
**********************************/
bool FUNCTIONITEM::isEqualTo(FUNCTIONITEM *cpfct, bool ignoreName/* = true*/)
{
    if(ignoreName)
        return function->isEqualTo(cpfct->function, true);
    return function->isEqualTo(cpfct->function);
}

/*****************************************************************
Compares two property regarding
of their function definition,
type definition, function arguments
and function types.

Q_PROPERTY( ConnectionState state READ state NOTIFY stateChanged);
******************************************************************/
bool PROPERTYITEM::isEqualTo(PROPERTYITEM *cpppt)
{
    if (!ast->type_id || !cpppt->ast->type_id)
        return false;
    if (type != cpppt->type)
        return false;

    if (!ast->property_name || !ast->property_name->name || !ast->property_name->name->identifier())
        return false;
    QString thistypename = ast->property_name->name->identifier()->chars();

    if (!cpppt->ast->property_name || !cpppt->ast->property_name->name || !cpppt->ast->property_name->name->identifier())
        return false;
    QString cppttypename = cpppt->ast->property_name->name->identifier()->chars();
    if(thistypename != cppttypename)
        return false;

    if ((this->readAst == 0) != (cpppt->readAst == 0))
        return false;
    if((this->writeAst == 0) != (cpppt->writeAst == 0))
        return false;
    if((this->resetAst == 0) != (cpppt->resetAst == 0))
        return false;
    if((this->notifyAst == 0) != (cpppt->notifyAst == 0))
        return false;
    //check for read function
    if(this->readAst){
        if(!this->readFct || !cpppt->readFct)
            return false;
        if(!this->readFct->isEqualTo(cpppt->readFct))
            return false;
    }
    //check for write function
    if(this->writeAst){
        if(!this->writeFct || !cpppt->writeFct)
            return false;
        if(!this->writeFct->isEqualTo(cpppt->writeFct))
            return false;
    }
    //check for reset function
    if(this->resetAst){
        if(!this->resetFct || !cpppt->resetFct)
            return false;
        if(!this->resetFct->isEqualTo(cpppt->resetFct))
            return false;
    }
    //check for notify function
    if(this->notifyAst){
        if(!this->notifyFct || !cpppt->notifyFct)
            return false;
        if(!this->notifyFct->isEqualTo(cpppt->notifyFct))
            return false;
    }
    return true;
}

/*****************************************************************
Compares two enums regarding
of their values created by the getEnumValueStringList function.
*****************************************************************/
bool QENUMITEM::isEqualTo(QENUMITEM *cpenum)
{
    if(this->values.count() != cpenum->values.count())
        return false;
    foreach(QString str, this->values){
        if(!cpenum->values.contains(str))
            return false;
    }
    return true;
}

/*****************************************************************
Compares two flags regarding
of their enum definitions and their
values created by the getEnumValueStringList function.
*****************************************************************/
bool QFLAGITEM::isEqualTo(QFLAGITEM *cpflag)
{
    if(this->enumvalues.count() != cpflag->enumvalues.count())
        return false;
    foreach(QString str, this->enumvalues){
        if(!cpflag->enumvalues.contains(str))
            return false;
    }
    return true;
}



ParseManager::ParseManager()
: pCppPreprocessor(0)
{

}

ParseManager::~ParseManager()
{
    if(pCppPreprocessor)
        delete pCppPreprocessor;
    if(::m_resultFile){
        ::m_resultFile->close();
        delete ::m_resultFile;
        ::m_resultFile = 0;
    }
}

/**************************************
Function for setting the include
Paths
**************************************/
void ParseManager::setIncludePath(const QStringList &includePath)
{
    m_includePaths = includePath;
}

/**************************************
public Function that starts the parsing
all of the files in the sourceFiles
string list.
**************************************/
void ParseManager::parse(const QStringList &sourceFiles)
{
    m_errormsgs.clear();
    if(pCppPreprocessor){
        delete pCppPreprocessor;
        pCppPreprocessor = 0;
    }

    if (! sourceFiles.isEmpty()) {
        m_strHeaderFile = sourceFiles[0];
        pCppPreprocessor = new CppTools::Internal::CppPreprocessor(QPointer<CPlusPlus::ParseManager>(this));
        pCppPreprocessor->setIncludePaths(m_includePaths);
        pCppPreprocessor->setFrameworkPaths(m_frameworkPaths);
        parse(pCppPreprocessor, sourceFiles);
    }
}

/*********************************************
private function that prepare the filelist
to parse and starts the parser.
*********************************************/
void ParseManager::parse(CppTools::Internal::CppPreprocessor *preproc,
                            const QStringList &files)
{
    if (files.isEmpty())
        return;

    //check if file is C++ header file
    QStringList headers;
    foreach (const QString &file, files) {
        const QFileInfo fileInfo(file);
        QString ext = fileInfo.suffix();
        if (ext.toLower() == "h")
            headers.append(file);
    }

    foreach (const QString &file, files) {
        preproc->snapshot.remove(file);
    }
    preproc->setTodo(headers);
    QString conf = QLatin1String("<configuration>");

    preproc->run(conf);
    for (int i = 0; i < headers.size(); ++i) {
        QString fileName = headers.at(i);
        preproc->run(fileName);
    }
}

//This function creates a class list for each class and its base classes in
//the header file that needs to be checked.
//e.g.
//      Cl1          Cl2
//     __|__        __|__
//    |     |      |     |
//   Cl11  Cl12   Cl21  Cl22
//
//==> list[0] = {Cl1, Cl11, Cl12}
//    list[1] = {Cl2, Cl21, Cl22}

QList<CLASSTREE*> ParseManager::CreateClassLists(bool isInterfaceHeader)
{
    QList<CLASSTREE*>ret;
    QList<CLASSLISTITEM*> classlist;
    QList<CLASSLISTITEM*> allclasslist;

    Trace("Following classes scaned for header file: " + m_strHeaderFile);
    //Iteration over all parsed documents
    if(getPreProcessor()){
        for (Snapshot::const_iterator it = getPreProcessor()->snapshot.begin()
            ; it != getPreProcessor()->snapshot.end(); ++it)
        {
            Document::Ptr doc = (*it);
            if(doc){
                QFileInfo fileinf(doc->fileName());
                QFileInfo fileinf1(m_strHeaderFile);
                //Get the Translated unit
                Control* ctrl = doc->control();
                TranslationUnit* trlUnit = ctrl->translationUnit();
                AST* pAst = trlUnit->ast();
                TranslationUnitAST *ptrAst = 0;
                if(pAst && (ptrAst = pAst->asTranslationUnit())){
                    //iteration over all translated declaration in this document
                    for (DeclarationListAST *pDecllist = ptrAst->declaration_list; pDecllist; pDecllist = pDecllist->next) {
                        if(pDecllist->value){
                            SimpleDeclarationAST *pSimpleDec = pDecllist->value->asSimpleDeclaration();
                            if(pSimpleDec){
                                //Iteration over class specifier
                                for (SpecifierListAST *pSimpleDecDecllist = pSimpleDec->decl_specifier_list; pSimpleDecDecllist; pSimpleDecDecllist = pSimpleDecDecllist->next) {
                                    ClassSpecifierAST * pclassspec = pSimpleDecDecllist->value->asClassSpecifier();
                                    if(pclassspec){
                                        CLASSLISTITEM* item = new CLASSLISTITEM();
                                        item->classspec = pclassspec;
                                        item->trlUnit = trlUnit;
                                        allclasslist.push_back(item);
                                        QString classname = item->trlUnit->spell(item->classspec->name->firstToken());
                                        Trace("- " + classname + " class scaned");

                                        //We found a class that is defined in the header file that needs to be checked
                                        if(fileinf.fileName().toLower() == fileinf1.fileName().toLower()){
                                            CLASSTREE* cltree = new CLASSTREE();
                                            cltree->highestlevelclass = item;
                                            cltree->classlist.push_back(item);
                                            ret.push_back(cltree);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    //after we search for the classes we need to search for the baseclasses
    Trace("Following classes found in Header file: " + m_strHeaderFile);
    foreach(CLASSTREE *cltree, ret){
        QString classname = cltree->highestlevelclass->trlUnit->spell(cltree->highestlevelclass->classspec->name->firstToken());
        Trace("- " + classname + " class found");
        QList<CLASSLISTITEM*> baseclasslist;
        getBaseClasses(cltree->highestlevelclass, baseclasslist, allclasslist, 0, isInterfaceHeader);
        cltree->classlist.append(baseclasslist);
    }
    return ret;
}

/********************************************
Gets all the baseclass from a class and
add those base classes into the baseclasslist
********************************************/
void ParseManager::getBaseClasses(const CLASSLISTITEM* pclass
                                  , QList<CLASSLISTITEM*> &baseclasslist
                                  , const QList<CLASSLISTITEM*> &allclasslist
                                  , int level
                                  , bool isInterfaceHeader)
{
    //iteration over the base_clause_list of the current class
    QString levelmarker = "  ";
    for(int i = 0; i < level; i++)
        levelmarker += "   ";
    levelmarker += "|- ";
    QList<CLASSLISTITEM*>child;

    for(BaseSpecifierListAST *pBaseSpecList = pclass->classspec->base_clause_list; pBaseSpecList; pBaseSpecList = pBaseSpecList->next)
    {
        BaseSpecifierAST *pBaseSpec = pBaseSpecList->value;
        bool found = false;
        foreach(CLASSLISTITEM* pclspec, allclasslist)
        {
            if(pclspec->classspec->symbol->name()
                && pBaseSpec->symbol->name()
                && pclspec->classspec->symbol->name()->isEqualTo(pBaseSpec->symbol->name()))
            {
                child.push_back(pclspec);
                baseclasslist.push_back(pclspec);
                QString classname = pclspec->trlUnit->spell(pclspec->classspec->name->firstToken());
                Trace(levelmarker + classname + " class found");
                found = true;
                break;
            }
        }
        if(!found && pBaseSpec->name){
            QString classname = pclass->trlUnit->spell(pBaseSpec->name->firstToken());
            if(isInterfaceHeader)
                Trace(levelmarker + classname + " class not found! Interface classes should not be inherited from Qt Objects!");
            else
                Trace(levelmarker + classname + " class not found!");
        }
    }
    //call the function recursive because all the basclasses can have other base classes
    foreach(CLASSLISTITEM* pchclass, child){
        getBaseClasses(pchclass, baseclasslist, allclasslist, ++level, isInterfaceHeader);
    }
}

/**************************************************
This function finds and creates all Elements wich
are significant for MetaDatas.
Those element will be added in the aparameter
lists.
**************************************************/
void ParseManager::getElements(QList<FUNCTIONITEM*> &functionlist
                               , QList<PROPERTYITEM*> &propertylist
                               , QList<QENUMITEM*> &qenumlist
                               , QList<ENUMITEM*> &enumlist
                               , QList<QFLAGITEM*> &qflaglist
                               , QList<QDECLAREFLAGSITEM*> &qdeclareflaglist
                               , const QList<CLASSLISTITEM*> classitems
                               , const CLASSLISTITEM* highestlevelclass)
{
    foreach(CLASSLISTITEM* classitem, classitems){
        QString classname = "";
        if(classitem->classspec->name)
            classname = classitem->trlUnit->spell(classitem->classspec->name->firstToken());
        for (DeclarationListAST *pmemberlist = classitem->classspec->member_specifier_list; pmemberlist; pmemberlist = pmemberlist->next) {
            /**********
            Functions
            **********/
            if (FunctionDefinitionAST *pfctdef = pmemberlist->value->asFunctionDefinition()){
                FUNCTIONITEM* item = new FUNCTIONITEM;
                item->trlUnit = classitem->trlUnit;
                item->function = pfctdef->symbol;
                item->classAst = classitem->classspec;
                item->highestlevelclass = highestlevelclass;
                functionlist.push_back(item);
                if(isMetaObjFunction(item))
                    Trace("  - " + getTraceFuntionString(item, classname) + " found");
            }

            SimpleDeclarationAST *pdecl = pmemberlist->value->asSimpleDeclaration();
            if(pdecl){
                for(List<Symbol*>* decllist = pdecl->symbols; decllist; decllist = decllist->next)
                {
                    Function* pfct = decllist->value->type()->asFunctionType();
                    if(pfct){
                        FUNCTIONITEM* item = new FUNCTIONITEM();
                        item->trlUnit = classitem->trlUnit;
                        item->function = pfct;
                        item->classAst = classitem->classspec;
                        item->highestlevelclass = highestlevelclass;
                        functionlist.push_back(item);
                        if(isMetaObjFunction(item))
                            Trace("  - " + getTraceFuntionString(item, classname) + " found");
                    }
                }
                /******
                enum
                ******/
                for(List<SpecifierAST*>* decllist = pdecl->decl_specifier_list; decllist; decllist = decllist->next)
                {
                    EnumSpecifierAST * penum = decllist->value->asEnumSpecifier();
                    if(penum){
                        ENUMITEM* item = new ENUMITEM();
                        item->ast = penum;
                        item->highestlevelclass = highestlevelclass;
                        item->trlUnit = classitem->trlUnit;
                        enumlist.push_back(item);
                    }
                }
            }
            else{
                /**********
                Q_PROPERTY
                **********/
                if(QtPropertyDeclarationAST *ppdecl = pmemberlist->value->asQtPropertyDeclaration()) {
                    propertylist.push_back(PROPERTYITEM::create(ppdecl, highestlevelclass));
                    if (ppdecl->property_name && ppdecl->property_name->name && ppdecl->property_name->name->identifier()) {
                        Trace("  - Q_PROPERTY: " + QLatin1String(ppdecl->property_name->name->identifier()->chars()) + " found");
                    }
                } else{
                    /**********
                    Q_ENUM
                    **********/
                    if (QtEnumDeclarationAST *pqenum = pmemberlist->value->asQtEnumDeclaration()) {
                        for (NameListAST *plist = pqenum->enumerator_list; plist; plist = plist->next) {
                            QENUMITEM* item = new QENUMITEM;
                            item->name = plist->value->name->identifier()->chars();
                            item->highestlevelclass = highestlevelclass;
                            qenumlist.push_back(item);
                            Trace("  - Q_ENUM: " + classname + "::" + item->name + " found");
                        }
                    }
                    else{
                        /**********
                        Q_FLAGS
                        **********/
                        if (QtFlagsDeclarationAST *pqflags = pmemberlist->value->asQtFlagsDeclaration()){
                            for (NameListAST *plist = pqflags->flag_enums_list; plist; plist = plist->next) {
                                QFLAGITEM* item = new QFLAGITEM;
                                item->name = plist->value->name;
                                item->highestlevelclass = highestlevelclass;
                                qflaglist.push_back(item);
                                Trace("  - Q_FLAGS: " + classname + "::" + item->name->identifier()->chars() + " found");
                            }
                        }
#if 0
                        /*The code for Q_DECLARE_FLAGS was wrong. It's optional, and only does a typedef.
                        That means, if you do the typedef yourself and not use Q_DECLARE_FLAGS, that *is* valid.
                        Meaning, if one would want to do a check like the ones in this app, one has to check the defined types in the class scope.*/
                        else {
                            /****************
                            Q_DECLARE_FLAGS
                            ****************/
                            QtDeclareFlagsDeclarationAST *pqdeclflags = pmemberlist->value->asQDeclareFlagsDeclarationAST();
                            if(pqdeclflags){
                                QDECLAREFLAGSITEM* item = new QDECLAREFLAGSITEM();
                                item->ast = pqdeclflags;
                                item->highestlevelclass = highestlevelclass;
                                item->trlUnit = classitem->trlUnit;
                                qdeclareflaglist.push_back(item);
                            }
                        }
#endif
                    }
                }
            }
        }
    }
}

/*********************************************
Function that starts the comare between the
parser result and their metadata content.
*********************************************/
bool ParseManager::checkAllMetadatas(ParseManager* pInterfaceParserManager, QString resultfile)
{
    bool ret = true;

    //Create output file
    if(resultfile != "" && ::m_resultFile == 0){
        ::m_resultFile = new QFile(resultfile);
        if (!::m_resultFile->open(QFile::WriteOnly | QFile::Truncate)) {
            delete ::m_resultFile;
            ::m_resultFile = 0;
         }
    }

    /************************************************
    Get all elements from the interface header file
    ************************************************/
    Trace("### Get all elements from the interface header file ###");
    QList<CLASSTREE*> ilookuplist = pInterfaceParserManager->CreateClassLists(true);
    QList<QList<FUNCTIONITEM*> > ifunctionslookuplist;
    QList<QList<PROPERTYITEM*> > ipropertieslookuplist;
    QList<QList<QENUMITEM*> > iqenumlookuplist;
    QList<QList<ENUMITEM*> > ienumlookuplist;
    QList<QList<QFLAGITEM*> > iqflaglookuplist;
    QList<QList<QDECLAREFLAGSITEM*> > iqdeclareflaglookuplist;
    Trace("Following MetaData found:");
    foreach(CLASSTREE* iclasstree, ilookuplist){
        QList<FUNCTIONITEM*>functionlist;
        QList<PROPERTYITEM*>propertylist;
        QList<QENUMITEM*>qenumlist;
        QList<ENUMITEM*>enumlist;
        QList<QFLAGITEM*> qflaglist;
        QList<QDECLAREFLAGSITEM*> qdeclareflag;
        getElements(functionlist
            , propertylist
            , qenumlist
            , enumlist
            , qflaglist
            , qdeclareflag
            , iclasstree->classlist
            , iclasstree->highestlevelclass);
        if(functionlist.size() > 0)
            ifunctionslookuplist.append(functionlist);
        if(propertylist.size() > 0)
            ipropertieslookuplist.append(propertylist);
        if(qenumlist.size() > 0)
            iqenumlookuplist.append(qenumlist);
        if(enumlist.size() > 0)
            ienumlookuplist.append(enumlist);
        if(qflaglist.size() > 0)
            iqflaglookuplist.append(qflaglist);
        if(qdeclareflag.size() > 0)
            iqdeclareflaglookuplist.append(qdeclareflag);
    }

    /************************************************
    Get all elements from the compare header file
    ************************************************/
    Trace("\n");
    Trace("### Get all elements from the compare header file ###");
    QList<CLASSTREE*> lookuplist = CreateClassLists(false);
    QList<QList<FUNCTIONITEM*> > functionslookuplist;
    QList<QList<PROPERTYITEM*> > propertieslookuplist;
    QList<QList<QENUMITEM*> > qenumlookuplist;
    QList<QList<ENUMITEM*> > enumlookuplist;
    QList<QList<QFLAGITEM*> > qflaglookuplist;
    QList<QList<QDECLAREFLAGSITEM*> > qdeclareflaglookuplist;
    Trace("Following MetaData found:");
    foreach(CLASSTREE* classtree, lookuplist){
        QList<FUNCTIONITEM*>functionlist;
        QList<PROPERTYITEM*>propertylist;
        QList<QENUMITEM*>qenumlist;
        QList<ENUMITEM*>enumlist;
        QList<QFLAGITEM*> qflaglist;
        QList<QDECLAREFLAGSITEM*> qdeclareflag;
        getElements(functionlist
            , propertylist
            , qenumlist
            , enumlist
            , qflaglist
            , qdeclareflag
            , classtree->classlist
            , classtree->highestlevelclass);
        if(functionlist.size() > 0)
            functionslookuplist.append(functionlist);
        if(propertylist.size() > 0)
            propertieslookuplist.append(propertylist);
        if(qenumlist.size() > 0)
            qenumlookuplist.append(qenumlist);
        if(enumlist.size() > 0)
            enumlookuplist.append(enumlist);
        if(qflaglist.size() > 0)
            qflaglookuplist.append(qflaglist);
        if(qdeclareflag.size() > 0)
            qdeclareflaglookuplist.append(qdeclareflag);
    }

    Trace("\n");
    Trace("### Result: ###");
    /******************************
    Check for function
    ******************************/
    Trace("Compare all interface MetaData functions:");
    QList<FUNCTIONITEM*> missingifcts = checkMetadataFunctions(functionslookuplist, ifunctionslookuplist);
    if(missingifcts.size() > 0){
        foreach(FUNCTIONITEM* ifct, missingifcts){
            m_errormsgs.append(getErrorMessage(ifct));
        }
        ret =  false;
        Trace("- Failed!");
    }
    else{
        Trace("- OK");
    }

    /******************************
    Check for properies
    ******************************/
    Trace("Compare all interface MetaData properties:");
    QList<PROPERTYITEM*> missingippts = checkMetadataProperties(propertieslookuplist, functionslookuplist, ipropertieslookuplist, ifunctionslookuplist);
    if(missingippts.size() > 0){
        foreach(PROPERTYITEM* ippt, missingippts){
            m_errormsgs.append(getErrorMessage(ippt));
        }
        ret =  false;
        Trace("- Failed!");
    }
    else{
        Trace("- OK");
    }

    /******************************
    Check for enums
    ******************************/
    Trace("Compare all interface MetaData enums:");
    QList<QENUMITEM*> missingiqenums = checkMetadataEnums(qenumlookuplist, enumlookuplist, iqenumlookuplist, ienumlookuplist);
    if(missingiqenums.size() > 0){
        foreach(QENUMITEM* ienum, missingiqenums){
            m_errormsgs.append(getErrorMessage(ienum));
        }
        ret =  false;
        Trace("- Failed!");
    }
    else{
        Trace("- OK");
    }

    /******************************
    Check for flags
    ******************************/
    Trace("Compare all interface MetaData flags:");
    QList<QFLAGITEM*> missingiqflags = checkMetadataFlags(qflaglookuplist, qdeclareflaglookuplist, enumlookuplist
                                                        , iqflaglookuplist, iqdeclareflaglookuplist, ienumlookuplist);
    if(missingiqflags.size() > 0){
        foreach(QFLAGITEM* iflags, missingiqflags){
            m_errormsgs.append(getErrorMessage(iflags));
        }
        ret =  false;
        Trace("- Failed!");
    }
    else{
        Trace("- OK");
    }

    /******************************
    Add summary
    ******************************/
    Trace("\n");
    Trace("### summary ###");
    if(m_errormsgs.size() > 0){
        Trace("- Folowing interface items are missing:");
        foreach(QString msg, m_errormsgs)
            Trace("  - " + msg);
    }
    else
        Trace("Interface is full defined.");

    //now delet all Classitems
    foreach(CLASSTREE* l, ilookuplist){
        l->classlist.clear();
    }
    foreach(CLASSTREE* l, lookuplist){
        l->classlist.clear();
    }
    //delete all functionitems
    foreach(QList<FUNCTIONITEM*>l, ifunctionslookuplist){
        l.clear();
    }
    foreach(QList<FUNCTIONITEM*>l, functionslookuplist){
        l.clear();
    }
    //delete all properties
    foreach(QList<PROPERTYITEM*>l, ipropertieslookuplist){
        l.clear();
    }
    foreach(QList<PROPERTYITEM*>l, propertieslookuplist){
        l.clear();
    }
    //delete all qenums
    foreach(QList<QENUMITEM*>l, iqenumlookuplist){
        l.clear();
    }
    foreach(QList<QENUMITEM*>l, iqenumlookuplist){
        l.clear();
    }
    //delete all enums
    foreach(QList<ENUMITEM*>l, ienumlookuplist){
        l.clear();
    }
    foreach(QList<ENUMITEM*>l, enumlookuplist){
        l.clear();
    }
    //delete all qflags
    foreach(QList<QFLAGITEM*>l, iqflaglookuplist){
        l.clear();
    }
    foreach(QList<QFLAGITEM*>l, qflaglookuplist){
        l.clear();
    }
    //delete all qdeclareflags
    foreach(QList<QDECLAREFLAGSITEM*>l, iqdeclareflaglookuplist){
        l.clear();
    }
    foreach(QList<QDECLAREFLAGSITEM*>l, qdeclareflaglookuplist){
        l.clear();
    }

    return ret;
}

//<-------------------------------------------------------  Start of MetaData functions
/***********************************
Function that checks all functions
which will occur in the MetaData
***********************************/
QList<FUNCTIONITEM*> ParseManager::checkMetadataFunctions(const QList<QList<FUNCTIONITEM*> > &classfctlist, const QList<QList<FUNCTIONITEM*> > &iclassfctlist)
{
    QList<FUNCTIONITEM*> missingifcts;
    //Compare each function from interface with function from header (incl. baseclass functions)
    QList<FUNCTIONITEM*> ifcts;
    foreach(QList<FUNCTIONITEM*>ifunctionlist, iclassfctlist){
        ifcts.clear();
        //check if one header class contains all function from one interface header class
        if(classfctlist.count() > 0){
            foreach(QList<FUNCTIONITEM*>functionlist, classfctlist){
                QList<FUNCTIONITEM*> tmpl = containsAllMetadataFunction(functionlist, ifunctionlist);
                if(tmpl.size() == 0){
                    ifcts.clear();
                    break;
                }
                else
                    ifcts.append(tmpl);
            }
        }
        else {
            foreach(FUNCTIONITEM *pfct, ifunctionlist)
                pfct->classWichIsNotFound << "<all classes>";
            ifcts.append(ifunctionlist);
        }
        missingifcts.append(ifcts);
    }
    return missingifcts;
}

/*********************************************
Helper function to check if a function will
occure in the MetaData.
*********************************************/
bool ParseManager::isMetaObjFunction(FUNCTIONITEM* fct)
{
    if(fct->function->isInvokable()
        || fct->function->isSignal()
        || fct->function->isSlot())
        return true;
    return false;
}

/****************************************************
Check if all function from iclassfctlist are defined
in the classfctlist as well.
It will return all the function they are missing.
****************************************************/
QList<FUNCTIONITEM*> ParseManager::containsAllMetadataFunction(const QList<FUNCTIONITEM*> &classfctlist, const QList<FUNCTIONITEM*> &iclassfctlist)
{
    QList<FUNCTIONITEM*> ret;
    foreach(FUNCTIONITEM* ifct, iclassfctlist){
        if(isMetaObjFunction(ifct)){
            bool found = false;
            QStringList missingimplinclasses;
            ClassSpecifierAST* clspec = 0;
            QString classname = "";
            foreach(FUNCTIONITEM* fct, classfctlist){
                if(clspec != fct->highestlevelclass->classspec){
                    clspec = fct->highestlevelclass->classspec;
                    //get the classname
                    unsigned int firsttoken = clspec->name->firstToken();
                    classname += fct->trlUnit->spell(firsttoken);
                    if(missingimplinclasses.indexOf(classname) < 0)
                        missingimplinclasses.push_back(classname);
                }
                if(fct->isEqualTo(ifct, false)){
                    found = true;
                    missingimplinclasses.clear();
                    Trace("- " + getTraceFuntionString(fct, classname) + " implemented");
                    break;
                }
            }
            if(!found){
                ifct->classWichIsNotFound.append(missingimplinclasses);
                ret.push_back(ifct);
                QString classname = ifct->trlUnit->spell(ifct->highestlevelclass->classspec->name->firstToken());
                Trace("- " + getTraceFuntionString(ifct, classname) + " not implemented!");
            }
        }
    }
    return ret;
}

/************************************
Function that gives back an error
string for a MetaData function
mismatch.
************************************/
QStringList ParseManager::getErrorMessage(FUNCTIONITEM* fct)
{
    QStringList ret;
    QString fctstring = "";
    QString fcttype = "";

    foreach(QString classname, fct->classWichIsNotFound){
        QString tmp;
        QTextStream out(&tmp);

        fcttype = "";
        fctstring = classname;
        fctstring += "::";

        unsigned int token = fct->function->sourceLocation() - 1;
        while(fct->trlUnit->tokenAt(token).isNot(T_EOF_SYMBOL)){
            fctstring += fct->trlUnit->tokenAt(token).spell();
            if(*fct->trlUnit->tokenAt(token).spell() == ')')
                break;
            fctstring += " ";
            token++;
        }

        Function* pfct = fct->function;
        if(pfct){
            fcttype = "type: ";
            //Check for private, protected and public
            if(pfct->isPublic())
                fcttype = "public ";
            if(pfct->isProtected())
                fcttype = "protected ";
            if(pfct->isPrivate())
                fcttype = "private ";

            if(pfct->isVirtual())
                fcttype += "virtual ";
            if(pfct->isPureVirtual())
                fcttype += "pure virtual ";

            if(pfct->isSignal())
                fcttype += "Signal ";
            if(pfct->isSlot())
                fcttype += "Slot ";
            if(pfct->isNormal())
                fcttype += "Normal ";
            if(pfct->isInvokable())
                fcttype += "Invokable ";
        }
        out << fcttype << fctstring;
        ret << tmp;
    }
    return ret;
}
//--->

//<-------------------------------------------------------  Start of Q_PROPERTY checks
/***********************************
Function that checks all Property
which will occur in the MetaData
***********************************/
QList<PROPERTYITEM*> ParseManager::checkMetadataProperties(const QList<QList<PROPERTYITEM*> > &classproplist
                                                                         , const QList<QList<FUNCTIONITEM*> > &classfctlist
                                                                         , const QList<QList<PROPERTYITEM*> > &iclassproplist
                                                                         , const QList<QList<FUNCTIONITEM*> > &iclassfctlist)
{
    QList<PROPERTYITEM*> missingiprops;
    //assign the property functions
    foreach(QList<PROPERTYITEM*>proplist, classproplist){
        foreach(PROPERTYITEM* prop, proplist){
            assignPropertyFunctions(prop, classfctlist);
        }
    }

    foreach(QList<PROPERTYITEM*>proplist, iclassproplist){
        foreach(PROPERTYITEM* prop, proplist){
            assignPropertyFunctions(prop, iclassfctlist);
        }
    }

    //Compare each qproperty from interface with qproperty from header (incl. baseclass functions)
    QList<PROPERTYITEM*> ippts;
    foreach(QList<PROPERTYITEM*>ipropertylist, iclassproplist){
        ippts.clear();
        //check if one header class contains all function from one interface header class
        if(classproplist.count() > 0){
            foreach(QList<PROPERTYITEM*>propertylist, classproplist){
                QList<PROPERTYITEM*> tmpl = containsAllPropertyFunction(propertylist, ipropertylist);
                if(tmpl.size() == 0)
                    ippts.clear();
                else
                    ippts.append(tmpl);
            }
        }
        else {
            foreach(PROPERTYITEM *pprop, ipropertylist){
                pprop->classWichIsNotFound << "<all classes>";
                QString name = pprop->ast->property_name->name->identifier()->chars();
                Trace("- Property: <all classes>::" + name + " not found!");
            }
            ippts.append(ipropertylist);
        }
        missingiprops.append(ippts);
    }
    return missingiprops;
}

static QString findName(ExpressionAST *ast)
{
    // The "old" icheck code assumed that functions were only a single identifier, so I'll assume the same:
    if (NameAST *nameAST = ast->asName())
        return nameAST->name->identifier()->chars();
    else
        return QString();
}

/**************************************
Function that resolves the dependensies
between Q_PROPERTY
and thier READ, WRITE, NOTIFY and RESET
functions.
***************************************/
void ParseManager::assignPropertyFunctions(PROPERTYITEM* prop, const QList<QList<FUNCTIONITEM*> > &fctlookuplist)
{
    //get the name of the needed functions
    QString readfctname;
    QString writefctname;
    QString resetfctname;
    QString notifyfctname;

    int needtofind = 0;
    if(prop->readAst){
        readfctname = findName(prop->readAst);
        needtofind++;
    }
    if(prop->writeAst){
        writefctname = findName(prop->writeAst);
        needtofind++;
    }
    if(prop->resetAst){
        resetfctname = findName(prop->resetAst);
        needtofind++;
    }
    if(prop->notifyAst){
        notifyfctname = findName(prop->notifyAst);
        needtofind++;
    }
    //Now iterate over all function to find all functions wich are defined in the Q_PROPERTY macro
    if(needtofind > 0){
        prop->foundalldefinedfct = false;
        foreach(QList<FUNCTIONITEM*> fctlist, fctlookuplist){
            foreach(FUNCTIONITEM* pfct, fctlist){
                QString fctname = pfct->trlUnit->spell(pfct->function->sourceLocation());
                //check the function type against the property type
                FullySpecifiedType retTy = pfct->function->returnType();

                if (!fctname.isEmpty() && retTy.isValid()) {
                    if(prop->readAst && fctname == readfctname){
                        if (prop->type != retTy)
                            continue;
                        prop->readFct = pfct;
                        needtofind--;
                    }
                    if(prop->writeAst && fctname == writefctname){
                        prop->writeFct = pfct;
                        needtofind--;
                    }
                    if(prop->resetAst && fctname == resetfctname){
                        prop->resetFct = pfct;
                        needtofind--;
                    }
                    if(prop->notifyAst && fctname == notifyfctname){
                        prop->notifyFct = pfct;
                        needtofind--;
                    }
                    if(needtofind <= 0){
                        //a flag that indicates if a function was missing
                        prop->foundalldefinedfct = true;
                        return;
                    }
                }
            }
        }
    }
}

/**************************************
Function that checks if all functions
dependencies in Q_PROPERTY have the
same arguments and retunr value.
***************************************/
QList<PROPERTYITEM*> ParseManager::containsAllPropertyFunction(const QList<PROPERTYITEM*> &classproplist, const QList<PROPERTYITEM*> &iclassproplist)
{
    QList<PROPERTYITEM*> ret;
    foreach(PROPERTYITEM* ipropt, iclassproplist){
        if(ipropt->foundalldefinedfct){
            bool found = false;
            QStringList missingimplinclasses;
            ClassSpecifierAST* clspec = 0;
            QString classname = "";
            foreach(PROPERTYITEM* propt, classproplist){
                if(clspec != propt->highestlevelclass->classspec){
                    clspec = propt->highestlevelclass->classspec;
                    //get the classname
                    unsigned int firsttoken = clspec->name->firstToken();
                    classname += propt->trlUnit->spell(firsttoken);
                    if(missingimplinclasses.indexOf(classname) < 0)
                        missingimplinclasses.push_back(classname);
                }
                if(propt->isEqualTo(ipropt)){
                    found = true;
                    missingimplinclasses.clear();
                    Trace("- Property: " + classname + "::" + propt->ast->property_name->name->identifier()->chars() + " found");
                    break;
                }
            }
            if(!found){
                ipropt->classWichIsNotFound.append(missingimplinclasses);
                ret.push_back(ipropt);
                QString classname = ipropt->trlUnit->spell(ipropt->highestlevelclass->classspec->name->firstToken());
                Trace("- Property: " + classname + "::" + ipropt->ast->property_name->name->identifier()->chars() + " not found!");
            }
        }
        else{
            QString classname = ipropt->trlUnit->spell(ipropt->highestlevelclass->classspec->name->firstToken());
            Overview oo;
            QString proptype = oo(ipropt->type);
            Trace("- Property: " + classname + "::" + proptype + " functions are missing!");
            ret.push_back(ipropt);
        }
    }
    return ret;
}

/************************************
Function that gives back an error
string for a Q_PROPERTY mismatch.
************************************/
QStringList ParseManager::getErrorMessage(PROPERTYITEM* ppt)
{
    QStringList ret;
    QString pptstring = "";

    if(!ppt->foundalldefinedfct)
    {
        QString tmp;
        QTextStream out(&tmp);

        unsigned int firsttoken = ppt->highestlevelclass->classspec->name->firstToken();
        unsigned int lasttoken = ppt->highestlevelclass->classspec->name->lastToken();
        for(unsigned int i = firsttoken; i < lasttoken; i++){
            out << ppt->trlUnit->spell(i);
        }
        out << "::";
        firsttoken = ppt->ast->firstToken();
        lasttoken = ppt->ast->lastToken();
        for(unsigned int i = firsttoken; i <= lasttoken; i++){
            out << ppt->trlUnit->spell(i) << " ";
        }
        out << endl << " -";
        if(ppt->readAst && !ppt->readFct)
            out << "READ ";
        if(ppt->writeAst && !ppt->writeFct)
            out << "WRITE ";
        if(ppt->resetAst && !ppt->resetFct)
            out << "RESET.";
        if(ppt->notifyAst && !ppt->notifyFct)
            out << "NOTIFY ";
        out << "functions missing." << endl;
        ret << tmp;
    }
    for(int i = 0; i < ppt->classWichIsNotFound.size(); i++){
        QString tmp;
        QTextStream out(&tmp);

        pptstring = ppt->classWichIsNotFound[i];
        pptstring += "::";

        unsigned int firsttoken = ppt->ast->firstToken();
        unsigned int lasttoken = ppt->ast->lastToken();
        for(unsigned int i = firsttoken; i <= lasttoken; i++){
            pptstring += ppt->trlUnit->spell(i);
            pptstring += " ";
        }

        out << pptstring;
        ret << tmp;
    }
    return ret;
}
//--->


//<------------------------------------------------------- Start of Q_ENUMS checks
/***********************************
Function that checks all enums
which will occur in the MetaData
***********************************/
QList<QENUMITEM*> ParseManager::checkMetadataEnums(const QList<QList<QENUMITEM*> > &classqenumlist
                                                , const QList<QList<ENUMITEM*> > &classenumlist
                                                , const QList<QList<QENUMITEM*> > &iclassqenumlist
                                                , const QList<QList<ENUMITEM*> > &iclassenumlist)
{
    QList<QENUMITEM*> missingiqenums;
    //assign the property functions
    foreach(QList<QENUMITEM*>qenumlist, classqenumlist){
        foreach(QENUMITEM* qenum, qenumlist){
            assignEnumValues(qenum, classenumlist);
        }
    }
    foreach(QList<QENUMITEM*>qenumlist, iclassqenumlist){
        foreach(QENUMITEM* qenum, qenumlist){
            assignEnumValues(qenum, iclassenumlist);
        }
    }

    //Compare each qenum from interface with qenum from header (incl. baseclass functions)
    QList<QENUMITEM*> iqenums;
    foreach(QList<QENUMITEM*>iqenumlist, iclassqenumlist){
        iqenums.clear();
        //check if one header class contains all function from one interface header class
        if(classqenumlist.count() > 0){
            foreach(QList<QENUMITEM*>qenumlist, classqenumlist){
                QList<QENUMITEM*> tmpl = containsAllEnums(qenumlist, iqenumlist);
                if(tmpl.size() == 0)
                    iqenums.clear();
                else
                    iqenums.append(tmpl);

            }
        }
        else {
            foreach(QENUMITEM *qenum, iqenumlist){
                qenum->classWichIsNotFound << "<all classes>";
                Trace("- Enum: <all classes>::" + qenum->name + " not found!");
            }
            iqenums.append(iqenumlist);
        }
        missingiqenums.append(iqenums);
    }

    return missingiqenums;
}

/*********************************************
Helper function which creates a string out of
an enumerator including its values.
*********************************************/
QStringList ParseManager::getEnumValueStringList(ENUMITEM *penum, QString mappedenumname/* = ""*/)
{
    QStringList ret;
    EnumSpecifierAST *penumsec = penum->ast;
    QString enumname = penum->trlUnit->spell(penumsec->name->firstToken());
    int enumvalue = 0;
    //now iterrate over all enumitems and create a string like following:
    //EnumName.EnumItemName.Value
    //ConnectionState.disconnected.0
    for (EnumeratorListAST *plist = penum->ast->enumerator_list; plist; plist = plist->next) {
        QString value = enumname;
        if(mappedenumname.size() > 0)
            value = mappedenumname;
        value += ".";
        value += penum->trlUnit->spell(plist->value->identifier_token);
        value += ".";
        if(plist->value->equal_token > 0 && plist->value->expression){
            QString v = penum->trlUnit->spell(plist->value->expression->firstToken());
            bool ch;
            int newval = enumvalue;
            if(v.indexOf("0x") >= 0)
                newval = v.toInt(&ch, 16);
            else
                newval = v.toInt(&ch, 10);
            if(ch)
                enumvalue = newval;
        }
        value += QString::number(enumvalue);
        enumvalue++;
        // now add this enumitem string in the VALUE list
        ret << value;
    }
    return ret;
}

/**************************************
Function that resolves the dependensies
between Q_ENUMS and enums.
***************************************/
void ParseManager::assignEnumValues(QENUMITEM* qenum, const QList<QList<ENUMITEM*> > &enumlookuplist)
{
    //iterate over all enums and find the one with the same name like enumname
    bool found = false;
    foreach (QList<ENUMITEM*> penumlist, enumlookuplist) {
        foreach(ENUMITEM *penum, penumlist){
            EnumSpecifierAST *penumsec = penum->ast;
            QString enumname1 = penum->trlUnit->spell(penumsec->name->firstToken());
            if(qenum->name == enumname1){
                qenum->values << getEnumValueStringList(penum);
                found = true;
                break;
            }
        }
        if(!found)
            qenum->foundallenums = false;
    }
}

/***********************************
Function that checkt if the Q_ENUMS
are completed defined and if the
Enum values are the same.
***********************************/
QList<QENUMITEM*> ParseManager::containsAllEnums(const QList<QENUMITEM*> &classqenumlist, const QList<QENUMITEM*> &iclassqenumlist)
{
    Overview oo;

    QList<QENUMITEM*> ret;
    foreach(QENUMITEM* iqenum, iclassqenumlist){
        bool found = false;
        QStringList missingimplinclasses;
        ClassSpecifierAST* clspec = 0;
        QString classname = "";
        foreach(QENUMITEM* qenum, classqenumlist){
            if(clspec != qenum->highestlevelclass->classspec){
                clspec = qenum->highestlevelclass->classspec;
                //get the classname
                classname += oo(clspec->symbol);
                if(missingimplinclasses.indexOf(classname) < 0)
                    missingimplinclasses.push_back(classname);
            }
            if(qenum->isEqualTo(iqenum)){
                found = true;
                missingimplinclasses.clear();
                Trace("- Enum: " + classname + "::" + qenum->name + " found");
                break;
            }
        }
        if(!found){
            iqenum->classWichIsNotFound.append(missingimplinclasses);
            ret.push_back(iqenum);
            QString classname = oo(iqenum->highestlevelclass->classspec->symbol);
            Trace("- Enum: " + classname + "::" + iqenum->name + " not found!");
        }
    }
    return ret;
}

/************************************
Function that gives back an error
string for a Q_ENUMS mismatch.
************************************/
QStringList ParseManager::getErrorMessage(QENUMITEM* qenum)
{
    Overview oo;
    QStringList ret;

    if(!qenum->foundallenums)
    {
        QString tmp;
        QTextStream out(&tmp);

        out << oo(qenum->highestlevelclass->classspec->symbol);
        out << "::Q_ENUMS " << qenum->name << " enum missing.";
        ret << tmp;
    }

    for (int i = 0; i < qenum->classWichIsNotFound.size(); i++){
        QString tmp;
        QTextStream out(&tmp);

        out << qenum->classWichIsNotFound[i] << "::Q_ENUMS "
                << qenum->name;
        ret << tmp;
    }
    return ret;
}
//--->

//<------------------------------------------------------- Start of Q_FLAGS checks
/***********************************
Function that checks all flags
which will occur in the MetaData
***********************************/
QList<QFLAGITEM*> ParseManager::checkMetadataFlags(const QList<QList<QFLAGITEM*> > &classqflaglist
                                            , const QList<QList<QDECLAREFLAGSITEM*> > &classqdeclareflaglist
                                            , const QList<QList<ENUMITEM*> > &classenumlist
                                            , const QList<QList<QFLAGITEM*> > &iclassqflaglist
                                            , const QList<QList<QDECLAREFLAGSITEM*> > &iclassqdeclareflaglist
                                            , const QList<QList<ENUMITEM*> > &iclassenumlist)
{
    QList<QFLAGITEM*> missingqflags;
    //assign the enums to the flags
    foreach(QList<QFLAGITEM*>qflaglist, classqflaglist){
        foreach(QFLAGITEM* qflag, qflaglist){
            assignFlagValues(qflag, classqdeclareflaglist, classenumlist);
        }
    }
    foreach(QList<QFLAGITEM*>qflaglist, iclassqflaglist){
        foreach(QFLAGITEM* qflag, qflaglist){
            assignFlagValues(qflag, iclassqdeclareflaglist, iclassenumlist);
        }
    }

    //Compare each qenum from interface with qenum from header (incl. baseclass functions)
    QList<QFLAGITEM*> iqflags;
    foreach(QList<QFLAGITEM*>iqflaglist, iclassqflaglist){
        iqflags.clear();
        //check if one header class contains all function from one interface header class
        if(classqflaglist.count() >0){
            foreach(QList<QFLAGITEM*>qflaglist, classqflaglist){
                QList<QFLAGITEM*> tmpl = containsAllFlags(qflaglist, iqflaglist);
                if(tmpl.size() == 0)
                    iqflags.clear();
                else
                    iqflags.append(tmpl);

            }
        }
        else {
            foreach(QFLAGITEM *pflag, iqflaglist){
                pflag->classWichIsNotFound << "<all classes>";
                QString name= pflag->name->identifier()->chars();
                Trace("- Flag: <all classes>::" + name + " not found!");
            }
            iqflags.append(iqflaglist);
        }
        missingqflags.append(iqflags);
    }
    return missingqflags;
}

/**************************************
Function that resolves the dependensies
between Q_FLAG, Q_DECLARE_FLAGS
and enums.
***************************************/
void ParseManager::assignFlagValues(QFLAGITEM* qflags, const QList<QList<QDECLAREFLAGSITEM*> > &qdeclareflagslookuplist, const QList<QList<ENUMITEM*> > &enumlookuplist)
{
    QString enumname;

    //try to find if there is a deflare flag macro with the same name as in qflagname
    Scope *classMembers = qflags->highestlevelclass->classspec->symbol;
    Symbol *s = classMembers->find(qflags->name);
    if (s->isTypedef()) {
        FullySpecifiedType ty = s->type();
        if (Enum *e = ty->asEnumType()) {
            enumname = e->name()->identifier()->chars();
        }
    }

    //now we have the right enum name now we need to find the enum
    bool found = false;
    foreach(QList<ENUMITEM*> enumitemlist, enumlookuplist){
        foreach(ENUMITEM* enumitem, enumitemlist){
            EnumSpecifierAST *penumspec = enumitem->ast;
            QString enumspecname = enumitem->trlUnit->spell(penumspec->name->firstToken());
            if(enumspecname == enumname){
                qflags->enumvalues << getEnumValueStringList(enumitem, qflags->name->identifier()->chars());
                found = true;
                break;
            }
        }
        if(found)
            break;
    }
    if(!found)
        qflags->foundallenums = false;
}

/*****************************************
Function that compares if all enums
and flags assigned by using the Q_FLAGS
are complete defined.
*****************************************/
QList<QFLAGITEM*> ParseManager::containsAllFlags(const QList<QFLAGITEM*> &classqflaglist, const QList<QFLAGITEM*> &iclassqflaglist)
{
    QList<QFLAGITEM*> ret;
    foreach(QFLAGITEM* iqflags, iclassqflaglist){
        if(iqflags->foundallenums){
            bool found = false;
            QStringList missingimplinclasses;
            ClassSpecifierAST* clspec = 0;
            QString classname = "";
            foreach(QFLAGITEM* qflags, classqflaglist){
                if(clspec != qflags->highestlevelclass->classspec){
                    clspec = qflags->highestlevelclass->classspec;
                    //get the classname
                    classname += clspec->symbol->name()->identifier()->chars();
                    if(missingimplinclasses.indexOf(classname) < 0)
                        missingimplinclasses.push_back(classname);
                }
                if(qflags->isEqualTo(iqflags)){
                    found = true;
                    missingimplinclasses.clear();
                    Trace("- Flag: " + classname + "::" + qflags->name->identifier()->chars() + " found");
                    break;
                }
            }
            if(!found){
                iqflags->classWichIsNotFound.append(missingimplinclasses);
                ret.push_back(iqflags);
                QString classname = iqflags->highestlevelclass->classspec->symbol->name()->identifier()->chars();
                Trace("- Flag: " + classname + "::" + iqflags->name->identifier()->chars() + " not found!");
            }
        }
        else
            ret.push_back(iqflags);
    }
    return ret;
}

/************************************
Function that gives back an error
string for a Q_FLAGS mismatch.
************************************/
QStringList ParseManager::getErrorMessage(QFLAGITEM* pfg)
{
    Overview oo;
    QStringList ret;

    if(!pfg->foundallenums)
    {
        QString tmp;
        QTextStream out(&tmp);

        out << oo(pfg->highestlevelclass->classspec->symbol->name());
        out << "::Q_FLAGS "<<pfg->name->identifier()->chars()<< ": enum missing.";
        ret << tmp;
    }
    for(int i = 0; i < pfg->classWichIsNotFound.size(); i++){
        QString tmp;
        QTextStream out(&tmp);

        out << pfg->classWichIsNotFound[i] << "::Q_FLAGS " << oo(pfg->name);
        ret << tmp;
    }
    return ret;
}

inline QString ParseManager::getTraceFuntionString(const FUNCTIONITEM *fctitem, const QString& classname)
{
    QString ret;

    if(fctitem->function->isPublic())
        ret = "public ";
    if(fctitem->function->isProtected())
        ret = "protected ";
    if(fctitem->function->isPrivate())
        ret = "private ";

    if(fctitem->function->isVirtual())
        ret += "virtual ";
    if(fctitem->function->isPureVirtual())
        ret += "pure virtual ";

    if(fctitem->function->isSignal())
        ret += "Signal ";
    if(fctitem->function->isSlot())
        ret += "Slot ";
    if(fctitem->function->isNormal())
        ret += "Normal ";
    if(fctitem->function->isInvokable())
        ret += "Invokable ";

    ret += classname;
    ret += "::";
    ret += fctitem->trlUnit->spell(fctitem->function->sourceLocation());
    return ret;
}

void ParseManager::Trace(QString value)
{
    if(::m_resultFile){
        QTextStream out(::m_resultFile);
        if(value == "\n")
            out << endl;
        else
            out << value << endl;
    }
}

PROPERTYITEM *PROPERTYITEM::create(QtPropertyDeclarationAST *ast, const CLASSLISTITEM *clazz)
{
    PROPERTYITEM *item = new PROPERTYITEM;
    item->ast = ast;
    item->highestlevelclass = clazz;
    item->trlUnit = clazz->trlUnit;

    if (ast->type_id) {
        Bind bind(item->trlUnit);
        item->type = bind(ast->type_id, clazz->classspec->symbol);
    }

    for (QtPropertyDeclarationItemListAST *it = ast->property_declaration_items;
         it; it = it->next) {
        if (!it->value->item_name_token)
            continue;
        const char *name = item->trlUnit->spell(it->value->item_name_token);
        if (!qstrcmp(name, "READ"))
            item->readAst = it->value->expression;
        else if (!qstrcmp(name, "WRITE"))
            item->writeAst = it->value->expression;
        else if (!qstrcmp(name, "RESET"))
            item->resetAst = it->value->expression;
        else if (!qstrcmp(name, "NOTIFY"))
            item->notifyAst = it->value->expression;
    }

    return item;
}

//--->
