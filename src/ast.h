#ifndef AST_H
#define AST_H

#include <list>
#include <memory>
#include <string>

#include "token.h"

namespace ast {

  // Abstract node for the AST interface
  class Node {
   public:
    Node() = delete;
    Node(std::string name) : name(name), tm(TYPE_NONE), size(0) {}
    Node(std::string name, TypeMark tm) : name(name), tm(tm), size(0) {}
    virtual ~Node() {}
    virtual void typeCheck() = 0;

    virtual std::string getName() { return name; }
    void setName(const std::string& n) { name = n; }

    TypeMark getTypeMark() { return tm; }
    void setTypeMark(const TypeMark& t) { tm = t; }

    int getSize() { return size; }
    void setSize(const int& s) { size = s; }

   protected:
    std::string name;
    TypeMark tm;  // Propagate during type checking
    int size;  // Propagate during type checking
  };

  // Numbers are all float and cast upon code gen
  // Type mark will reflect int vs float
  template <class T>
  class Literal : public Node {
   public:
    Literal(const TypeMark& tm, const T& val) :
      Node("Literal", tm),
      val(val) { typeCheck(); }
    void typeCheck() { size = 0; }
    T getVal() { return val; }

   protected:
    T val;
  };

  // Expression, arith_op, relation, and term ops
  class BinaryOp : public Node {
   public:
    BinaryOp(std::unique_ptr<Node> lhs, std::unique_ptr<Node> rhs,
        std::shared_ptr<Token> op_tok) :
      Node("Binary Op"),
      lhs(std::move(lhs)),
      rhs(std::move(rhs)),
      op_tok(op_tok) { typeCheck(); }
    void typeCheck();

   protected:
    std::unique_ptr<Node> lhs;
    std::unique_ptr<Node> rhs;
    std::shared_ptr<Token> op_tok;
  };

  // `not' and `-' ops
  class UnaryOp : public Node {
   public:
    UnaryOp(std::unique_ptr<Node> lhs, std::shared_ptr<Token> op_tok) :
      Node("Unary Op"),
      lhs(std::move(lhs)),
      op_tok(op_tok) { typeCheck(); }
    void typeCheck();

   protected:
    std::unique_ptr<Node> lhs;
    std::shared_ptr<Token> op_tok;
  };

  class VariableReference : public Node { // Destination and Name
   public:

    VariableReference(std::shared_ptr<IdToken> id_tok,
        std::unique_ptr<Node> expr) :
      Node("Variable Reference"), id_tok(id_tok),
      expr(std::move(expr)) { typeCheck(); }
    VariableReference(std::shared_ptr<IdToken> id_tok) :
      Node("Variable Reference"), id_tok(id_tok), expr(nullptr) { typeCheck(); }
    void typeCheck();

   protected:
    std::shared_ptr<IdToken> id_tok;
    std::unique_ptr<Node> expr;
  };

  class AssignmentStatement : public Node {
   public:
    AssignmentStatement(std::unique_ptr<VariableReference> dest,
        std::unique_ptr<Node> expr) :
      Node("Assignment Statement"),
      dest(std::move(dest)),
      expr(std::move(expr)) { typeCheck(); }
    void typeCheck();

   protected:
    std::unique_ptr<VariableReference> dest;
    std::unique_ptr<Node> expr;
  };

  class IfStatement : public Node {
   public:
    IfStatement(std::unique_ptr<Node> if_cond,
        std::list<std::unique_ptr<Node>> then_stmt_list,
        std::list<std::unique_ptr<Node>> else_stmt_list) :
      Node("If Statement"),
      if_cond(std::move(if_cond)),
      then_stmt_list(std::move(then_stmt_list)),
      else_stmt_list(std::move(else_stmt_list)) { typeCheck(); }
    void typeCheck();

   protected:
    std::unique_ptr<Node> if_cond;
    std::list<std::unique_ptr<Node>> then_stmt_list, else_stmt_list;
  };

  class LoopStatement : public Node {
   public:
    LoopStatement(std::unique_ptr<AssignmentStatement> assign,
        std::unique_ptr<Node> expr,
        std::list<std::unique_ptr<Node>> stmt_list) :
      Node("Loop Statement"),
      assign(std::move(assign)),
      expr(std::move(expr)),
      stmt_list(std::move(stmt_list)) { typeCheck(); }
    void typeCheck();

