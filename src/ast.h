#ifndef AST_H
#define AST_H

#include <memory>
#include <string>

#include "environment.h"
#include "token.h"

namespace ast {

  // Abstract node for the AST interface
  class Node {
   public:
    Node(std::string name) : name(name), tm(TYPE_NONE) {}
    Node(std::string name, TypeMark tm) : name(name), tm(tm) {}
    virtual ~Node() = 0;
    virtual bool typeCheck() = 0;
    TypeMark getTypeMark() { return tm; }
    std::string getName() { return name; }

   protected:
    const std::string name;
    TypeMark tm;
    // TODO: Do I need this? I think not.
    static std::shared_ptr<Environment> env;
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
    BinaryOp(std::shared_ptr<Node> lhs, std::shared_ptr<Node> rhs,
        std::shared_ptr<Token> op_tok) :
      Node("Binary Op"),
      lhs(lhs),
      rhs(rhs),
      op_tok(op_tok) {}

   protected:
    std::shared_ptr<Node> lhs;
    std::shared_ptr<Node> rhs;
    std::shared_ptr<Token> op_tok;
  };

  // Not and negative ops
  class UnaryOp : public Node {
   public:
    UnaryOp(std::shared_ptr<Node> lhs, std::shared_ptr<Token> op_tok) :
      Node("Unary Op"),
      lhs(lhs),
      op_tok(op_tok) {}

   protected:
    std::shared_ptr<Node> lhs;
    std::shared_ptr<Token> op_tok;
  };

  class VariableReference : public Node { // Destination and Name
   public:

    VariableReference(std::shared_ptr<IdToken> id_tok,
        std::shared_ptr<Node> expr_node) :
      Node("Variable Reference"),
      id_tok(id_tok),
      expr_node(expr_node) {}

    VariableReference(std::shared_ptr<IdToken> id_tok) :
      Node("Variable Reference"),
      id_tok(id_tok),
      expr_node(nullptr) {}

   protected:
    std::shared_ptr<IdToken> id_tok;
    std::shared_ptr<Node> expr_node;
  };

  class AssignmentStatement : public Node {
   public:
    AssignmentStatement(std::shared_ptr<VariableReference> dest,
        std::shared_ptr<Node> expr) :
      Node("Assignment Statement") {}

   protected:
    std::shared_ptr<VariableReference> dest;
    std::shared_ptr<Node> expr;
  };

  class IfStatement : public Node {
   public:
    IfStatement(std::shared_ptr<Node> if_cond, std::shared_ptr<Node> then_stmt,
        std::shared_ptr<Node> else_stmt) :
      Node("If Statement"),
      if_cond(if_cond),
      then_stmt(then_stmt),
      else_stmt(else_stmt) {}

   protected:
    std::shared_ptr<Node> if_cond, then_stmt, else_stmt;
  };

  class LoopStatement : public Node {
   public:
    LoopStatement(std::shared_ptr<AssignmentStatement> assign,
        std::shared_ptr<Node> expr, std::shared_ptr<Node> stmt) :
      Node("Loop Statement"),
      assign(assign),
      expr(expr),
      stmt(stmt) {}

   protected:
    std::shared_ptr<AssignmentStatement> assign;
    std::shared_ptr<Node> expr, stmt;
  };

  // expr
  class ReturnStatement : public Node {
   public:
  };

  // expr, arg_list
  class ArgumentList : public Node {
   public:
  };

  // id, arg_list
  class ProcedureCall : public Node {
   public:
  };

  // id, tm, number (array size)
  class VariableDeclaration : public Node {
   public:
  };

  // declaration(s), statement(s)
  class ProcedureBody : public Node {
   public:
  };

  // var_decl, param_list
  class ParameterList : public Node {
   public:
  };

  // id, tm, param_list
  class ProcedureHeader : public Node {
   public:
  };

  // procedure_header, procedure_body
  class ProcedureDeclaration : public Node {
   public:
  };

  // statement, next_statement
  class Statement : public Node {
   public:
  };

  // global, declaration (either proc or var), next_decl
  class Declaration : public Node {
   public:
  };

  // declaration(s), statement(s)
  class ProgramBody : public Node {
   public:

   protected:
    std::shared_ptr<Declaration> decl;
    std::shared_ptr<Statement> stmt;
  };

  // id, body
  class Prog : public Node {
   public:
    Prog(std::shared_ptr<IdToken> id_tok,
        std::shared_ptr<ProgramBody> prog_body) :
      Node("Program"),
      id_tok(id_tok),
      prog_body(prog_body) {}

   protected:
    std::shared_ptr<IdToken> id_tok;
    std::shared_ptr<ProgramBody> prog_body;
  };

} // namespace ast

#endif // AST_H
