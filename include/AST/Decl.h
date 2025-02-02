//
// Created by tangny on 2021/12/6.
//

// Decl基类及其派生类

// 在组织类顺序时，使属于同一层的类排在一起
// e.g. Decl类在第0层，
// NamedDecl派生自Decl，所以在第1层
// ValueDecl派生自NamedDecl，所以在第2层
// 依此类推
// 派生自同一类型的派生类排在一起

// 因为Type在AST中特指类型
// 所以在涉及AST节点相关操作时，使用Kind代替Type

//Fixme：
// 析构函数还均未声明

#ifndef FRONTEND_DECL_H
#define FRONTEND_DECL_H

#include <vector>
#include <string>
#include<map>
#include <cassert>
#include "include/AST/Type.h"
#include "include/AST/AbstractASTNode.h"

// 提前声明
// 避免循环引用
class Stmt;
class CompoundStmt;
class DeclStmt;
class Expr;
class CastExpr;

// 解决顺序问题
class ParamVarDecl;

// Decl基类
// 所有Decl均应直接或间接继承该类
class Decl : public AbstractASTNode {
public:
    // 在其中枚举所有Decl的子类
    // 参考DeclBase.h第92行左右定义
    // 和DeclNodes.inc中定义
    // 问题：clang中对于部分高层派生类未定义本身Kind
    // 只定义了first##Class和last##Class
    // 有何作用？
    enum Kind : short {
        k_Decl,
        k_TranslationUnitDecl,
        k_NamedDecl,
        k_TypeDecl,
        k_RecordDecl,
        k_EnumDecl,
        k_ValueDecl,
        k_DeclaratorDecl,
        k_FunctionDecl,
        k_VarDecl,
        k_ParamVarDecl
    };

    short declKind;

    Decl() {
        basicKind = bk_Decl;
        declKind = k_Decl;
    }

    // 返回派生类类型
    Kind getKind() const { return static_cast<Kind>(declKind); }
};

// 所有会包含代码作用域的Decl类都会从它派生
// 如FunctionDecl，BlockDecl等
// 该类可以实现如查找声明的作用域中所有声明的功能
// 如果一个类ClassA及其所有派生类都要继承该类
// 只在ClassA上声明继承
// 所有应该直接继承该类的类如下：
///   TranslationUnitDecl
///   ExternCContext
///   NamespaceDecl
///   TagDecl
///   OMPDeclareReductionDecl
///   OMPDeclareMapperDecl
///   FunctionDecl
///   ObjCMethodDecl (本项目确认不需要)
///   ObjCContainerDecl (本项目确认不需要)
///   LinkageSpecDecl
///   ExportDecl
///   BlockDecl
class DeclContext {
private:
    std::vector< Decl* > decls;
    std::vector<std::map<std::string, QualType>* >symbolTables;
public:
    DeclContext() {
        decls.resize(0);
    }

    int getNumDecls() const { return decls.size(); }

    Decl* getDecl(int pos) const {
        assert(pos < decls.size() && "Asking for decl out of bound.");
        return decls[pos];
    }

