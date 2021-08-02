#ifndef PARSER_H
#define PARSER_H

#include <fstream>
#include <memory>
#include <stack>
#include <string>
#include <unordered_map>

#include "environment.h"
#include "scanner.h"
#include "token.h"
#include "type_checker.h"

class Parser {
public:
  Parser();
  bool init(const std::string&);
  bool parse();  // program

private:
  std::shared_ptr<Environment> env;
  Scanner scanner;
  TypeChecker type_checker;
  std::shared_ptr<Token> tok;
  std::stack<std::shared_ptr<IdToken>> function_stack;
  bool panic_mode;
  void scan();
  bool matchToken(const TokenType&);
  bool expectToken(const TokenType&);
  void panic();
  void push_scope(std::shared_ptr<IdToken>);
  void pop_scope();
  void programHeader();
  void programBody();
  void declarations(bool);
  void statements();
  void declaration(bool);
  void procedureDeclaration(const bool&);
  void procedureHeader(const bool&);
  void parameterList();
  std::shared_ptr<IdToken> parameter();
  void procedureBody();
  std::shared_ptr<IdToken> variableDeclaration(const bool&);
  TypeMark typeMark();
  int bound();
  void statement();
  TypeMark procedureCall();
  void assignmentStatement();
  TypeMark destination(int&);
  void ifStatement();
  void loopStatement();
  void returnStatement();
  std::shared_ptr<IdToken> identifier(const bool&);
  TypeMark expression(int&);
  TypeMark expressionPrime(const TypeMark&, int&);
  TypeMark arithOp(int&);
  TypeMark arithOpPrime(const TypeMark&, int&);
  TypeMark relation(int&);
  TypeMark relationPrime(const TypeMark&, int&);
  TypeMark term(int&);
  TypeMark termPrime(const TypeMark&, int&);
  TypeMark factor(int&);
  TypeMark name(int&);
  void argumentList(const int&, std::shared_ptr<IdToken>);
  std::shared_ptr<Token> number();
  std::shared_ptr<LiteralToken<std::string>> string();
};

#endif // PARSER_H
