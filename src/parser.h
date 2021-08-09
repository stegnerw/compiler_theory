#ifndef PARSER_H
#define PARSER_H

#include <fstream>
#include <memory>
#include <stack>
#include <string>
#include <unordered_map>

#include "code_gen.h"
#include "environment.h"
#include "scanner.h"
#include "token.h"
#include "type_checker.h"

class Parser {
public:
  Parser();
  bool init(const std::string&);
  bool parse(const std::string&);  // program

private:
  std::shared_ptr<Environment> env;
  Scanner scanner;
  CodeGen code_gen;
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
  std::shared_ptr<IdToken> variableDeclaration(const bool&, const bool&);
  TypeMark typeMark();
  int bound();
  void statement();
  TypeMark procedureCall(std::string&);
  void assignmentStatement();
  TypeMark destination(int&, std::string&);
  void ifStatement();
  void loopStatement();
  void returnStatement();
  std::shared_ptr<IdToken> identifier(const bool&);
  TypeMark expression(int&, std::string&);
  TypeMark expressionPrime(const TypeMark&, int&, std::string&);
  TypeMark arithOp(int&, std::string&);
  TypeMark arithOpPrime(const TypeMark&, int&, std::string&);
  TypeMark relation(int&, std::string&);
  TypeMark relationPrime(const TypeMark&, int&, std::string&);
  TypeMark term(int&, std::string&);
  TypeMark termPrime(const TypeMark&, int&, std::string&);
  TypeMark factor(int&, std::string&);
  TypeMark name(int&, std::string&);
  void argumentList(const int&, std::shared_ptr<IdToken>,
      std::vector<std::string>&, std::vector<TypeMark>&);
  std::shared_ptr<Token> number();
  std::shared_ptr<LiteralToken<std::string>> string();
};

#endif // PARSER_H
