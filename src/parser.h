#ifndef PARSER_H
#define PARSER_H

#include <fstream>
#include <list>
#include <memory>
#include <stack>
#include <string>
#include <unordered_map>

#include "ast.h"
#include "environment.h"
#include "scanner.h"
#include "token.h"
#include "type_checker.h"

class Parser {
public:
  Parser();
  bool init(const std::string&);
  std::unique_ptr<ast::Program> parse();

private:
  std::shared_ptr<Environment> env;
  Scanner scanner;
  TypeChecker type_checker;
  std::shared_ptr<Token> tok;
  std::stack<std::shared_ptr<IdToken>> function_stack;
  bool panic_mode;
  void scan();
  bool match(const TokenType&);
  bool expect(const TokenType&);
  bool expectScan(const TokenType&);
  void panic();
  void pushScope(std::shared_ptr<IdToken>);
  void popScope();
  std::shared_ptr<IdToken> programHeader();
  std::unique_ptr<ast::ProgramBody> programBody();
  std::list<std::unique_ptr<ast::Node>> declarations(bool);
  std::list<std::unique_ptr<ast::Node>> statements();
  std::unique_ptr<ast::Node> declaration(bool);
  std::unique_ptr<ast::ProcedureDeclaration> procedureDeclaration(const bool&);
  std::unique_ptr<ast::ProcedureHeader> procedureHeader(const bool&);
  std::unique_ptr<ast::ParameterList> parameterList();
  std::unique_ptr<ast::VariableDeclaration> parameter();
  std::unique_ptr<ast::ProcedureBody> procedureBody();
  std::unique_ptr<ast::VariableDeclaration> variableDeclaration(const bool&);
  TypeMark typeMark();
  std::unique_ptr<ast::Literal<int>> bound();
  std::unique_ptr<ast::Node> statement();
  std::unique_ptr<ast::ProcedureCall> procedureCall();
  std::unique_ptr<ast::AssignmentStatement> assignmentStatement();
  std::unique_ptr<ast::VariableReference> destination(int&);
  std::unique_ptr<ast::IfStatement> ifStatement();
  std::unique_ptr<ast::LoopStatement> loopStatement();
  std::unique_ptr<ast::ReturnStatement> returnStatement();
  // TODO: Make this a ast::Node type? Would be more consistent
  std::shared_ptr<IdToken> identifier(const bool&);
  // <expression> and descendants will return unary or binary operator nodes
  std::unique_ptr<ast::Node> expression(int&);
  std::unique_ptr<ast::Node> expressionPrime(const TypeMark&, int&);
  std::unique_ptr<ast::Node> arithOp(int&);
  std::unique_ptr<ast::Node> arithOpPrime(const TypeMark&, int&);
  std::unique_ptr<ast::Node> relation(int&);
  std::unique_ptr<ast::Node> relationPrime(const TypeMark&, int&);
  std::unique_ptr<ast::Node> term(int&);
  std::unique_ptr<ast::Node> termPrime(const TypeMark&, int&);
  // Return one of: binary/unary op node, literal, or variable reference
  std::unique_ptr<ast::Node> factor(int&);
  std::unique_ptr<ast::VariableReference> name(int&);
  std::unique_ptr<ast::ArgumentList> argumentList(const int&, std::shared_ptr<IdToken>);
  // Either a float or an int
  std::unique_ptr<ast::Node> number();
  std::unique_ptr<ast::Literal<std::string>> string();
};

#endif // PARSER_H
