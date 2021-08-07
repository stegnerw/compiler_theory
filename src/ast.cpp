#include "ast.h"

#include <list>
#include <memory>
#include <string>

#include "token.h"
#include "type_checker.h"

namespace ast {

  void BinaryOp::typeCheck() {
  };

  void UnaryOp::typeCheck() {
  };

  void VariableReference::typeCheck() {
  };

  void AssignmentStatement::typeCheck() {
    auto op_tok = std::make_shared<Token>(TOK_OP_ASS, ":=");
    type_checker::checkCompatible(op_tok, dest->getTypeMark(),
        expr->getTypeMark());
    type_checker::checkArraySize(op_tok, dest->getSize(), expr->getSize());
  };

  void IfStatement::typeCheck() {
  };

  void LoopStatement::typeCheck() {
  };

  void ReturnStatement::typeCheck() {
  };

  void ProcedureCall::typeCheck() {
  };

  // TODO: Type checking node: check bound size
  void VariableDeclaration::typeCheck() {

    // Check for bound
    if (bound != nullptr) {
    }
  };

  void ProcedureBody::typeCheck() {
  };

  void ProcedureHeader::typeCheck() {
  };

  void ProcedureDeclaration::typeCheck() {
  };

  void ProgramBody::typeCheck() {
  };

  void Program::typeCheck() {
  };

} // namespace ast