    void addDecl( Decl *decl ) { decls.emplace_back(decl); }
    void addSymbolTable(std::map<std::string, QualType>*newtable)
    {
        symbolTables.push_back(newtable);
    }
    //当前符号表
    std::map<std::string, QualType>* getCurSymbolTable()
    {
        return symbolTables[symbolTables.size() - 1];
    }
    //当前局部作用域结束了，表也退出
    void exitCurSymbolTable()
    {
        symbolTables.pop_back();
    }
    bool checkSymbol(std::string name,QualType& type)
    {
        //检查一个变量是否出现过，存在返回true并更新QualType
        if(symbolTables.size()>0)
        {
            for (int it = symbolTables.size() - 1; it >=0; --it)
            {
                if(symbolTables[it]->size()==0)
                    continue;
                //此时的it是个map
                if (symbolTables[it]->find(name) != symbolTables[it]->end())
                {
                    type= symbolTables[it]->find(name)->second;
                    std::cout<<name<<" is a identifier~ \n";
                    return true;
                }
            }
            return false;
        }
        //没出现就返回false
        else
            return false;

    }
    bool  addSymbol(std::string name, QualType type)
    {
        std::map<std::string, QualType>* curtable = this->getCurSymbolTable();
        if (!checkSymbol(name,type))
        {
            std::cout<<"add a symbol "<<name<<std::endl;
            curtable->insert(std::make_pair(name, type));
            return true;
        }
        std::cout << "Error, it has already declared!";
        return false;
    }
    void clearSymbolTable()
    {
        symbolTables.clear();
    }
};
// 全局的上下文
// 主要用于保存全局变量，类/结构体定义和函数定义（目前）
class GlobalContext {
protected:
    std::vector< Decl* > decls;
    std::vector< Stmt* > stmts;
    //全局符号表
    std::map<std::string, QualType> symbolTable;
public:
    GlobalContext() {
        decls.resize(0);
        stmts.resize(0);
    }

    int getNumDecls() const { return decls.size(); }

    Decl* getDecl(int pos) {
        assert(pos < decls.size() && "Asking for decl out of bound.");
        return decls[pos];
    }

    void addDecl( Decl *decl )
    {
        decls.emplace_back(decl);
    }

    int getNumStmts() const { return stmts.size(); }

    Stmt* getStmt(int pos) {
        assert(pos < stmts.size() && "Asking for decl out of bound.");
        return stmts[pos];
    }
    void addStmt( Stmt *stmt ) { stmts.emplace_back(stmt); }
    std::map<std::string, QualType> getSymbolTable()
    {
        return symbolTable;
    }
    void addSymbol(std::string _name, QualType _type)
    {
        if(!checkSymbol(_name,_type))
        {
            std::cout<<"add a symbol "<<_name<<std::endl;
            short k=_type.getTypeKind();
            symbolTable.insert(make_pair(_name, _type));
        }
        else
            std::cout<<_name<<" has been already added\n";

    }
    bool checkSymbol(std::string name, QualType&type)
    {
        //能找到返回true
        if(symbolTable.find(name)!=symbolTable.end())
        {
            type=symbolTable.find(name)->second;
            std::cout<<name<<" is a global identifier~ \n";
            return true;
        }
        return false;
    }
    void clearSymbolTable()
    {
        symbolTable.clear();
    }
};

class TranslationUnitDecl : public Decl, public GlobalContext {
protected:
    std::vector< Decl* > decls;
    std::vector< Stmt* > stmts;
public:
    TranslationUnitDecl()
            : Decl(), GlobalContext() {
        declKind = k_TranslationUnitDecl;
        decls.resize(0);
        stmts.resize(0);
    }

    int getNumDecls() const { return decls.size(); }

    Decl* getDecl(int pos) {
        assert(pos < decls.size() && "Asking for decl out of bound.");
        return decls[pos];
    }

    void addDecl( Decl *decl ) { decls.emplace_back(decl); }

    int getNumStmts() const { return stmts.size(); }

    Stmt* getStmt(int pos) {
        assert(pos < stmts.size() && "Asking for decl out of bound.");
        return stmts[pos];
    }

    void addStmt( Stmt *stmt ) { stmts.emplace_back(stmt); }
};

class NamedDecl : public Decl {
protected:
    std::string name;

public:
    NamedDecl()
    : Decl() {
        declKind = k_NamedDecl;
        name = "";
    }

    std::string getName() const { return name; }

    void setName(std::string _name)  { name = _name; }
};

class TypeDecl : public NamedDecl {
public:
    TypeDecl()
    : NamedDecl() {
        declKind = k_TypeDecl;
    }
};

class RecordDecl : public TypeDecl {
public:
    std::vector<DeclStmt*> declStmts;
    bool isS;

public:
    RecordDecl()
    : TypeDecl() {
        declKind = k_RecordDecl;
        declStmts.resize(0);
        isS = true;
    }

    int getNumDeclStmts() const { return declStmts.size(); }

