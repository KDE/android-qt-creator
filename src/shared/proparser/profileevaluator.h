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

#ifndef PROFILEEVALUATOR_H
#define PROFILEEVALUATOR_H

#include "proitems.h"

#include <QtCore/QHash>
#include <QtCore/QStringList>
#ifdef PROEVALUATOR_THREAD_SAFE
# include <QtCore/QMutex>
# include <QtCore/QWaitCondition>
#endif

QT_BEGIN_NAMESPACE

struct ProFileOption;
class ProFileParser;

class ProFileEvaluatorHandler
{
public:
    // qmake/project configuration error
    virtual void configError(const QString &msg) = 0;
    // Some error during evaluation
    virtual void evalError(const QString &filename, int lineNo, const QString &msg) = 0;
    // error() and message() from .pro file
    virtual void fileMessage(const QString &msg) = 0;

    enum EvalFileType { EvalProjectFile, EvalIncludeFile, EvalConfigFile, EvalFeatureFile, EvalAuxFile };
    virtual void aboutToEval(ProFile *parent, ProFile *proFile, EvalFileType type) = 0;
    virtual void doneWithEval(ProFile *parent) = 0;
};


class ProFileEvaluator
{
    class Private;

public:
    class FunctionDef {
    public:
        FunctionDef(ProFile *pro, int offset) : m_pro(pro), m_offset(offset) { m_pro->ref(); }
        FunctionDef(const FunctionDef &o) : m_pro(o.m_pro), m_offset(o.m_offset) { m_pro->ref(); }
        ~FunctionDef() { m_pro->deref(); }
        FunctionDef &operator=(const FunctionDef &o)
        {
            if (this != &o) {
                m_pro->deref();
                m_pro = o.m_pro;
                m_pro->ref();
                m_offset = o.m_offset;
            }
            return *this;
        }
        ProFile *pro() const { return m_pro; }
        const ushort *tokPtr() const { return m_pro->tokPtr() + m_offset; }
    private:
        ProFile *m_pro;
        int m_offset;
    };

    struct FunctionDefs {
        QHash<ProString, FunctionDef> testFunctions;
        QHash<ProString, FunctionDef> replaceFunctions;
    };

    enum TemplateType {
        TT_Unknown = 0,
        TT_Application,
        TT_Library,
        TT_Script,
        TT_Subdirs
    };

    // Call this from a concurrency-free context
    static void initialize();

    ProFileEvaluator(ProFileOption *option, ProFileParser *parser, ProFileEvaluatorHandler *handler);
    ~ProFileEvaluator();

    ProFileEvaluator::TemplateType templateType() const;
    void setCumulative(bool on); // Default is true!
    void setOutputDir(const QString &dir); // Default is empty

    // -nocache, -cache, -spec, QMAKESPEC
    // -set persistent value
    void setCommandLineArguments(const QStringList &args);

    enum LoadFlag {
        LoadProOnly = 0,
        LoadPreFiles = 1,
        LoadPostFiles = 2,
        LoadAll = LoadPreFiles|LoadPostFiles
    };
    Q_DECLARE_FLAGS(LoadFlags, LoadFlag)
    bool accept(ProFile *pro, LoadFlags flags = LoadAll);

    bool contains(const QString &variableName) const;
    QString value(const QString &variableName) const;
    QStringList values(const QString &variableName) const;
    QStringList values(const QString &variableName, const ProFile *pro) const;
    QStringList absolutePathValues(const QString &variable, const QString &baseDirectory) const;
    QStringList absoluteFileValues(
            const QString &variable, const QString &baseDirectory, const QStringList &searchDirs,
            const ProFile *pro) const;
    QString propertyValue(const QString &val) const;

private:
    Private *d;

    friend struct ProFileOption;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(ProFileEvaluator::LoadFlags)

// This struct is from qmake, but we are not using everything.
struct ProFileOption
{
    ProFileOption();
    ~ProFileOption();

    //simply global convenience
    //QString libtool_ext;
    //QString pkgcfg_ext;
    //QString prf_ext;
    //QString prl_ext;
    //QString ui_ext;
    //QStringList h_ext;
    //QStringList cpp_ext;
    //QString h_moc_ext;
    //QString cpp_moc_ext;
    //QString obj_ext;
    //QString lex_ext;
    //QString yacc_ext;
    //QString h_moc_mod;
    //QString cpp_moc_mod;
    //QString lex_mod;
    //QString yacc_mod;
    QString dir_sep;
    QString dirlist_sep;
    QString qmakespec;
    QString cachefile;
    QHash<QString, QString> properties;

    enum TARG_MODE { TARG_UNIX_MODE, TARG_WIN_MODE, TARG_MACX_MODE, TARG_MAC9_MODE, TARG_QNX6_MODE };
    TARG_MODE target_mode;
    //QString pro_ext;
    //QString res_ext;

    static const struct TargetModeMapElement {
        const char * const qmakeOption;
        const TARG_MODE targetMode;
    } modeMap[];
    static const int modeMapSize;

  private:
    void setHostTargetMode();

    friend class ProFileEvaluator;
    friend class ProFileEvaluator::Private;
    QHash<ProString, ProStringList> base_valuemap; // Cached results of qmake.conf, .qmake.cache & default_pre.prf
    ProFileEvaluator::FunctionDefs base_functions;
    QStringList feature_roots;
    QString qmakespec_name;
#ifdef PROEVALUATOR_THREAD_SAFE
    QMutex mutex;
    QWaitCondition cond;
    bool base_inProgress;
#endif
};

Q_DECLARE_TYPEINFO(ProFileEvaluator::FunctionDef, Q_MOVABLE_TYPE);

QT_END_NAMESPACE

#endif // PROFILEEVALUATOR_H
