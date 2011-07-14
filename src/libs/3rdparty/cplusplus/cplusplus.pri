
DEPENDPATH += $$PWD
INCLUDEPATH += $$PWD

HEADERS += \
    $$PWD/CPlusPlus.h \
    $$PWD/AST.h \
    $$PWD/ASTVisitor.h \
    $$PWD/ASTMatcher.h \
    $$PWD/ASTPatternBuilder.h \
    $$PWD/ASTfwd.h \
    $$PWD/TypeMatcher.h \
    $$PWD/CPlusPlusForwardDeclarations.h \
    $$PWD/Control.h \
    $$PWD/CoreTypes.h \
    $$PWD/DiagnosticClient.h \
    $$PWD/FullySpecifiedType.h \
    $$PWD/Lexer.h \
    $$PWD/LiteralTable.h \
    $$PWD/Literals.h \
    $$PWD/MemoryPool.h \
    $$PWD/Name.h \
    $$PWD/NameVisitor.h \
    $$PWD/Names.h \
    $$PWD/Parser.h \
    $$PWD/Scope.h \
    $$PWD/Bind.h \
    $$PWD/Symbol.h \
    $$PWD/Symbols.h \
    $$PWD/SymbolVisitor.h \
    $$PWD/Token.h \
    $$PWD/TranslationUnit.h \
    $$PWD/Type.h \
    $$PWD/TypeVisitor.h \
    $$PWD/ObjectiveCTypeQualifiers.h \
    $$PWD/QtContextKeywords.h \
    $$PWD/Templates.h

SOURCES += \
    $$PWD/AST.cpp \
    $$PWD/ASTVisit.cpp \
    $$PWD/ASTMatch0.cpp \
    $$PWD/ASTVisitor.cpp \
    $$PWD/ASTClone.cpp \
    $$PWD/ASTPatternBuilder.cpp \
    $$PWD/ASTMatcher.cpp \
    $$PWD/TypeMatcher.cpp \
    $$PWD/Control.cpp \
    $$PWD/CoreTypes.cpp \
    $$PWD/DiagnosticClient.cpp \
    $$PWD/FullySpecifiedType.cpp \
    $$PWD/Keywords.cpp \
    $$PWD/ObjectiveCAtKeywords.cpp \
    $$PWD/ObjectiveCTypeQualifiers.cpp \
    $$PWD/Lexer.cpp \
    $$PWD/LiteralTable.cpp \
    $$PWD/Literals.cpp \
    $$PWD/MemoryPool.cpp \
    $$PWD/Name.cpp \
    $$PWD/NameVisitor.cpp \
    $$PWD/Names.cpp \
    $$PWD/Parser.cpp \
    $$PWD/Scope.cpp \
    $$PWD/Bind.cpp \
    $$PWD/Symbol.cpp \
    $$PWD/Symbols.cpp \
    $$PWD/SymbolVisitor.cpp \
    $$PWD/Token.cpp \
    $$PWD/TranslationUnit.cpp \
    $$PWD/Type.cpp \
    $$PWD/TypeVisitor.cpp \
    $$PWD/QtContextKeywords.cpp \
    $$PWD/Templates.cpp