   protected:
    std::unique_ptr<AssignmentStatement> assign;
    std::unique_ptr<Node> expr;
    std::list<std::unique_ptr<Node>> stmt_list;
  };

  class ReturnStatement : public Node {
   public:
    ReturnStatement(const TypeMark& tm, std::unique_ptr<Node> expr) :
      Node("Return Statement", tm),
      expr(std::move(expr)) { typeCheck(); }
    void typeCheck();

   protected:
    std::unique_ptr<Node> expr;
  };

  class ProcedureCall : public Node {
   public:
    ProcedureCall(std::shared_ptr<IdToken> id_tok,
        std::list<std::unique_ptr<Node>> arg_list) :
      Node("Procedure Call"),
      id_tok(id_tok),
      arg_list(std::move(arg_list)) { typeCheck(); }
    void typeCheck();

   protected:
    std::shared_ptr<IdToken> id_tok;
    std::list<std::unique_ptr<Node>> arg_list;
  };

  // TODO: Type checking node: check bound size
  class VariableDeclaration : public Node {  // also Parameter
   public:
    VariableDeclaration(const TypeMark& tm, const bool& global,
        std::shared_ptr<IdToken> id_tok,
        std::unique_ptr<Literal<float>> bound) :
      Node("Variable Declaration", tm), global(global), id_tok(id_tok),
      bound(std::move(bound)) { typeCheck(); }
    std::shared_ptr<IdToken> getIdTok() { return id_tok; }
    void typeCheck();

   protected:
    bool global;
    std::shared_ptr<IdToken> id_tok;
    std::unique_ptr<Literal<float>> bound;
  };

  class ProcedureBody : public Node {
   public:
    ProcedureBody(std::list<std::unique_ptr<Node>> decl_list,
        std::list<std::unique_ptr<Node>> stmt_list) :
      Node("Procedure Body"),
      decl_list(std::move(decl_list)),
      stmt_list(std::move(stmt_list)) { typeCheck(); }
    void typeCheck();

   protected:
    std::list<std::unique_ptr<Node>> decl_list;
    std::list<std::unique_ptr<Node>> stmt_list;
  };

  class ProcedureHeader : public Node {
   public:
    ProcedureHeader(const TypeMark& tm, const bool& global,
        std::shared_ptr<IdToken> id_tok,
        std::list<std::unique_ptr<VariableDeclaration>> param_list) :
      Node("Procedure Header", tm),
      global(global),
      id_tok(id_tok),
      param_list(std::move(param_list)) { typeCheck(); }
    void typeCheck();

   protected:
    bool global;
    std::shared_ptr<IdToken> id_tok;
    std::list<std::unique_ptr<VariableDeclaration>> param_list;
  };

  class ProcedureDeclaration : public Node {
   public:
    ProcedureDeclaration(std::unique_ptr<ProcedureHeader> proc_head,
        std::unique_ptr<ProcedureBody> proc_body) :
      Node("Procedure Declaration"),
      proc_head(std::move(proc_head)),
      proc_body(std::move(proc_body)) { typeCheck(); }
    void typeCheck();

   protected:
    std::unique_ptr<ProcedureHeader> proc_head;
    std::unique_ptr<ProcedureBody> proc_body;
  };

  class ProgramBody : public Node {
   public:
    ProgramBody(std::list<std::unique_ptr<Node>> decl_list,
        std::list<std::unique_ptr<Node>> stmt_list) :
      Node("Program Body"),
      decl_list(std::move(decl_list)),
      stmt_list(std::move(stmt_list)) { typeCheck(); }
    void typeCheck();

   protected:
    std::list<std::unique_ptr<Node>> decl_list;
    std::list<std::unique_ptr<Node>> stmt_list;
  };

  class Program : public Node {
   public:
    Program(std::shared_ptr<IdToken> id_tok,
        std::unique_ptr<ProgramBody> prog_body) :
      Node("Program"),
      id_tok(id_tok),
      prog_body(std::move(prog_body)) { typeCheck(); }
    void typeCheck();

   protected:
    std::shared_ptr<IdToken> id_tok;
    std::unique_ptr<ProgramBody> prog_body;
  };

} // namespace ast

#endif // AST_H
