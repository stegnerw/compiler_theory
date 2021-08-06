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
    Node(std::string name) : name(name), tm(TYPE_NONE) {}
    Node(std::string name, TypeMark tm) : name(name), tm(tm) {}
    virtual ~Node() = 0;
    //virtual bool typeCheck() = 0;
    virtual std::string getName() { return name; }
    TypeMark getTypeMark() { return tm; }

   protected:
    std::string name;
    TypeMark tm;
  };

  template <class T>
  class Literal : public Node {
   public:
    Literal<T>(const T& val, const TypeMark& tm) :
      Node("Literal", tm),
      val(val) {}

    bool typeCheck() { return tm != TYPE_NONE; }
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
      op_tok(op_tok) {}

   protected:
    std::unique_ptr<Node> lhs;
    std::unique_ptr<Node> rhs;
    std::shared_ptr<Token> op_tok;
  };

  // Not and negative ops
  class UnaryOp : public Node {
   public:
    UnaryOp(std::unique_ptr<Node> lhs, std::unique_ptr<Token> op_tok) :
      Node("Unary Op"),
      lhs(std::move(lhs)),
      op_tok(std::move(op_tok)) {}

   protected:
    std::unique_ptr<Node> lhs;
    std::unique_ptr<Token> op_tok;
  };

  class VariableReference : public Node { // Destination and Name
   public:

    VariableReference(std::shared_ptr<IdToken> id_tok,
        std::unique_ptr<Node> expr_node) :
      Node("Variable Reference"),
      id_tok(id_tok),
      expr_node(std::move(expr_node)) {}

    VariableReference(std::shared_ptr<IdToken> id_tok) :
      Node("Variable Reference"),
      id_tok(id_tok),
      expr_node(nullptr) {}

   protected:
    std::shared_ptr<IdToken> id_tok;
    std::unique_ptr<Node> expr_node;
  };

  class AssignmentStatement : public Node {
   public:
    AssignmentStatement(std::unique_ptr<VariableReference> dest,
        std::unique_ptr<Node> expr) :
      Node("Assignment Statement"),
      dest(std::move(dest)),
      expr(std::move(expr)) {}

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
      else_stmt_list(std::move(else_stmt_list)) {}

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
      stmt_list(std::move(stmt_list)) {}

   protected:
    std::unique_ptr<AssignmentStatement> assign;
    std::unique_ptr<Node> expr;
    std::list<std::unique_ptr<Node>> stmt_list;
  };

  class ReturnStatement : public Node {
   public:
    ReturnStatement(std::unique_ptr<Node> expr) :
      Node("Return Statement"),
      expr(std::move(expr)) {}

   protected:
    std::unique_ptr<Node> expr;
  };

  class ArgumentList : public Node {
   public:
    ArgumentList(std::shared_ptr<Node> expr,
        std::unique_ptr<ArgumentList> arg_list) :
      Node("Argument List"),
      expr(expr),
      arg_list(std::move(arg_list)) {}

   protected:
    std::shared_ptr<Node> expr;
    std::unique_ptr<ArgumentList> arg_list;
  };

  class ProcedureCall : public Node {
   public:
    ProcedureCall(std::shared_ptr<IdToken> id_tok,
        std::unique_ptr<ArgumentList> arg_list) :
      Node("Procedure Call"),
      id_tok(id_tok),
      arg_list(std::move(arg_list)) {}

   protected:
    std::shared_ptr<IdToken> id_tok;
    std::unique_ptr<ArgumentList> arg_list;
  };

  // also Parameter
  class VariableDeclaration : public Node {
   public:
    VariableDeclaration(std::shared_ptr<IdToken> id_tok,
        std::unique_ptr<Literal<int>> bound) :
      Node("Variable Declaration"),
      id_tok(id_tok),
      bound(std::move(bound)) {}

   protected:
    std::shared_ptr<IdToken> id_tok;
    std::unique_ptr<Literal<int>> bound;
  };

  class ProcedureBody : public Node {
   public:
    ProcedureBody(std::list<std::unique_ptr<Node>> decl_list,
        std::list<std::unique_ptr<Node>> stmt_list) :
      Node("Procedure Body"),
      decl_list(std::move(decl_list)),
      stmt_list(std::move(stmt_list)) {}

   protected:
    std::list<std::unique_ptr<Node>> decl_list;
    std::list<std::unique_ptr<Node>> stmt_list;
  };

  class ParameterList : public Node {
   public:
    ParameterList(std::unique_ptr<VariableDeclaration> var_decl,
        std::unique_ptr<ParameterList> param_list) :
      Node("Parameter List"),
      var_decl(std::move(var_decl)),
      param_list(std::move(param_list)) {}

   protected:
    std::unique_ptr<VariableDeclaration> var_decl;
    std::unique_ptr<ParameterList> param_list;
  };

  class ProcedureHeader : public Node {
   public:
    ProcedureHeader(std::shared_ptr<IdToken> id_tok, TypeMark tm,
        std::unique_ptr<ParameterList> param_list) :
      Node("Procedure Header", tm), id_tok(id_tok) {}

   protected:
    std::shared_ptr<IdToken> id_tok;
    std::unique_ptr<ParameterList> param_list;
  };

  class ProcedureDeclaration : public Node {
   public:
    ProcedureDeclaration(std::unique_ptr<ProcedureHeader> proc_head,
        std::unique_ptr<ProcedureBody> proc_body) :
      Node("Procedure Declaration"),
      proc_head(std::move(proc_head)),
      proc_body(std::move(proc_body)) {}

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
      stmt_list(std::move(stmt_list)) {}

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
      prog_body(std::move(prog_body)) {}

   protected:
    std::shared_ptr<IdToken> id_tok;
    std::unique_ptr<ProgramBody> prog_body;
  };

} // namespace ast

#endif // AST_H
