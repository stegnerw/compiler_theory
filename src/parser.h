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
  std::unique_ptr<ast::Literal<float>> bound();
  std::unique_ptr<ast::Node> statement();
  std::unique_ptr<ast::ProcedureCall> procedureCall();
  std::unique_ptr<ast::AssignmentStatement> assignmentStatement();
  std::unique_ptr<ast::VariableReference> destination();
  std::unique_ptr<ast::IfStatement> ifStatement();
  std::unique_ptr<ast::LoopStatement> loopStatement();
  std::unique_ptr<ast::ReturnStatement> returnStatement();
  std::shared_ptr<IdToken> identifier(const bool&);
  std::unique_ptr<ast::Node> expression();
  std::unique_ptr<ast::Node> expressionPrime(std::unique_ptr<ast::Node>);
  std::unique_ptr<ast::Node> arithOp();
  std::unique_ptr<ast::Node> arithOpPrime(std::unique_ptr<ast::Node>);
  std::unique_ptr<ast::Node> relation();
  std::unique_ptr<ast::Node> relationPrime(std::unique_ptr<ast::Node>);
  std::unique_ptr<ast::Node> term();
  std::unique_ptr<ast::Node> termPrime(std::unique_ptr<ast::Node>);
  std::unique_ptr<ast::Node> factor();
  std::unique_ptr<ast::VariableReference> name();
  std::unique_ptr<ast::ArgumentList> argumentList();
  std::unique_ptr<ast::Literal<float>> number();
  std::unique_ptr<ast::Literal<std::string>> string();
};

#endif // PARSER_H