    DeclStmt* getDeclStmt( int pos ) {
        assert(pos < declStmts.size());
        return declStmts[pos];
    }

    void addDeclStmt(DeclStmt *_declStmt) {
        declStmts.emplace_back(_declStmt);
    }

    bool hasTag() const { return name != ""; }

    bool isStruct() const { return isS ; }

    bool isUnion() const { return !isS; }

    void setStruct() { isS = 1; }

    void setUnion() { isS = 0; }
};

class EnumDecl : public TypeDecl {
public:
    std::vector<std::pair<std::string, Expr *>> enumerators;

public:
    EnumDecl()
            : TypeDecl() {
        declKind = k_EnumDecl;
        enumerators.resize(0);
    }

    int getNumEnumerators() const { return enumerators.size(); }

    std::pair<std::string, Expr *> getEnumerator( int pos ) {
        assert(pos < enumerators.size());
        return enumerators[pos];
    }

    void addEnumerator(std::string _name, Expr *_value) {
        enumerators.emplace_back(std::make_pair(_name, _value));
    }

    void setEnumeratorExpr(int pos, Expr *_value) {
        enumerators[pos].second = _value;
    }

    bool hasTag() const { return name != ""; }
};

class ValueDecl : public NamedDecl {
protected:
    QualType valueType;

public:
    ValueDecl()
    : NamedDecl() {
        declKind = k_ValueDecl;
    }

    ValueDecl( std::string _name, QualType _valueType )
    : NamedDecl() {
        declKind = k_ValueDecl;
        valueType = _valueType;
    }

    QualType getQualType() const { return valueType; }

    void setQualType( QualType _valueType ) { valueType =  _valueType; }
};

class DeclaratorDecl : public ValueDecl {
public:
    DeclaratorDecl()
    : ValueDecl() {
        declKind = k_DeclaratorDecl;
    }

    DeclaratorDecl( std::string _name, QualType _valueType )
    : ValueDecl(_name, _valueType) {
        declKind = k_DeclaratorDecl;
    }
};

class FunctionDecl : public DeclaratorDecl, public DeclContext {
protected:
    std::vector< ParamVarDecl* > params;

    CompoundStmt *functionBody;

public:
    FunctionDecl()
    : DeclaratorDecl(), DeclContext() {
        declKind = k_FunctionDecl;
        params.resize(0);
        functionBody = nullptr;
    }

    FunctionDecl( std::string _name, QualType _retType )
    : DeclaratorDecl( _name, _retType ), DeclContext() {
        declKind = k_FunctionDecl;
        params.resize(0);
        functionBody = nullptr;
    }

    int getNumParams() { return params.size(); }

    ParamVarDecl* getParam(int pos) {
        assert(pos < params.size() && "Asking for param out of bound.");
        return params[pos];
    }

    void addParam( ParamVarDecl* param ) { params.emplace_back(param); }

    void setParams(std::vector< ParamVarDecl* > _params) {
        params = _params;
    }

    CompoundStmt* getBody() { return functionBody; }

    void setBody(CompoundStmt *_functionBody) { functionBody = _functionBody; }
};
class VarDecl : public DeclaratorDecl {
protected:
    Expr *initializer;

public:
    VarDecl()
            : DeclaratorDecl() {
        declKind = k_VarDecl;
        initializer = nullptr;
    }

    VarDecl( std::string _name, QualType _valueType )
            : DeclaratorDecl( _name, _valueType) {
        declKind = k_VarDecl;
        initializer = nullptr;
    }

    bool hasInitializer() { return initializer != nullptr; }

    Expr* getInitializer() { return initializer; }

    void setInitializer( Expr *_initializer ) { initializer = _initializer; }
};


class ParamVarDecl : public VarDecl {
public:
    ParamVarDecl()
            : VarDecl() {
        declKind = k_ParamVarDecl;
    }

    ParamVarDecl( std::string _name, QualType _valueType )
            : VarDecl( _name, _valueType) {
        declKind = k_ParamVarDecl;
    }
};



#endif //FRONTEND_DECL_H
